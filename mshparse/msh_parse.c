#include <msh_parse.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//define msh's structs
struct msh_command{
	char* comm_arguments[MSH_MAXARGS + 2]; //plus 1 for the command/program itself plus 1 for NULL termination
	bool command_last; //boolean flag for last command
	int comm_args_count; //how many arguments in a command so far
	char* command; 
};

//a pipeline is a set of commands
//a struct array of msh_commands
//of length MSH_MAXCMNDS
struct msh_pipeline{
	struct msh_command * pipeline_commands[MSH_MAXCMNDS];
	char * pipe; 
	int background_pipe;
	int pipeline_comm_count;//how many commands in a pipe so far
};

//a sequence is a set of pipelines
//1 foreground pipeline and a maximum
//of MSH_MAXBACKGROUND amount of background pipelines
struct msh_sequence{
	struct msh_pipeline * sequence_pipelines[MSH_MAXBACKGROUND + 1];
	int cur;
	int seq_pipeline_count;//how many pipelines are in sequence so far
};

//free the passed in pipeline
void
msh_pipeline_free(struct msh_pipeline *p)
{
	free(p->pipe); //free pointer to pipe

	int i;
	for (i = 0; i < MSH_MAXCMNDS; i++)
	{
		if(p->pipeline_commands[i] != NULL)
		{			
			free(p->pipeline_commands[i]->command); //free the command pointer
			
			int j;
			for(j = 0; j < (MSH_MAXARGS + 1); j++)
			{
				if(p->pipeline_commands[i]->comm_arguments[j] != NULL)
				{
					free(p->pipeline_commands[i]->comm_arguments[j]); //free arguments
				}
			}
			free(p->pipeline_commands[i]); //free the command
		}
	}

	free(p); //free overall pipeline struct
}

//deallocate the entire sequence except
//the piepelines that have already been removed
//using msh_sequence_pipeline
void
msh_sequence_free(struct msh_sequence *s)
{
	//free internal structures for remaining pipelines
	//some pipelines were already freed by msh_pipeline_free
	int i;
	for(i = 0; i < s->seq_pipeline_count; i++)
	{
		//check if pipeline is already free by msh_pipeline_free
		if(s->sequence_pipelines[i] != NULL)
		{
			//free pipeline using msh_pipeline_free()
			msh_pipeline_free(s->sequence_pipelines[i]);
		}
	}
	free(s); //free outer sequence structure
}

//allocate the sequence structure
struct msh_sequence *
msh_sequence_alloc(void)
{
	//allocate space w/ calloc for msh_sequence
	struct msh_sequence * s = calloc(1, sizeof(struct msh_sequence));

	//calloc() fails in allocating memory for the sequence
	if(s == NULL)
	{
		printf("msh_sequence_alloc: calloc() failure\n");
		return NULL;
	}

	//initialize cur and pipeline count
	s->cur = 0;
	s->seq_pipeline_count = 0;

	return s;
}

//return the passed in pipeline's input
//that was used to make that pipeline in the first place
char *
msh_pipeline_input(struct msh_pipeline *p)
{
	return p->pipe;
}

//takes a string that has pipelines and commands
//and puts them into the given sequence  
msh_err_t
msh_sequence_parse(char *str, struct msh_sequence *seq)
{
	int index = 0; //current position in sequence, which is an array of struct pipelines
	char *token, *ptr; 
	char *str_copy = malloc(strlen(str) + 1);

	//malloc() failure
	if(str_copy == NULL)
	{
		printf("MSH Error: %s\n", msh_pipeline_err2str(-5));
		return -5;
	}

	strcpy(str_copy, str);
	for(token = strtok_r(str_copy, ";", &ptr); token != NULL; token = strtok_r(NULL, ";", &ptr))
	{
		//error checking for pipelines
		int error = 0;
		int i;
		for (i = 0; i < (int)strlen(token); i++)
		{
			if(token[i] == '|' && error == 0)
			{
				free(str_copy);
				printf("MSH Error: %s\n", msh_pipeline_err2str(-8));
				return -8;
			}

			else if(token[i] != ' ')
			{
				error = 1;
			}
		}

		if(token[strlen(token) - 1] == '|')
		{
			free(str_copy);
			printf("MSH Error: %s\n", msh_pipeline_err2str(-8));
			return -8;
		}
		
		//allocate pipelines
		seq->sequence_pipelines[index] = calloc(1, sizeof(struct msh_pipeline));

		//calloc() failure
		if(seq->sequence_pipelines[index] == NULL)
		{
			free(str_copy);
			printf("MSH Error: %s\n", msh_pipeline_err2str(-5));
			return -5;
		}
		seq->sequence_pipelines[index]->pipe = strdup(token); //put token into sequence

		//check whether to run the pipeline in the background
		for(size_t i = 0; i < strlen(token); i++)
		{
			int and_counter = 0;
			if(token[i] == '&')
			{
				and_counter++;

				//cannot have more arguments after &
				if((token[i + 1] != '\0'))
				{
					printf("%s\n", msh_pipeline_err2str(-3));
					return -3;
				}
			}

			if(and_counter > 1)
			{
				printf("%s\n", msh_pipeline_err2str(-3));
				return -3;
			}
			
			if(and_counter == 0) seq->sequence_pipelines[index]->background_pipe = 0;
			else seq->sequence_pipelines[index]->background_pipe = 1;
		}

		seq->sequence_pipelines[index]->pipeline_comm_count = 1;

		int command_counter = 0; //counter for number of commands
		char * token_copy = malloc(strlen(token) + 1);

		//malloc() failure
		if(token_copy == NULL)
		{
			free(str_copy);
			printf("MSH Error: %s\n", msh_pipeline_err2str(-5));
			return -5;
		}
		strcpy(token_copy, token);

		char *tok, *tok_ptr;
		for(tok = strtok_r(token_copy, "|", &tok_ptr); tok != NULL; tok = strtok_r(tok_ptr, "|", &tok_ptr))
		{
			//error checking related to commands
			int err = 0;
			int i;
			for(i = 0; i < (int) strlen(tok); i++)
			{
				if(tok[i] != ' ')
				{
					err = 1;
				}
			}

			if(err == 0)
			{
				seq->seq_pipeline_count++;
				free(str_copy);
				free(token_copy);
				printf("MSH Error: %s\n", msh_pipeline_err2str(-8));
				return -8;
			}

			if(seq->sequence_pipelines[index]->pipeline_comm_count > MSH_MAXCMNDS)
			{
				seq->seq_pipeline_count++;
				free(str_copy);
				free(token_copy);
				printf("MSH Error: %s\n", msh_pipeline_err2str(-7));
				return -7;
			}

			//allocate commands
			seq->sequence_pipelines[index]->pipeline_commands[command_counter] = calloc(1, sizeof(struct msh_command));

			//calloc() allocation failure
			if(seq->sequence_pipelines[index]->pipeline_commands[command_counter] == NULL)
			{
				free(str_copy);
				free(token_copy);
				printf("MSH Error: %s\n", msh_pipeline_err2str(-5));
				return -5;
			}

			seq->sequence_pipelines[index]->pipeline_commands[command_counter]->command_last = false;

			seq->sequence_pipelines[index]->pipeline_commands[command_counter]->comm_args_count = 1;

			int arguments_counter = 0; //counter for number of arguments
			char *tok_copy = malloc(strlen(tok) + 1);

			//malloc() failure
			if(tok_copy == NULL)
			{
				free(str_copy);
				free(token_copy);
				printf("MSH Error: %s\n", msh_pipeline_err2str(-5));
				return -5;
			}
			strcpy(tok_copy, tok);

			char* _token, *_ptr;
			for(_token = strtok_r(tok_copy, " ", &_ptr); _token != NULL; _token = strtok_r(_ptr, " ", &_ptr))
			{
				//error checking related to arguments
				if(seq->sequence_pipelines[index]->pipeline_commands[command_counter]->comm_args_count > (MSH_MAXARGS + 1))
				{
					seq->seq_pipeline_count++;
					free(str_copy);
					free(token_copy);
					free(tok_copy);
					printf("MSH Error: %s\n", msh_pipeline_err2str(-6));
					return -6;
				}

				//allocate arguments
				if(arguments_counter == 0)
				{
					seq->sequence_pipelines[index]->pipeline_commands[command_counter]->command = strdup(_token);
				}

				seq->sequence_pipelines[index]->pipeline_commands[command_counter]->comm_arguments[arguments_counter] = strdup(_token);

				seq->sequence_pipelines[index]->pipeline_commands[command_counter]->comm_args_count++;

				arguments_counter++;
			}
			seq->sequence_pipelines[index]->pipeline_comm_count++;

			//null terminate the comm_arguments array
			seq->sequence_pipelines[index]->pipeline_commands[command_counter]->comm_arguments[arguments_counter] = NULL;

			command_counter++;
			free(tok_copy);
		}
		//set boolean to true for final command
		seq->sequence_pipelines[index]->pipeline_commands[command_counter - 1]->command_last = true;

		index++;
		seq->seq_pipeline_count++;
		free(token_copy);
	}
	free(str_copy);
	return 0; //success
}

//dequeues the first pipeline in sequence
//return pointer to the nth command in the pipeline
struct msh_pipeline *
msh_sequence_pipeline(struct msh_sequence *s)
{
	//the pipeline is already freed, or was empty beforehand
	if(s->sequence_pipelines[s->cur] == NULL)
	{
		return NULL;
	}

	//allocate new space
	struct msh_pipeline *p = malloc(sizeof(struct msh_pipeline));

	//malloc() memory allocation failure
	if(p == NULL)
	{
		printf("msh_sequence_pipeline: malloc() failure\n");
		return NULL;
	}

	//copy sequence's pipeline to the newly allocated space
	memcpy(p, s->sequence_pipelines[s->cur], sizeof(struct msh_pipeline));

	free(s->sequence_pipelines[s->cur]); //dequeue pipeline from sequence

	s->sequence_pipelines[s->cur] = NULL; //set the pipeline to NULL

	s->cur++; //increment index

	return p;
}

//returns the specified command in a pipeline
struct msh_command *
msh_pipeline_command(struct msh_pipeline *p, size_t nth)
{
	return p->pipeline_commands[nth];
}

//returns 1 if the pipeline should be run in the background returns 0 if not
int
msh_pipeline_background(struct msh_pipeline *p)
{
	if(p->background_pipe == 1) return 1;
	return 0;
}

//returns 1 if the command is the final command or returns 0 if not
int
msh_command_final(struct msh_command *c)
{
	if (c->command_last == true) return 1;
	return 0;
}

void
msh_command_file_outputs(struct msh_command *c, char **stdout, char **stderr)
{
	(void)c;
	(void)stdout;
	(void)stderr;
}

//return the command's program
char *
msh_command_program(struct msh_command *c)
{
	//return first char* b/c program is written first then its arguments
	return c->command;
}

//return the null-terminated array of arguments for the given command
char **
msh_command_args(struct msh_command *c)
{
	return c->comm_arguments;
}

void
msh_command_putdata(struct msh_command *c, void *data, msh_free_data_fn_t fn)
{
	(void)c;
	(void)data;
	(void)fn;
}

void *
msh_command_getdata(struct msh_command *c)
{
	(void)c;

	return NULL;
}
