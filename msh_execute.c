#include <msh.h>
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
#include <fcntl.h>

//foreground process struct
struct fg_comms{
	pid_t fg_pid;
	char * fg_program; 
};

//background process struct
struct bg_comms {
	pid_t bg_pid; //the background program's pid, needed for fg or cntl-c
	char * bg_program; //the name of the background program, needed when using jobs
	char * bg_args; //the arguments that go with the program
};

//initialize fg and bg structs as global variables
struct fg_comms fg_commands[MSH_MAXBACKGROUND];
struct bg_comms bg_commands[MSH_MAXBACKGROUND];
int bg_count; //current count of background commands
int fg_count;


//Helper functions:

//add fg command to array
void
fg_add(pid_t pid, char *program)
{
	//reset the array everytime count is 0, needed once whole program finishes or after cntl-c
	if(fg_count == 0) memset(fg_commands, 0, sizeof(struct fg_comms) * MSH_MAXBACKGROUND);

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
	for(int i = 0; i < fg_count; i++) kill(fg_commands[i].fg_pid, SIGINT);
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

//redirection error checking
msh_err_t
redirection_parse(struct msh_command *c, struct msh_pipeline *p)
{
	int pipe_counter = msh_pipeline_parse(p) - 1; // "|", 1>, 1>>, 2>, 2>> counters
	int one_arrow = 0;
	int one_arrows = 0;
	int two_arrow = 0; 
	int two_arrows = 0;
	
	for(int i = 0; i < c->comm_args_count - 1; i++)
	{
		if(strcmp(c->comm_arguments[i], "1>") == 0) one_arrow++;
		if(strcmp(c->comm_arguments[i], "1>>") == 0) one_arrows++;
		if(strcmp(c->comm_arguments[i], "2>") == 0) two_arrow++;
		if(strcmp(c->comm_arguments[i], "2>>") == 0) two_arrows++;
	}
	
	//cannot have redirected to more than one file
	//this is not allowed: cmd 1> a.txt 1> b.txt 
	if((one_arrow > 1) || (one_arrows > 1) || (two_arrow > 1) || (two_arrows > 1))
	{
		printf("%s\n", msh_pipeline_err2str(-2));
		return -2;
	}

	//cannot have a redirection and output sent to a pipe
	if((one_arrow == 1) && (pipe_counter > 0))
	{
		printf("%s\n", msh_pipeline_err2str(-10));
		return -10;
	}

	if((one_arrows == 1) && (pipe_counter > 0))
	{
		printf("%s\n", msh_pipeline_err2str(-10));
		return -10;
	}

	if((two_arrow == 1) && (pipe_counter > 0))
	{
		printf("%s\n", msh_pipeline_err2str(-10));
		return -10;
	}

	if((two_arrows == 1) && (pipe_counter > 0))
	{
		printf("%s\n", msh_pipeline_err2str(-10));
		return -10;
	}

	return 0; //SUCCESS
}

//parse command for redirection symbols
int
redirect_stat(struct msh_command *c)
{
	for(int i = 0; i < c->comm_args_count - 1; i++)
	{
		if(strcmp(c->comm_arguments[i], "1>") == 0) return 1;

		if(strcmp(c->comm_arguments[i], "1>>") == 0) return 2;

		if(strcmp(c->comm_arguments[i], "2>") == 0) return 3;

		if(strcmp(c->comm_arguments[i], "2>>") == 0) return 4;
	}

	return 0;
}

//file to redirect to
char *
redirect_file(struct msh_command * c)
{
	char * file_name = NULL;
	for(int i = 0; i < c->comm_args_count - 1; i++)
	{
		if( (strcmp(c->comm_arguments[i], "1>>") == 0) ||
		    (strcmp(c->comm_arguments[i], "1>") == 0)  ||
			(strcmp(c->comm_arguments[i], "2>") == 0)  ||
			(strcmp(c->comm_arguments[i], "2>>") == 0))
		{
			file_name = c->comm_arguments[i + 1];
			c->comm_arguments[i] = NULL;
			c->comm_arguments[i + 1] = NULL;
			c->comm_args_count -= 2;
			return file_name;
		}
	}
	return file_name;
}
//end of helper functions


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
	int output_fd = 0;
	fg_count = 0;

	for(int i = 0; i < command_count; i++) //iterate through every command
	{
		program = msh_command_program(p->pipeline_commands[i]);
		c = msh_pipeline_command(p, i);
		args_list = msh_command_args(c);

		//check for redirection errors first
		if(redirection_parse(c, p) != 0)
		{
			msh_pipeline_free(p);
			exit(EXIT_FAILURE);
		}

		if(msh_pipeline_background(p) == 1) //change arg list for background command
		{
			for(int i = 0; i < c->comm_args_count; i++)
			{
				if(c->comm_arguments[i + 1] == NULL) //current command is &
				{
					c->comm_arguments[i] = c->comm_arguments[i + 1]; //remove & symbol for execvp
					c->comm_args_count--; //decrement argument count
				}
			}
		}

		//supporting built-in commands:
		if(strcmp(c->command, "exit") == 0) exit(EXIT_SUCCESS); //typing "exit" leaves the shells

		//typing cd changes the directory
		if(strcmp(c->command, "cd") == 0)
		{
			//ensuring user typed correct format; args count <= 3 (one for cd, one for argument, one for NULL)
			if(c->comm_args_count > 3) perror("msh: cd: too many arguments");

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
			char * ptr;
			int index = (int) strtol(c->comm_arguments[1], &ptr, 10);
			if(bg_commands[index].bg_pid != 0) //check if a command w/ index given is in background array
			{
				kill(bg_commands[index].bg_pid, SIGCONT);
				fg_add(bg_commands[index].bg_pid, bg_commands[index].bg_program);//add to foreground command array using fg_add function
				bg_commands[index].bg_pid = 0; //remove background command from array
				bg_count--;//decrement bg_count
				break;
			}
			else
			{
				printf("msh: fg: %d%s: no such job\n", index, ptr); ///background command does not exit w/ given index
				exit(EXIT_SUCCESS);
			}
		}

		//jobs prints out list of bg commands like this: [0] sleep 10
		if(strcmp(c->command, "jobs") == 0)
		{
			for(int j = 0; j < bg_count; j++) printf("[%d] %s\n", j, bg_commands[j].bg_program);
			exit(EXIT_SUCCESS);
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
		int redirect_status = redirect_stat(c); //check redirection status of command
	    char * file_redirect = redirect_file(c); //file redirection

		if(pid == -1)
		{
			perror("forking failed");
			msh_pipeline_free(p);
			exit(EXIT_FAILURE);
		}

		else if(pid == 0) //child process
		{
			if(carryover != 0) //not the first command
			{
				dup2(carryover, STDIN_FILENO); //read from carryover 
				close(carryover);
			}

			if(fds[1] != 0)
			{
				dup2(fds[1], STDOUT_FILENO); //DUP write end of pipe into STDOUT
				close(fds[1]); //redirect STDOUT to write side of the pipe we just created.
			}

			/*
			REDIRECTION: 1>, 1>>, 2>, 2>>
			1, for standout 2, for stderr
			> delete the file, then output to that file
			>> append to exiting file
			support these:
			1. redirection of stdout via a pipe (M1)
			2. redirection of stdout to a file
			3. no redirection of stdout - goes to command line
			4. redirection of stderr to a file

			STDIN = 0   fds[0] = read
			STDOUT = 1  fds[1] = write
			*/

			//redirect output
			if(redirect_status == 1) // case: 1>
			{
				output_fd = open(file_redirect, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				dup2(output_fd, STDOUT_FILENO);
				close(output_fd);
			}

			if(redirect_status == 2) //case: 1>>
			{
				output_fd = open(file_redirect, O_WRONLY | O_CREAT | O_APPEND, 0666);
				dup2(output_fd, STDOUT_FILENO);
				close(output_fd);
			}

			if(redirect_status == 3) //case: 2>
			{
				output_fd = open(file_redirect, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				dup2(output_fd, STDERR_FILENO);
				close(output_fd);
			}

			if(redirect_status == 4) //case: 2>>
			{
				output_fd = open(file_redirect, O_WRONLY | O_CREAT | O_APPEND, 0666);
				dup2(output_fd, STDERR_FILENO);
				close(output_fd);
			}

			if(i < (command_count - 1)) close(fds[0]);

			execvp(program, args_list); //execute program
			perror("msh_execute: execvp"); //execvp doesn't work
			exit(EXIT_FAILURE);
		}
		else //Parent process has child's pid
		{
			if(carryover != 0) close(carryover);
			
			if(fds[1] != 0) close(fds[1]);

			if(i < (command_count - 1)) close(fds[1]);

			carryover = fds[0]; //reassign carryover

			if(msh_pipeline_background(p) == 0) fg_add(pid, program); //foreground process

			//add background process to array that can be later found by typing jobs
			if(msh_pipeline_background(p) == 1)
			{
				if(bg_count == MSH_MAXBACKGROUND) perror("max background limit reached"); //check MSH_MAXBACKGROUND limit is not reached

				//memset first background
				if(bg_count == 0) memset(bg_commands, 0, sizeof(struct bg_comms) * MSH_MAXBACKGROUND);
				for(int i = 0; i < MSH_MAXBACKGROUND; i++)
				{
					if(bg_commands[i].bg_pid == 0)
					{
						bg_commands[i].bg_pid = pid; // add the command to array of background commands
						bg_commands[i].bg_program = program;
						bg_commands[i].bg_args = c->comm_arguments[1];
						bg_count++; //incrememnt background command count
						printf("[%d] %s %s\n", i, program, c->comm_arguments[1]); //print job order, command, & args
						break;
					}
				}
			}
		}		
	} //outside for-loop
	
	for(int i = 0; i < command_count; i++) //wait for commands
	{
		if(msh_pipeline_background(p) == 0) wait(NULL); //except for the background commands
		fg_count = 0;
	}

	msh_pipeline_free(p);
	return;
}

void
sig_handler(int signal_number, siginfo_t *info, void *context)
{
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
			printf("\n%d: Cntl-Z pressed. We've been asked to suspend. Go to background\n", getpid());
			fflush(stdout);
			bg_count++;//increment bg count
			break;
		}
		//terminate foreground processes with cntl-c
		case SIGINT: {
			printf("\n%d: Cntl-C pressed. Terminate foreground process\n", getpid());
			fflush(stdout);
			fg_kill();
			break;
		}
		//run background command to foreground - user typed 'fg'
    	case SIGCONT: {
			fflush(stdout);
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