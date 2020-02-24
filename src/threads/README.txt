README.txt
GLENN NESS, CIS520, Project0

As this was the first project in CIS520, not many changes were necessary to implement the new test log-mega.txt.
A single file was created in the src/tests/threads folder called "alarm-mega". 
In the file the program simply generated 70 alarms using tests::threads::alarm and was written using perl. 
The below existing files were changed in the src/tests/threads folder
In tests.c, {"alarm-mega", test_alarm_mega} was added
In tests.h "extern test_fun test_alarm_mega;" was added
In Rubric.alarm "4	alarm-mega" was added
In alarm-wait.c a new function called test_alarm_mega was added 
as a simple analog to the already existing test_alarm_multiple
