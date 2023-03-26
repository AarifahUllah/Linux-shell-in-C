#include <msh_parse.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

//define msh's structs
struct msh_command{
	char* msh_command_arguments[MSH_MAXARGS + 2]; //plus 1 for the command/program itself plus 1 for NULL termination
	bool msh_command_last; //boolean flag for last command (msh_final_command())
};

//a pipeline is a set of commands
//a struct array of msh_commands
//of length MSH_MAXCMNDS
struct msh_pipeline{
	struct msh_command msh_pipeline_commands[MSH_MAXCMNDS];
};

//a sequence is a set of pipelines
//1 foreground pipeline and a maximum
//of MSH_MAXBACKGROUND amount of background pipelines
struct msh_sequence{
	struct msh_pipeline msh_sequence_pipelines[MSH_MAXBACKGROUND + 1];
};

//free the passed in pipeline
void
msh_pipeline_free(struct msh_pipeline *p)
{
	free(p);
}

void
msh_sequence_free(struct msh_sequence *s)
{
	//free internal structures for remaining pipelines
	//some pipelines were already freed by msh_pipeline_free
	
	free(s);
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

	return s;
}

char *
msh_pipeline_input(struct msh_pipeline *p)
{
	(void)p;

	return NULL;
}

msh_err_t
msh_sequence_parse(char *str, struct msh_sequence *seq)
{
	(void)str;
	(void)seq;

	return 0;
}

struct msh_pipeline *
msh_sequence_pipeline(struct msh_sequence *s)
{
	(void)s;

	return NULL;
}

struct msh_command *
msh_pipeline_command(struct msh_pipeline *p, size_t nth)
{
	(void)p;
	(void)nth;

	return NULL;
}

int
msh_pipeline_background(struct msh_pipeline *p)
{
	(void)p;

	return 0;
}

int
msh_command_final(struct msh_command *c)
{
	(void)c;

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
	(void)c;

	return NULL;
}

char **
msh_command_args(struct msh_command *c)
{
	(void)c;

	return NULL;
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
