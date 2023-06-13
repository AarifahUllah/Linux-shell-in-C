#include <msh.h>
#include <msh_execute.h>
#include <msh_parse.h>
#include <msh_parse.c>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

//foreground process struct
struct fg_pid_struct {
	pid_t fg_pid[16]; //process ID, one foreground child at a time, array for each 
	int fg_pid_count;
};

struct fg_pid_struct fg_pids;

//call after signal processes to reset to -1
void
fg_pid_reset()
{ 
	fg_pids.fg_pid_count = 0; //reset foreground pid count
}

//add particular fg pid into array
void
fg_pid_add(pid_t pid)
{
	if (fg_pids.fg_pid_count == 16) perror("past max args");

	fg_pids.fg_pid[fg_pids.fg_pid_count] = pid;
	fg_pids.fg_pid_count++;
}

//send kill signal to every foreground pid
void
kill_pid()
{
	for(int i = 0; i < fg_pids.fg_pid_count; i++)
	{
		printf("killing: %d\n", (int) fg_pids.fg_pid[i]);
		kill(fg_pids.fg_pid[i], SIGINT); //send to the specified pid at index i
	}
}

int
msh_pipeline_parse(struct msh_pipeline *p)
{ 
	char *token, *ptr;
	char * input_copy = malloc(strlen(p->pipe) + 1);
	int counter = 0; //count how many "|" separators there are

	//malloc() failure
	if(input_copy == NULL)
	{
		perror("msh_execute: malloc failure");
		free(input_copy);
		exit(EXIT_FAILURE);
	}

	strcpy(input_copy, p->pipe);
	for(token = strtok_r(input_copy, "|", &ptr); token != NULL; token = strtok_r(NULL, "|", &ptr))
	{
		counter++;
	}

	free(input_copy);
	return counter;
}

void
msh_execute(struct msh_pipeline *p)
{
	char * program; //read pipeline's program
	struct msh_command * c; //retrieve the commands
	char ** args_list; //arguments list
	int command_count = msh_pipeline_parse(p);//determine how many commands there are
	pid_t pid;
	int fds[2]; //set up the pipe
	int carryover = 0;

	//STDIN = 0   fds[0] = read
	//STDOUT = 1  fds[1] = write
	for(int i = 0; i < command_count; i++) //iterate through every command
	{
		program = msh_command_program(p->pipeline_commands[i]);
		c = msh_pipeline_command(p, i);
		args_list = msh_command_args(c);

		//supporting built-in commands:
		//typing "exit" leaves the shell
		if(strcmp(c->command, "exit") == 0) exit(EXIT_SUCCESS);

		//typing cd changes the directory
		if(strcmp(c->command, "cd") == 0)
		{
			//ensuring user typed correct format; args count <= 3 (one for cd, one for argument, one for NULL)
			if(c->comm_args_count > 3) perror("bash: cd: too many arguments");

			char * directory = c->comm_arguments[1]; //cd [directory] is just one argument

			//cd to return home
			if(directory == NULL || strcmp(directory, "~") == 0)
			{
				if(chdir(getenv("HOME")) == -1) perror("chdir");
			}

			else
			{
				//cd w/ a specified path using ~
				if(directory[0] == '~')
				{
					char * home_directory = getenv("HOME");
					char * new_directory = malloc(strlen(home_directory) + strlen(directory) + 1);
					if(new_directory == NULL) perror("cd malloc() failure");
					strcpy(new_directory, home_directory); //add home directory as part of the new directory
					strcat(new_directory, &directory[1]); //concatenate the previous directory to the new one
					directory = new_directory; //reassign to "directory" variable for use in line 125
				}
				//cd using relative path
				if(chdir(directory) == -1)
				{
					printf("bash: %s: No such file or directory\n", directory);
					exit(EXIT_FAILURE);
				}

				if(directory != c->comm_arguments[1]) free(directory); //free the potential malloc() on line 118
			}

			continue; //other commands that still need to be executed
		}

		//changing current commands to background/foreground
		if(strcmp(c->command, "fg") == 0)
		{
			//check if command is already in foreground

		}

		if(strcmp(c->command, "bg") == 0)
		{
			//check if command is already in background
		}

		//user typed jobs
		if(strcmp(c->command, "jobs") == 0)
		{
			//print out list of jobs

		}

		//executing commands using pipes & execvp
		if(i < (command_count - 1))
		{
			if(pipe(fds) == -1) //set up the pipe
			{
				perror("pipe creation, opening pipe");
				exit(EXIT_FAILURE);
			}
		}

		pid = fork(); //fork process returns 0 or the id of child

		if(pid == -1)
		{
			perror("forking failed");
			msh_pipeline_free(p);
			exit(EXIT_FAILURE);
		}

		else if(pid == 0)
		{
			if(carryover != 0)
			{
				dup2(carryover, STDIN_FILENO);

				close(carryover);
			}

			if(fds[1] != 0)
			{
				//DUP write end of pipe into STDOUT
				//redirect STDOUT to write side of the pipe we just created.
				dup2(fds[1], STDOUT_FILENO);

				close(fds[1]);
			}

			if(i < (command_count - 1)) close(fds[0]);

			execvp(program, args_list); //execute program
		}
		else //Parent process has child's pid
		{
			//foreground process
			if(msh_pipeline_background(p) == 0)
			{
				//set global to child process's id
				printf("adding pid: %d\n", pid);
				fg_pid_add(pid);
			}

			if(carryover != 0) close(carryover);
			
			if(fds[1] != 0) close(fds[1]);

			if(i < (command_count - 1)) close(fds[1]);

			carryover = fds[0]; //reassign carryover
		}		
	} //outside for-loop
	//wait for all the commands, but not & commands
	for(int i = 0; i < command_count; i++)
	{
		//do not wait for background commands
		if(msh_pipeline_background(p) == 0) wait(NULL);
	}
	for(int i = 0; i < fg_pids.fg_pid_count; i++)
	{
		printf("i: %d count: %d pid: %d\n", i, fg_pids.fg_pid_count, fg_pids.fg_pid[i]);
	}
	
	msh_pipeline_free(p);
	return;
}

void
sig_handler(int signal_number, siginfo_t *info, void *context)
{
	//printf("sig handler was called:%d\n", signal_number);
	(void) info;
	(void) context;
	switch(signal_number){
		//the child process has terminated
		case SIGCHLD: {
			//printf("%d: Child process %d has exited.\n", getpid(), info->si_pid);
			//fflush(stdout);
			break;
		}
		//terminate a process, sent to pipeline processes (cntl-z)
		case SIGTERM: {
			//printf("%d: We've been asked to terminate. Exit!\n", getpid());
			fflush(stdout);
			exit(EXIT_SUCCESS);
			break;
		}
		//foreground pipeline is suspended, able to receive input in shell again
		//user typed 'cntl-z', 'bg'
		case SIGTSTP: {
			printf("%d: Cntl-Z pressed. We've been asked to suspend. Go to background\n", getpid());
			fflush(stdout);
			break;
		}
		//terminate foreground processes with cntl-c
		case SIGINT: {
			printf("%d:Cntl-C pressed. Terminate foreground process\n", getpid());
			fflush(stdout);
			kill_pid();
			fg_pid_reset();
			break;
		}
		//run a background command to foreground - user typed 'fg'
    	case SIGCONT: {
			break;
		}
	}
}

void
setup_signal(int signo, void (*fn)(int, siginfo_t *, void *))
{
	sigset_t masked;
	struct sigaction siginfo;

	sigemptyset(&masked);
	sigaddset(&masked, signo);
	siginfo = (struct sigaction) {
		.sa_sigaction = fn,
		.sa_mask = masked,
		.sa_flags = SA_SIGINFO
	};

	if (sigaction(signo, &siginfo, NULL) == -1)
	{
		perror("sigaction error");
		exit(EXIT_FAILURE);
	}
}

//set up signal handlers here
void
msh_init(void)
{
	setup_signal(SIGCHLD, sig_handler);
	setup_signal(SIGTERM, sig_handler);
	setup_signal(SIGTSTP, sig_handler);
	setup_signal(SIGINT, sig_handler);
	setup_signal(SIGCONT, sig_handler);

	return;
}