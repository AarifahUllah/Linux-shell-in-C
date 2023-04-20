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
	int carryover = STDIN_FILENO;

	for(int i = 0; i < command_count; i++) //iterate through every command
	{
		program = msh_command_program(p->pipeline_commands[i]);
		c = msh_pipeline_command(p, i);
		args_list = msh_command_args(c);

		//there is only one command in the pipeline
		if(command_count == 1)
		{
			//execute the one program
			if(execvp(program, args_list))
			{
				perror("msh_execute: execvp");
				exit(EXIT_FAILURE);
			}
			return;
		}

		//more than one program with "|" separators:

		if(pipe(fds) == -1) //set up the pipe
		{
			perror("pipe creation, opening pipe");
			exit(EXIT_FAILURE);
			return;
		}

		pid = fork(); //fork process

		if(pid == -1)
		{
			perror("forking process");
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
				if(dup2(fds[0], STDIN_FILENO) == -1)
				{
					perror("parent dup stdin");
					exit(EXIT_FAILURE);
					return;
				}
			}

			//the middle commands
			if(i != 0)
			{
				//redirect STDOUT to write side of the pipe we just created.
				if(close(STDIN_FILENO) == -1) //not reading
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
				}
			}

			//not the last command
			if(msh_command_final(c) == 0)
			{
				close(STDOUT_FILENO);

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
				}
			}

			//execute progam
			execvp(program, args_list);
			exit(EXIT_SUCCESS);
		}

		//reassign
		carryover = fds[0];

		if(close(fds[1]) == -1)
		{
				perror("close");
				exit(EXIT_FAILURE);
				return;
		}
			
	} //outside for-loop

	//if fds[1] doesn't use stdout then close
	if (fds[1] != STDOUT_FILENO)
	{
		close(fds[1]);
	
	}

	if(carryover != STDIN_FILENO)
	{
		close(carryover);
	}
	
	//wait for all the commands
	//note: wait is in order of execute
	for(int i = 0; i < command_count; i++)
	{
		wait(NULL);
	}
	
	msh_pipeline_free(p); //free the pipeline
	return;
}


//M2: set up signal handlers here
void
msh_init(void)
{
	return;
}