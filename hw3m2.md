Grade: 0/100
Extra Credit: 0
At commit: 4834ac4

All possible grades:

+ at commit 4834ac4: 0 + 0
+ at commit c2b21a9: 0 + 0
+ at commit e1395ce: 0 + 0
+ at commit f2b39e6: 0 + 0
+ at commit 9d2ff3a: 0 + 0
+ at commit ef528f8: 0 + 0
+ at commit f940bed: 0 + 0
+ at commit 804217c: 0 + 0
+ at commit fbd832a: 0 + 0
+ at commit 37cdc74: 0 + 0
+ at commit 34ec4c7: 0 + 0
+ at commit 90e8677: 0 + 0
+ at commit 868e1da: 0 + 0
+ at commit 1594a07: 0 + 0
+ at commit 4d1304d: 0 + 0
+ at commit 55f73ae: 0 + 0
+ at commit 86ee9b7: 0 + 0

-------------------------

Explanation:

* [error:lvl1] [fail]: FAILURE: one command, background -- hello world pipeline is backgrounded at tests/m2_01_background.c:126 when evaluating msh_pipeline_background(p) == 1

FAILURE: two commands, background -- hello world pipeline is backgrounded at tests/m2_01_background.c:99 when evaluating msh_pipeline_background(p) == 1

FAILURE: two commands, one with arguments, background -- hello world pipeline is backgrounded at tests/m2_01_background.c:69 when evaluating msh_pipeline_background(p) == 1

FAILURE: two pipelines, two background -- hello2 pipeline is backgrounded at tests/m2_01_background.c:34 when evaluating msh_pipeline_background(p) == 1

SUCCESS (0 of 4): pipelines with background

* [error:lvl2] [fail]: Valgrind on tests/m2_01_background.test: FAILURE

* [error:lvl2] [fail]: FAILURE: Multiple background & specification error -- Two & in a pipeline at tests/m2_02_background_parse_errors.c:15 when evaluating ret == MSH_ERR_MISUSED_BACKGROUND

FAILURE: Background & before end of pipeline error -- background & specification before end of pipeline at tests/m2_02_background_parse_errors.c:30 when evaluating ret == MSH_ERR_MISUSED_BACKGROUND && s != ((void *)0)

SUCCESS (0 of 2): background specification errors

* [error:lvl3] [fail]: Valgrind on tests/m2_02_background_parse_errors.test: FAILURE

* [error:lvl3] [fail]: Expected:

[0] sleep 2



---

Actual:





FAILURE: control-c -- control-c at tests/m2_03_background_signal.c:9 when evaluating harness_exec("tests/signals_testing/cntlc.example") == 0

Expected:

[0] sleep 2



---

Actual:



FAILURE: control-z -- control-z at tests/m2_03_background_signal.c:17 when evaluating harness_exec("tests/signals_testing/cntlc.example") == 0

SUCCESS (0 of 2): Control-c and control-z signal tests

* [error:lvl4] [fail]: Valgrind on tests/m2_03_background_signal.test: FAILURE

* [error:lvl4] [fail]: FAILURE on tests/m2_03_single_bkground.txt

---

msh output for input "echo hello &":

hello &

---

Expected output:

hello

* [error:lvl5] [fail]: Valgrind on tests/m2_03_single_bkground.txt: FAILURE

