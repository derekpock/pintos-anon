To incorperate 70 iterations of alarms instead of just 7, the folder structure
in src/tests/threads were modified where each occurance of alarm-multiple was
replicated for the newly created alarm-mega function.

One file was added, src/tests/threads/alarm-mega.ck, which is identical to the
alarm-multiple.ck file, except the check_alarm function is called with a
parameter of 70 instead of 7.

5 other files were modified, each in the src/tests/threads folder. Modifications
made are as follows.
	- Make.tests
		Add alarm-mega to the test/threads_TESTS variable.
	- tests.c
		Add alarm-mega to the test struct, pointing "alarm-mega" to the
		test_alarm_mega function, defined in alarm-wait.c.
	- tests.h
		Add the test_func test_alarm_mega to the header file. Implemented in
		alarm-wait.c.
	- alarm-wait.c
		Created and implemented the test_alarm_mega function, which is the same
		as the test_alarm_multiple, except calls test_sleep with 5, 70 instead
		of 5, 7.
	- Rubric.alarm
		Adds alarm-mega to this Rubric list, which may not actually change much.
