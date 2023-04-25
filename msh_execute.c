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
	pid_t pid; //process ID
	int fds[2]; //set up the pipe
	int carryover = fds[0];

	//STDIN = 0   fds[0] = read
	//STDOUT = 1  fds[1] = write

	for(int i = 0; i < command_count; i++) //iterate through every command
	{
		program = msh_command_program(p->pipeline_commands[i]);
		c = msh_pipeline_command(p, i);
		args_list = msh_command_args(c);

		if(pipe(fds) == -1) //set up the pipe
		{
			perror("pipe creation, opening pipe");
			exit(EXIT_FAILURE);
			return;
		}

		pid = fork(); //fork process returns 0 or the id of child

		if(pid == -1)
		{
			perror("forking failed");
			exit(EXIT_FAILURE);
			return;
		}

		else if(pid == 0)
		{
			//there is only one command
			if(command_count == 1)
			{
				//just execute program, but
				//close both of the file descriptors
				if (close(fds[0]) == -1)
				{
					perror("read close failed");
					exit(EXIT_FAILURE);
				}

				if(close(fds[1]) == -1)
				{
					perror("write close failed");
					exit(EXIT_FAILURE);
				}

				//execute the one program
				execvp(program, args_list);
				perror("msh_execute: execvp");
				exit(EXIT_FAILURE);
			}

			//the first command && not the only command
			if(i == 0 && msh_command_final(c) == 0)
			{
				//redirections
				//first command
				/*
				1. close the STDOUT
				2. DUP write end of pipe into STDOUT
				3. close both of the file descriptors
				4. exec
				*/
			
				//1. close the STDOUT
				if(close(STDOUT_FILENO) == -1)
				{
					exit(EXIT_FAILURE);
				}

				//2. DUP write end of pipe into STDOUT
				//redirect STDOUT to write side of the pipe we just created.
				dup2(fds[1], STDOUT_FILENO);

				//dup2(fds[0], STDIN_FILENO);
				

				//3. close both of the file descriptors
				if (close(fds[0]) == -1)
				{
					perror("read close failed");
					exit(EXIT_FAILURE);
				}

				if(close(fds[1]) == -1)
				{
					perror("write close failed");
					exit(EXIT_FAILURE);
				}

				//4. execute program
				execvp(program, args_list);

				//set carryover
				carryover = fds[0];
			}

			//the middle commands
			if(i != 0 && msh_command_final(c) == 0)
			{
				//redirections
				//close the STDIN
				if(close(STDIN_FILENO) == -1)
				{
					exit(EXIT_FAILURE);
				}

				dup2(carryover, STDIN_FILENO);

				if(close(STDOUT_FILENO) == -1)
				{
					exit(EXIT_FAILURE);
				}

				//close both of the file descriptors
				if (close(fds[0]) == -1)
				{
					perror("read close failed");
					exit(EXIT_FAILURE);
				}

				if(close(fds[1]) == -1)
				{
					perror("write close failed");
					exit(EXIT_FAILURE);
				}
				if(close(carryover) == -1)
				{
					exit(EXIT_FAILURE);
				}

				dup2(fds[1], STDOUT_FILENO);

				//execute program
				execvp(program, args_list);

				//set carryover
				carryover = fds[0];
			}

			//the last command
			if(msh_command_final(c) == 1)
			{
				//redirections
				//1. close the STDIN
				if (close(STDIN_FILENO) == -1)
				{
					exit(EXIT_FAILURE);
				}
				//2. Dup read end of pipe into STDIN
				dup2(carryover, STDIN_FILENO);

				//close the file descriptors
				if (close(fds[0]) == -1)
				{
					perror("read close failed");
					exit(EXIT_FAILURE);
				}

				if(close(fds[1]) == -1)
				{
					perror("write close failed");
					exit(EXIT_FAILURE);
				}

				if(close(carryover) == -1)
				{
					exit(EXIT_FAILURE);
				}

				//execute program
				execvp(program, args_list);
			}

			/* second command, 1 of two
			1.close the STDIN
			2. Dup read end of pipe into STDIN
			3. close both file descriptors
			4. exec
			*/
		else
		{
			//wait for all the commands, but not & commands
			for(int i = 0; i < command_count; i++)
			{
				//do not wait for background commands
				if(msh_pipeline_background(p) == 0)
				{
					wait(NULL);
				}
			}
		}

		}			
	} //outside for-loop
	
	//wait for all the commands, but not & commands
	/*for(int i = 0; i < command_count; i++)
	{
		//do not wait for background commands
		if(msh_pipeline_background(p) == 0)
		{
			wait(NULL);
		}
	}*/
	
	msh_pipeline_free(p); //free the pipeline
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
			printf("%d: We've been asked to terminate. Exit!\n", getpid());
			fflush(stdout);
			exit(EXIT_SUCCESS);
			break;
		}
		//foreground pipeline is suspended, able to receive input in shell again
		//user typed 'cntl-z', 'bg'
		case SIGTSTP: {
			printf("%d: We've been asked to suspend. Go to background\n", getpid());
			fflush(stdout);
			break;
		}
		//terminate all foreground processes with cntl-c
		case SIGINT: {
			//printf("%d:Terminate foreground process\n", getpid());

			printf("\n");
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
	//int ret;

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