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
		exit(EXIT_FAILURE);
	}

	strcpy(input_copy, p->pipe);
	for(token = strtok_r(input_copy, "|", &ptr); token != NULL; token = strtok_r(NULL, "|", &ptr))
	{
		counter++;
	}

	return counter;
}

//msh_execute executes the parsed pipeline and returns after the pipeline completes
//M2: background pipes
void
msh_execute(struct msh_pipeline *p)
{
	//determine how many "|" separators there are
	//or in other words, how many commands there a
	int commmand_count = msh_pipeline_parse(p);

	//no forks are required if there is one program
	if(commmand_count == 1)
	{
		//read pipeline's program
		char * program = msh_command_program(p->pipeline_commands[0]);

		//retrieve the commands
		struct msh_command * c = msh_pipeline_command(p, 0);

		//arguments list
		char ** args_list = msh_command_args(c);

		//execute the one program
		if(execvp(program, args_list))
		{
			perror("msh_execute: execvp");
			exit(EXIT_FAILURE);
		}
	}

	//there is more than one program
	else
	{
		pid_t pid; //process ID

		//set up the pipe
		int fds[2];

		if(pipe(fds) == -1)
		{
			perror("pipe creation, opening pipe");
			exit(EXIT_FAILURE);
		}

		//for loop based on command_count
		int i; //i is for how many commands there are p[0], p[1], etc.
		//and also how many forks there are
		for(i = 0; i < commmand_count; i++)	
		{
			pid = fork();

			if(pid == -1)
			{
				perror("forking process");
				exit(EXIT_FAILURE);
			}

			//Child process
			else if(pid == 0)
			{
				//redirect STDOUT to write side of the pipe we just created.

				if(close(fds[0]) == -1) //not reading
				{
					perror("close");
					exit(EXIT_FAILURE);
				}

				if(dup2(fds[1], STDOUT_FILENO == -1))
				{
					perror("child dup stdout");
					exit(EXIT_FAILURE);
				}

				struct stat buf;

				if(fstat(STDOUT_FILENO, &buf) == -1)
				{
					perror("fstat");
					exit(EXIT_FAILURE);
				}

				if(!S_ISFIFO(buf.st_mode))
				{
					exit(EXIT_FAILURE);
				}

				//read pipeline's program
				char * program = msh_command_program(p->pipeline_commands[i]);

				//retrieve the commands
				struct msh_command * c = msh_pipeline_command(p, i);

				//arguments list
				char ** args_list = msh_command_args(c);

				//execute the one program
				if(execvp(program, args_list))
				{
					perror("msh_execute: execvp");
					exit(EXIT_FAILURE);
				}

				exit(EXIT_SUCCESS);
			}

			//Parent process
			else
			{
				//redirect STDIN to reader side of the pipe we just created.

				//not writing, close standard in
				if(close(STDIN_FILENO) == -1)
				{
					exit(EXIT_FAILURE);
				}

				//replace with input side of the pipe
				if(dup2(fds[0], STDIN_FILENO) == -1)
				{
					perror("parent dup stdin");
					exit(EXIT_FAILURE);
				}

				//be sure to close pipes or else
				//child will always wait for additional input

				//wait for the child process to finish (this changes later for background pipes)
				waitpid(pid, NULL, 0); //wait() waits for any child process
				//waitpid() refers to parent

				struct stat sbuf;

				if(fstat(STDIN_FILENO, &sbuf) == -1)
				{
					perror("fstat");
					exit(EXIT_FAILURE);
				}

				if(!S_ISFIFO(sbuf.st_mode))
				{
					exit(EXIT_FAILURE);
				}

				//scanf and printf here to read and write
				/*char buf[256];
				scanf("%s", buf);

				printf("%s\n", buf);*/
			}
		}
	}

	return;
}

//M2: set up signal handlers here
void
msh_init(void)
{
	return;
}