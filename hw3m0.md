Grade: 0/100
Extra Credit: 0
At commit: 308a21b1bd9252d987990a8ee84725466851b6e3

All possible grades:

+ at commit 308a21b1bd9252d987990a8ee84725466851b6e3: 0 + 0

-------------------------

Explanation:

* [error:lvl1] [fail]: FAILURE: simply parse a command -- hello pipeline found at tests/m0_01_single_cmd_pipeline.c:115 when evaluating p != ((void *)0)

FAILURE: single command, no arguments -- hello found pipeline at tests/m0_01_single_cmd_pipeline.c:87 when evaluating p != ((void *)0)

FAILURE: single command, one argument -- helloagain found pipeline at tests/m0_01_single_cmd_pipeline.c:55 when evaluating p != ((void *)0)

FAILURE: single command, multiple arguments -- hey found pipeline at tests/m0_01_single_cmd_pipeline.c:20 when evaluating p != ((void *)0)

SUCCESS (0 of 4): single command, various arguments

* [error:lvl2] [fail]: Valgrind on tests/m0_01_single_cmd_pipeline.test: FAILURE

* [error:lvl2] [fail]: FAILURE: two commands, no arguments -- hello pipeline at tests/m0_02_pipelines.c:59 when evaluating p != ((void *)0)

FAILURE: two commands, one with arguments -- hello pipeline at tests/m0_02_pipelines.c:19 when evaluating p != ((void *)0)

SUCCESS (0 of 2): pipeline of commands, various arguments

* [error:lvl3] [fail]: Valgrind on tests/m0_02_pipelines.test: FAILURE

* [error:lvl3] [fail]: FAILURE: simple sequences -- hello pipeline at tests/m0_03_sequences.c:20 when evaluating p != ((void *)0)

FAILURE: sequence of pipelines -- hello pipeline at tests/m0_03_sequences.c:58 when evaluating p != ((void *)0)

SUCCESS (0 of 2): sequences

* [error:lvl4] [fail]: Valgrind on tests/m0_03_sequences.test: FAILURE

* [error:lvl4] [fail]: FAILURE: pipeline with no command before or after | -- error for no command at tests/m0_04_errors.c:15 when evaluating ret == MSH_ERR_PIPE_MISSING_CMD

FAILURE: too many commands -- error for too many commands at tests/m0_04_errors.c:30 when evaluating ret == MSH_ERR_TOO_MANY_CMDS

FAILURE: too many args -- error for too many args at tests/m0_04_errors.c:45 when evaluating ret == MSH_ERR_TOO_MANY_ARGS

SUCCESS (0 of 3): Testing edge cases and errors

* [error:lvl5] [fail]: Valgrind on tests/m0_04_errors.test: FAILURE

