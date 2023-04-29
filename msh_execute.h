#pragma once
#include <unistd.h>

struct pid_struct;

//call after signal processes to reset to -1
void reset_pid();


//add particular pid into array
void add_pid(pid_t pid);

//send kill signal to every pid
void kill_pid();
