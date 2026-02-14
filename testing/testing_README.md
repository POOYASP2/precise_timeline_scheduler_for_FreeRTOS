## Commons
Defines some common utilities for all tests.
TEST_PASS = 0 and TEST_FAIL = 1 are the values used for indicating the status of a test, inside the variable test_result_t.

Every test should define a 'run_test' function where they use TEST_ASSERT to verify conditions, if any condition is false then the test ends with a TEST_FAIL status. At the end of every test 'run_test' should return TEST_PASS, which will only be executed if none of the assertions failed.

Once the test is over, a special function is called automatically to exit qemu.

In the dummy_tests directory there are some very simple examples.

## Tests
### Input Validity
These tests simply check the functions xPreprocessSchedule and xValidateSchedule used before starting the scheduler.

The preprocess test checks: \
-a task always start and ends within a subframe's boundaries; \
-the assignment of subframes and their relative timeslots; \
-the special case where a end time coincides with a subframe boundary; 

The validate test checks: \
-valid start and end times (end must not be before or same as the start); \
-a task always start and ends within a subframe's boundaries (same a preprocess); \
-tasks use a valid subframe id; \
-the overlapping rules for tasks (hrt tasks cannot overlap, srt can).

### Scheduler
These tests check the correctness of the scheduler's behavior. Since these tests are more complex and difficult to debug, they should be all be defined in a separate file.

Scheduler tests use a special assert, ASSERT_RTOS, for assertions and have a function, vTestPlatformBringUp(bool startLogger), to use logging inside the test. The logging function has to be inserted into the run_test function in order to work.

Since scheduler tests use qemu they need to exit. On the last task that should be run, use qemu_exit(TEST_PASS) to exit qemu and indicate a successful result (this is usually done in a vOrchestrator function).
#### 

## How to run
Inside the testing directory there is another Makefile that runs the root Makefile to start qemu and then can run each test separately using the main() function defined in 'test_common.c'.

Use 'make clean' to clean any compiled objects. \
Use 'make list' to see the list of all tests. \
Use 'make all' to run all tests. \
Use 'make test TEST=/example/test_example.c to run a single test.
