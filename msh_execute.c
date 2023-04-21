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
	//int fds[2]; //set up the pipe
	//int carryover = STDIN_FILENO;

	printf("calling execute\n");
	for(int i = 0; i < command_count; i++) //iterate through every command
	{
		program = msh_command_program(p->pipeline_commands[i]);
		c = msh_pipeline_command(p, i);
		args_list = msh_command_args(c);

		//there is only one command in the pipeline
		/*if(command_count == 1)
		{
			//execute the one program
			if(execvp(program, args_list)) //replaced with code for pwd, ls doesn't go to ./msh
			{
				perror("msh_execute: execvp");
				exit(EXIT_FAILURE);
			}
			return; //not reached
		}*/

		//more than one program with "|" separators:

		/*if(pipe(fds) == -1) //set up the pipe
		{
			perror("pipe creation, opening pipe");
			exit(EXIT_FAILURE);
			return;
		}*/

		pid = fork(); //fork process returns 0 or the id of child

		if(pid == -1)
		{
			perror("forking failed");
			exit(EXIT_FAILURE);
			return;
		}
		
		else if(pid == 0)
		{
			//the first command && not the only command
			if(i == 0 && msh_command_final(c) == 0)
			{
				//replace with input side of the pipe
				/*if(dup2(fds[1], STDOUT_FILENO) == -1)
				{
					perror("parent dup stdin");
					exit(EXIT_FAILURE);
					return;
				}*/
				/*if(dup2(fds[0], STDIN_FILENO) == -1)
				{
					perror("parent dup stdin");
					exit(EXIT_FAILURE);
					return;
				}*/
			}

			//the middle commands
			if(i != 0)
			{
				//redirect STDOUT to write side of the pipe we just created.
				/*if(close(STDIN_FILENO) == -1) //not reading
				{
					perror("close");
					exit(EXIT_FAILURE);
					return;
				}

				if(dup2(carryover, STDIN_FILENO) == -1)
				{
					perror("child dup stdin");
					exit(EXIT_FAILURE);
					return;
				}

				if (close(fds[0]) == -1)//not reading
				{
					perror("close");
					exit(EXIT_FAILURE);
					return;
				}*/
			}

			//not the last command
			if(msh_command_final(c) == 0)
			{
				/*close(STDOUT_FILENO);

				//replace with input side of the pipe
				if(dup2(fds[1], STDOUT_FILENO) == -1)
				{
					perror("child dup stdout");
					exit(EXIT_FAILURE);
					return;
				}

				if (close(fds[0]) == -1)//not reading
				{
					perror("close");
					exit(EXIT_FAILURE);
					return;
				}*/
			}

			//execute progam, no code runs after
			execvp(program, args_list);
			perror("exec failed"); //exec returns only if it fails
			exit(EXIT_FAILURE);
		}
		printf("created child process with pid:%d\n", pid);

		//reassign
		/*carryover = fds[0];

		if(close(fds[1]) == -1)
		{
				perror("close");
				exit(EXIT_FAILURE);
				return;
		}*/
			
	} //outside for-loop

	//if fds[1] doesn't use stdout then close
	/*if (fds[1] != STDOUT_FILENO)
	{
		close(fds[1]);
	
	}

	if(carryover != STDIN_FILENO)
	{
		close(carryover);
	}*/
	
	//wait for all the commands
	//note: wait is in order of execute
	for(int i = 0; i < command_count; i++)
	{
		int pid = wait(NULL);
		printf("188: wait return pid:%d\n", pid);
	}
	
	msh_pipeline_free(p); //free the pipeline
	return;
}

/*void
sig_handler(int signal_number, siginfo_t *info, void *context)
{
	switch(signal_number){
		//the child process has terminated
		case SIGCHLD: {
			printf("%d: Child process %d has exited.\n", getpid(), info->si_pid);
			fflush(stdout);
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
		//terminate a foreground process with cntl-c
		case SIGINT: {
			break;
		}
		//run a background command to foreground - user typed 'fg'
    	case SIGCONT: {
			break;
		}
	}

	return;
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
}*/

//M2: set up signal handlers here
void
msh_init(void)
{
	/*
	 * The signal information is inherited across a fork,
	 * and is set the same for the parent and child.
	 */

	//setup_signal(SIGCHLD, sig_handler);
	//setup_signal(SIGTERM, sig_handler);

	return;
}