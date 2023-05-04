Grade: 0/100
Extra Credit: 0
At commit: 663ae42

All possible grades:

+ at commit 663ae42: 0 + 0
+ at commit 48a74ba: 0 + 0
+ at commit 6ddc7ee: 0 + 0
+ at commit 1f79d8d: 0 + 0
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

* [error:lvl1] [fail]: segmentation fault or didn't finish for other reason

* [error:lvl2] [fail]: Valgrind on tests/m3_01_redirections.test: FAILURE

* [error:lvl2] [fail]: FAILURE: parse error: redirect to too many files -- hello cmd redir to multiple file error at tests/m3_02_redir_errors.c:15 when evaluating ret == MSH_ERR_REDIRECTED_TO_TOO_MANY_FILES

FAILURE: parse error: multiple redirections -- hello cmd redirection error at tests/m3_02_redir_errors.c:30 when evaluating ret == MSH_ERR_MULT_REDIRECTIONS

FAILURE: parse error: redirection to both pipe and file -- hello cmd redundant pipe error at tests/m3_02_redir_errors.c:45 when evaluating ret == MSH_ERR_REDUNDANT_PIPE_REDIRECTION

SUCCESS (0 of 3): Redirection error parsing.

* [error:lvl3] [fail]: Valgrind on tests/m3_02_redir_errors.test: FAILURE

* [error:lvl3] [fail]: FAILURE on tests/m3_03_simple.txt

---

msh output for input "echo hello 1>> out.txt ; cat out.txt ; rm out.txt":

hello 1>> out.txt

---

Expected output:

hello

* [error:lvl4] [fail]: Valgrind on tests/m3_03_simple.txt: FAILURE

* [error:lvl4] [fail]: FAILURE on tests/m3_04_append.txt

---

msh output for input "echo hello 1>> out.txt ; echo world 1>> out.txt ; cat out.txt ; rm out.txt":

hello 1>> out.txt

world 1>> out.txt

---

Expected output:

hello

world

* [error:lvl5] [fail]: Valgrind on tests/m3_04_append.txt: FAILURE

* [error:lvl5] [fail]: FAILURE on tests/m3_05_stderr_redir.txt

---

msh output for input "tests/print_stderr hello 2>> outerr.txt 1>> out.txt ; echo world 1>> out.txt ; cat outerr.txt ; cat out.txt ; rm out.txt ; rm outerr.txt":

world 1>> out.txt

world 1>> out.txt

---

Expected output:

hello

world

* [error:lvl6] [fail]: Valgrind on tests/m3_05_stderr_redir.txt: FAILURE

