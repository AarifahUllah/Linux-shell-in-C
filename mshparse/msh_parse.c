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
	bool background_pipe;
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
	unsigned int i;
	for (i = 0; i < MSH_MAXCMNDS; i++)
	{
		if(p->pipeline_commands[i] != NULL)
		{			
			free(p->pipeline_commands[i]->command); //free the command pointer
			unsigned int j;
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
	free(p->pipe); //free pointer to pipe

	free(p); //free overall pipeline struct
	p = NULL; //set p to NULL after the free() for msh_sequence_free 
}

void
msh_sequence_free(struct msh_sequence *s)
{
	//free internal structures for remaining pipelines
	//some pipelines were already freed by msh_pipeline_free
	unsigned int i;
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


char *
msh_pipeline_input(struct msh_pipeline *p)
{
	return p->pipe;
}

msh_err_t
msh_sequence_parse(char *str, struct msh_sequence *seq)
{
	(void)seq;
	//break by pipelines- ';'
	char *token, *ptr; 
	for(token = strtok_r(str, ";", &ptr); token != NULL; token = strtok_r(NULL, ";", &ptr))
	{
		
		//insert each token into sequence
		int index = 0; //keep track of index in struct sequence, the array of pipelines
		seq->sequence_pipelines[index] = calloc(1, sizeof(struct msh_pipeline));
		seq->sequence_pipelines[index]->pipe = strdup(token); //put token into sequence

		//check whether to run the pipeline in the background
		if(token[strlen(token) - 2] == '&')
		{
			seq->sequence_pipelines[index]->background_pipe = true;
		}
		else
		{
			seq->sequence_pipelines[index]->background_pipe = false;
		}

			
	}
	//no space found in sequence
	//return with error
	
	return 0;
}

struct msh_pipeline *
msh_sequence_pipeline(struct msh_sequence *s)
{
	//the sequence is empty, there are no more pipelines
	(void)s;

	return NULL;
}

//returns the specified command in a pipeline
struct msh_command *
msh_pipeline_command(struct msh_pipeline *p, size_t nth)
{
	//struct msh_command * comm = p->pipeline_commands[nth];
	return p->pipeline_commands[nth];
}

int
msh_pipeline_background(struct msh_pipeline *p)
{
	if(p->background_pipe == true)
	{
		return 1;
	}

	return 0;
}

int
msh_command_final(struct msh_command *c)
{
	//the last command in pipeline
	if (c->command_last == true){
		return 1;
	}

	//c is not the last command
	return 0;
}

void
msh_command_file_outputs(struct msh_command *c, char **stdout, char **stderr)
{
	(void)c;
	(void)stdout;
	(void)stderr;
}

char *
msh_command_program(struct msh_command *c)
{
	//return first char* b/c program is written first then its arguments
	char * program;
	for (program = strtok(c->comm_arguments, " "); program != NULL; program = strtok(NULL, " "))
	{
		return program;
		break;
	}
	//else command is NULL (has no programs or arguments)
	return NULL;
}

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
