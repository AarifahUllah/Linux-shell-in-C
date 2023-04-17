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

void
msh_execute(struct msh_pipeline *p)
{
	char * program; //read pipeline's program
	struct msh_command * c; //retrieve the commands
	char ** args_list; //arguments list
	int command_count = msh_pipeline_parse(p);//determine how many commands there are
	int carryover; // for fd[0]
	pid_t pid; //process ID
	struct stat buf;
	int fds[2]; //set up the pipe

	int i;
	for(i = 0; i < command_count; i++) //iterate through every command
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
		//STDIN = 0    fd[0] = read
		//STDOUT = 1   fd[1] = write
		else if(pid == 0) //left process
		{
			//redirect STDOUT to write side of the pipe we just created.
			if(close(STDOUT_FILENO) == -1) //not reading
			{
				perror("close");
				exit(EXIT_FAILURE);
				return;
			}

			if(dup2(carryover, STDIN_FILENO) == -1)
			{
				perror("parent dup stdin");
				exit(EXIT_FAILURE);
				return;
			}

			//case 1: the first command in pipeline & not the last
			if(i == 0 && msh_command_final(c) == 0)
			{
				//replace with input side of the pipe
				if(dup2(fds[1], STDOUT_FILENO) == -1)
				{
					perror("parent dup stdin");
					exit(EXIT_FAILURE);
					return;
				}
			}

			//dup2(fds[1], STDOUT_FILENO);

			//case 2: the middle commands
			/*if(i != 0 && msh_command_final(c) == 0)
			{
				//replace with input side of the pipe
				if(dup2(carryover, STDOUT_FILENO) == -1)
				{
					perror("parent dup stdin");
					exit(EXIT_FAILURE);
					return;
				}
			}*/

			if (close(fds[0]) == -1) {//not reading
				perror("close");
				exit(EXIT_FAILURE);
				return;
			}

			/*if(close(fds[1]) == -1)
			{
				perror("close");
				exit(EXIT_FAILURE);
				return;
			}*/

			if(fstat(STDOUT_FILENO, &buf) == -1)
			{
				perror("fstat");
				exit(EXIT_FAILURE);
				return;
			}

			if(!S_ISFIFO(buf.st_mode))
			{
				exit(EXIT_FAILURE);
				return;
			}

			//execute progam
			execvp(program, args_list);
			exit(EXIT_SUCCESS);
		}

		else //right process
		{
			//redirect STDIN to reader side of the pipe we just created.
			//not writing, close standard in
			if(close(STDIN_FILENO) == -1)
			{
				exit(EXIT_FAILURE);
				return;
			}

			//case 1: the first command in pipeline & not the last
			if(i == 0 && msh_command_final(c) == 0)
			{
				//replace with input side of the pipe
				if(dup2(fds[0], STDIN_FILENO) == -1)
				{
					perror("parent dup stdin");
					exit(EXIT_FAILURE);
					return;
				}
			}

			//case 2: middle commands
			if(i != 0 && msh_command_final(c) == 0)
			{
				//replace with input side of the pipe
				if(dup2(carryover, STDIN_FILENO) == -1)
				{
					perror("parent dup stdin");
					exit(EXIT_FAILURE);
					return;
				}
			}

			//be sure to close pipes or else
			//left process will always wait for additional input
			/*if (close(fds[0]) == -1)//not reading
			{
				perror("close");
				exit(EXIT_FAILURE);
				return;
			}*/
			if(close(fds[1]) == -1)
			{
				perror("close");
				exit(EXIT_FAILURE);
				return;
			}

			//wait for left-side process to finish
			waitpid(pid, NULL, 0); //waitpid() refers to right side

			if(fstat(STDIN_FILENO, &buf) == -1)
			{
				perror("fstat");
				exit(EXIT_FAILURE);
				return;
			}
			if(!S_ISFIFO(buf.st_mode))
			{
				exit(EXIT_FAILURE);
				return;
			}

			carryover = fds[0];

			//case 3: the last command
			
			//no more carryovers
			char buf[256];
			while(scanf("%s", buf) != EOF)
			{
				printf("%s\n", buf);
			}
		}
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



/*
  
  if (fork() == 0)
  {
	// replace cat's stdout with write part of 1st pipe
    dup2(pipes[1], 1);
    // close all pipes (very important!); end we're using was safely copied
    close(pipes[0]);
    close(pipes[1]);
    close(pipes[2]);
    close(pipes[3]);
    execvp(*cat_args, cat_args);
   }
  else
  {
	// fork second child (to execute grep)
	
	if (fork() == 0)
	{
	  // replace grep's stdin with read end of 1st pipe
	  
	  dup2(pipes[0], 0);
	  // replace grep's stdout with write end of 2nd pipe
	  dup2(pipes[3], 1);
	  // close all ends of pipes
	  close(pipes[0]);
	  close(pipes[1]);
	  close(pipes[2]);
	  close(pipes[3]);
	  execvp(*grep_args, grep_args);
	}
	
	else
	{
	  // fork third child (to execute cut)
	  if (fork() == 0)
	  {
		// replace cut's stdin with input read of 2nd pipe
	    dup2(pipes[2], 0);
	    // close all ends of pipes
		close(pipes[0]);
		close(pipes[1]);
		close(pipes[2]);
		close(pipes[3]);
	    execvp(*cut_args, cut_args);
		}
	}
    }
      
  // only the parent gets here and waits for 3 children to finish
  
  close(pipes[0]);
  close(pipes[1]);
  close(pipes[2]);
  close(pipes[3]);
  for (i = 0; i < 3; i++)
    wait(&status);
}*/