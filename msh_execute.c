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

struct pid_struct {
	//global variable foregroung process
	pid_t fg_pid[16]; //process ID, one foreground child at a time, array for each 
	int pid_count;
};

struct pid_struct pid_global; //global instance of struct

//call after signal processes to reset to -1
void
reset_pid()
{ 
	pid_global.pid_count = 0; //reset pid count
}

//add particular pid into array
void
add_pid(pid_t pid)
{
	if(pid_global.pid_count == 16)
	{
		//no more allowed
		perror("past max args");
		return;
	}
	pid_global.fg_pid[pid_global.pid_count] = pid; //assign at the place and increment
	pid_global.pid_count++;
	//printf("add_pid:%d count:%d\n", (int) pid, (int) (pid_global.pid_count));
}

//send kill signal to every pid
void
kill_pid()
{
	//printf("kill_pid count:%d\n", pid_global.pid_count);
	for(int i = 0; i < pid_global.pid_count; i++)
	{
		//printf("killing: %d\n", (int) (pid_global.fg_pid[i]));
		kill(pid_global.fg_pid[i], SIGINT); //send to the specified pid
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
				//printf("adding pid: %d\n", pid);
				add_pid(pid);
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
	
	msh_pipeline_free(p); //free the pipeline ???
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
			printf("%d: We've been asked to suspend. Go to background\n", getpid());
			fflush(stdout);
			break;
		}
		//terminate foreground processes with cntl-c
		case SIGINT: {
			//printf("%d:Terminate foreground process\n", getpid());
			kill_pid();
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