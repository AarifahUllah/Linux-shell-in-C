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
struct fg_comms{
	pid_t fg_pid;
	char * fg_program; 
};

//background process struct
struct bg_comms {
	pid_t bg_pid; //the background program's pid, needed for fg or cntl-c
	char * bg_program; //the name of the background program, needed when using jobs
};

//initialize fg and bg structs as global variables
struct fg_comms fg_commands[MSH_MAXBACKGROUND];
struct bg_comms bg_commands[MSH_MAXBACKGROUND];
int bg_count; //current count of background commands
int fg_count; //current count of foreground commands (unsure if needed)


//Helper functions:

//add fg command to array
void
fg_add(pid_t pid, char *program)
{
	//reset the array everytime count is 0, needed once whole program finishes or after cntl-c
	if(fg_count == 0) memset(fg_commands, 0, sizeof(fg_comms) * MSH_MAXBACKGROUND);

	for(int i = 0; i < fg_count; i++)
	{
		if(fg_commands[i].fg_pid == 0)
		{
			fg_commands[i].fg_pid = pid;
			fg_commands[i].fg_program = program;
			fg_count++;
			break;
		}
	}
}

//send kill signal to every foreground pid
void
fg_kill()
{
	for(int i = 0; i < fg_pids.fg_pid_count; i++) kill(fg_commands[i].fg_pid, SIGINT);
	fg_count = 0; //reset foreground command count after kiling each process
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

//parse command for redirection symbols (do redirection error checking here later)
int
redirect(struct msh_command *c)
{
	if(strstr(c->comm_arguments, "1>") != NULL) return 1;

	if(strstr(c->comm_arguments, "1>>") != NULL) return 2;

	if(strstr(c->comm_arguments, "2>") != NULL) return 3;

	if(strstr(c->comm_arguments, "2>>") != NULL) return 4;

	else return 0; //return 0 if no redirects
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
			int index = (int) c->comm_arguments;
			printf("index: %d\n", index); //check the casting
			//check if a command w/ index given is in background array
			if(bg_commands[index] != 0)
			{
				kill(bg_commands.bg_pid, SIGCONT);
				fg_add(bg_commands[index].bg_pid, bg_commands[index].bg_program);//add to foreground command array using fg_add function
				bg_commands[index].bg_pid = 0; //remove background command from array
				bg_count--;//decrement bg_count
			}
			else perror("process doesn't exist"); ///background command does not exit w/ given index (find exact message)
		}

		//do we need bg?
		if(strcmp(c->command, "bg") == 0)
		{
			//check if command is already in background
		}

		//jobs prints out list of bg commands like this: [0] sleep 10
		if(strcmp(c->command, "jobs") == 0)
		{
			for(int i = 0; i < bg_count; i++) printf("[%d] %s\n", i, bg_commands[i].bg_program);
		}

		//REDIRECTION: 1>, 1>>, 2>, 2>>

		//STDIN = 0   fds[0] = read
		//STDOUT = 1  fds[1] = write
		
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

		int redirect_status = redirect(c); //check redirection status of command

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

			//add background process to array that can be later found by typing jobs
			if(msh_pipeline_background(p) == 1)
			{
				//memset first background
				if(bg_count == 0) memset(bg_commands, 0, sizeof(struct bg_commands_struct) * MSH_MAXBACKGROUND);

				//check that MSH_MAXBACKGROUND limit is not reached
				if(bg_count == MSH_MAXBACKGROUND) perror("max background limit reached");

				for(int i = 0; i < bg_count; i++)
				{
					if(bg_commands[i].bg_pid == 0)
					{
						bg_commands[i].bg_pid = pid; // add the command to array of background commands
						bg_commands[i].bg_program = program; //need to fix
						bg_count++; //incrememnt background command count
						printf("[%d] %d\n", i, pid); //print job order and process id
						break;
					}
				}
			}

			if(carryover != 0) close(carryover);
			
			if(fds[1] != 0) close(fds[1]);

			if(i < (command_count - 1)) close(fds[1]);

			carryover = fds[0]; //reassign carryover
		}		
	} //outside for-loop
	//wait for all the commands except for background commands
	for(int i = 0; i < command_count; i++)
	{
		if(msh_pipeline_background(p) == 0) wait(NULL);
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
		//terminate a process, sent to pipeline processes
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
			bg_count++;//increment bg count
			break;
		}
		//terminate foreground processes with cntl-c
		case SIGINT: {
			printf("%d:Cntl-C pressed. Terminate foreground process\n", getpid());
			fflush(stdout);
			fg_kill();
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