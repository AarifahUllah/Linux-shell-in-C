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

	//determine how many commands there are
	int command_count = msh_pipeline_parse(p);


	//no forks are required if there is one program
	if(command_count == 1)
	{
		program = msh_command_program(p->pipeline_commands[0]);
	    c = msh_pipeline_command(p, 0);
		args_list = msh_command_args(c);

		//execute the one program
		if(execvp(program, args_list))
		{
			perror("msh_execute: execvp");
			exit(EXIT_FAILURE);
		}
	}

	pid_t pid; //process ID
	int status; //for parent to wait on the children
	
	//make (command_count - 1) amount of pipes 
	//each of the pipes has 2 fds
	int fds[2*(command_count - 1)]; 
	
	//set up each of the pipes
	int j;
	for(j = 0; j < (2*(command_count - 1)); j+=2)
	{
		if(pipe(fds + j) == -1)
		{
			perror("pipe creation, opening pipe");
			exit(EXIT_FAILURE);
		}
	}

	//fork() when there is more than one program
	int i;
	for(i = 0; i < command_count; i++)
	{
		pid = fork();
	}

	if(pid == -1)
	{
		perror("forking process");
		exit(EXIT_FAILURE);
	}

	//first fork
	else if (pid == 0)
	{
		dup2(fds[1], 1);

		int t;
		for(t = 0; t < 2*(command_count-1); t++)
		{
			close(fds[t]);
		}

		if(execvp(program, args_list))
		{
			perror("msh_execute: execvp");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}

	//second fork
	else
	{
		if(pid == 0)
		{
			dup2(fds[0],0);

			dup2(fds[3],1);

			int r;
			for(r = 0; r < 2*(command_count-1); r++)
			{
				close(fds[r]);
			}

			program = msh_command_program(p->pipeline_commands[1]);
			c = msh_pipeline_command(p, 1);
			args_list = msh_command_args(c);

	    	if(execvp(program, args_list))
			{
				perror("msh_execute: execvp");
				exit(EXIT_FAILURE);
			}
			exit(EXIT_SUCCESS);
		}

		else
		{
			if(pid == 0)
			{
				dup2(fds[2], 0);
				int r;
				for(r = 0; r < 2*(command_count-1); r++)
				{
					close(fds[r]);
				}

				program = msh_command_program(p->pipeline_commands[2]);
				c = msh_pipeline_command(p, 2);
				args_list = msh_command_args(c);
				if(execvp(program, args_list))
				{
					perror("msh_execute: execvp");
					exit(EXIT_FAILURE);
				}
			}
		}
	}

	//the parent process, waits for all child processes to finish
	int k;
	for(k = 0; k < 2*(command_count-1); k++)
	{
		close(fds[k]);
	}

	int m;
	for(m = 0; m < command_count; m++)
	{
		wait(&status);
	}

	return;
}

//msh_execute executes the parsed pipeline and returns after the pipeline completes
//M2: background pipes
/*void
msh_execute(struct msh_pipeline *p)
{
	char * program; //read pipeline's program
	struct msh_command * c; //retrieve the commands
	char ** args_list; //arguments list

	//determine how many commands there are
	int commmand_count = msh_pipeline_parse(p);


	//no forks are required if there is one program
	if(commmand_count == 1)
	{
		program = msh_command_program(p->pipeline_commands[0]);
	    c = msh_pipeline_command(p, 0);
		args_list = msh_command_args(c);

		//execute the one program
		if(execvp(program, args_list))
		{
			perror("msh_execute: execvp");
			exit(EXIT_FAILURE);
		}
	}

	pid_t pid; //process ID
	struct stat buf;
	int status; //for parent to wait on the children
	int i;
	
	//make (command_count - 1) amount of pipes 
	//each of the pipes has 2 fds
	int fds[2]; //set up the pipe

	if(pipe(fds) == -1)
	{
		perror("pipe creation, opening pipe");
		exit(EXIT_FAILURE);
	}

	//fork() when there is more than one program
	int i;
	for(i = 0; i < commmand_count; i++)
	{
		pid = fork();
	}

	if(pid == -1)
	{
		perror("forking process");
		exit(EXIT_FAILURE);
	}

	//Child process
	else if(pid == 0)
	{
		//redirect STDOUT to write side of the pipe we just created.

		if(close(STDOUT_FILENO) == -1) //not reading
		{
			perror("close");
			exit(EXIT_FAILURE);
		}

		dup2(fds[1], STDOUT_FILENO);

		if (close(fds[0]) == -1) {//not reading
			perror("close");
			exit(EXIT_FAILURE);
        }

		if(close(fds[1]) == -1)
		{
			perror("close");
			exit(EXIT_FAILURE);
		}

		if(fstat(STDOUT_FILENO, &buf) == -1)
		{
			perror("fstat");
			exit(EXIT_FAILURE);
		}

			if(!S_ISFIFO(buf.st_mode))
			{
				exit(EXIT_FAILURE);
			}

			int k;
			for(k = 0; k < commmand_count; k++)
			{
				program = msh_command_program(p->pipeline_commands[i]); //read pipeline's program
			
				c = msh_pipeline_command(p, i); //retrieve the commands

				args_list = msh_command_args(c); //arguments list

				//execute program
				execvp(program, args_list);
				exit(EXIT_SUCCESS);
			}
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
		if (close(fds[0]) == -1)//not reading
		{
			perror("close");
			exit(EXIT_FAILURE);
       	}

		if(close(fds[1]) == -1)
		{
			perror("close");
			exit(EXIT_FAILURE);
		}

		//wait for the child process to finish (this changes later for background pipes)
		waitpid(pid, NULL, 0); //wait() waits for any child process
		//waitpid() refers to parent

		if(fstat(STDIN_FILENO, &buf) == -1)
		{
			perror("fstat");
			exit(EXIT_FAILURE);
		}

		if(!S_ISFIFO(buf.st_mode))
		{
			exit(EXIT_FAILURE);
		}
		char buf[256];
		scanf("%s", buf);
		printf("%s\n", buf);
	}
	return;
}*/

//M2: set up signal handlers here
void
msh_init(void)
{
	return;
}



/*#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


int main(int argc, char **argv)
{
  int status;
  int i;

  // arguments for commands; your parser would be responsible for
  // setting up arrays like these

  char *cat_args[] = {"cat", "scores", NULL};
  char *grep_args[] = {"grep", "Villanova", NULL};
  char *cut_args[] = {"cut", "-b", "1-10", NULL};

  // make 2 pipes (cat to grep and grep to cut); each has 2 fds

  int pipes[4];
  pipe(pipes); // sets up 1st pipe
  pipe(pipes + 2); // sets up 2nd pipe

  // we now have 4 fds:
  // pipes[0] = read end of cat->grep pipe (read by grep)
  // pipes[1] = write end of cat->grep pipe (written by cat)
  // pipes[2] = read end of grep->cut pipe (read by cut)
  // pipes[3] = write end of grep->cut pipe (written by grep)

  // Note that the code in each if is basically identical, so you
  // could set up a loop to handle it.  The differences are in the
  // indicies into pipes used for the dup2 system call
  // and that the 1st and last only deal with the end of one pipe.

  // fork the first child (to execute cat)
  
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