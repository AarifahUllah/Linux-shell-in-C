# Extra Credit Attempt

I tried doing additional job control by adding the "jobs" command and the "fg" command.

The fg command uses SIGCONT to continue the suspended or background commands into the foreground. These commands, like the sleep commands in the README.md can then be suspended again with cntl-z, or terminated with cntl-c. This follows the format specified in the extra credit portion given in the README.md. The fg command also checks a command is in the index given.

Sample commands include:

Input:
sleep 10 &; sleep 8 &; fg 1

Output:
[0] sleep 10
[1] sleep 8
^Z
1129753: Cntl-Z pressed. We've been asked to suspend. Go to background

Input:
sleep 10 &; sleep 8 &; fg 0

Output:
[0] sleep 10
[1] sleep 8
^C
1129858:Cntl-C pressed. Terminate foreground process

Input:
sleep 10 &; fg 1

Output:
[0] sleep 10
fg: 1: no such job

The "jobs" command works similarly and outputs an indexed array based on the oldest commands, starting from 0. It also attempts to print the programs name. However, if there are no jobs, the command returns nothing.

Input:
sleep 10 &; sleep 8 &; jobs

Output:
[0] sleep 10
[1] sleep 8
[0] @cUP+V
[1] 

Input: 
jobs

Output:
