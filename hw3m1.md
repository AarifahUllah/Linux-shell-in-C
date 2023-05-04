Grade: 0/100
Extra Credit: 0
At commit: 1594a07

All possible grades:

+ at commit 1594a07: 0 + 0
+ at commit 4d1304d: 0 + 0
+ at commit 55f73ae: 0 + 0
+ at commit 86ee9b7: 0 + 0

-------------------------

Explanation:

* [error:lvl1] [fail]: FAILURE on tests/m1_00_single_cmd.txt

---

msh output for input "echo":

---

Expected output:



* [error:lvl2] [fail]: Valgrind on tests/m1_00_single_cmd.txt: FAILURE

* [error:lvl2] [fail]: FAILURE on tests/m1_01_simple_cmd.txt

---

msh output for input "echo hello":

---

Expected output:

hello

* [error:lvl3] [fail]: Valgrind on tests/m1_01_simple_cmd.txt: FAILURE

* [error:lvl3] [fail]: FAILURE on tests/m1_02_args.txt

---

msh output for input "echo hello world":

---

Expected output:

hello world

* [error:lvl4] [fail]: Valgrind on tests/m1_02_args.txt: FAILURE

* [error:lvl4] [fail]: FAILURE on tests/m1_03_pipeline.txt

---

msh output for input "ls tests/ | grep m0_ | grep pipeline":

---

Expected output:

m0_01_single_cmd_pipeline.c

m0_01_single_cmd_pipeline.d

m0_01_single_cmd_pipeline.test

m0_02_pipelines.c

m0_02_pipelines.d

m0_02_pipelines.test

* [error:lvl5] [fail]: Valgrind on tests/m1_03_pipeline.txt: FAILURE

* [error:lvl5] [fail]: FAILURE on tests/m1_04_seq.txt

---

msh output for input "ls -lh /usr/bin/ | grep thisisnotintheoutput ; ls tests/ | grep pipeline ; echo hello":

---

Expected output:

m0_01_single_cmd_pipeline.c

m0_01_single_cmd_pipeline.d

m0_01_single_cmd_pipeline.test

m0_02_pipelines.c

m0_02_pipelines.d

m0_02_pipelines.test

m1_03_pipeline.txt

hello

* [error:lvl6] [fail]: Valgrind on tests/m1_04_seq.txt: FAILURE

* [error:lvl6] [fail]: FAILURE on tests/m1_05_builtins.txt

---

msh output for input "cd tests/ ; ls -1 | grep m1_05 ; cd ~ ; ls -a1 | grep .bashrc":

---

Expected output:

m1_05_builtins.txt

.bashrc

* [error:lvl7] [fail]: Valgrind on tests/m1_05_builtins.txt: FAILURE

