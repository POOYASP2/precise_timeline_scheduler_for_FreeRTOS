## Commons
Defines some common utilities for all tests.
TEST_PASS = 0 and TEST_FAIL = 1 are the values used for indicating the status of a test, inside the variable test_result_t.

Every test should define a 'run_test' function where they use ASSERT to verify conditions, if any condition is false then the test ends with a TEST_FAIL status. At the end of every test 'run_test' should return TEST_PASS, which will only be executed if none of the assertions failed.

Once the test is over, a special function is called automatically to exit qemu (in scheduler tests, this is handled differently).

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

Scheduler tests use a special assert, ASSERT_RTOS, for assertions and have a function, vTestPlatformBringUp(bool startLogger, timeline_schedule, numTasks), to use logging inside the test. The logging function has to be inserted into the run_test function in order to work.

Since scheduler tests use qemu they need to exit. On the last task that should be run, use qemu_exit(TEST_PASS) to exit qemu and indicate a successful result.

NOTE: the function that starts the scheduler performs validation and preprocessing automatically. Doing preprocessing twice leads to errors, so if you need to do check the result of preprocessing do it on a copy of the timeline schedule.

#### Major Frame Loop
Checks that the reset function correctly resets tasks for the looping behavior of the major frame.

#### Polling
Tests that mechanisms for Producer_Consumer work correctly.

#### Preemption
Checks for the mechanisms of preemption for simple

#### Task Updating
Checks that the updating function (assumed to run every tick) udpates tasks correctly according to if they met their deadlines.

### Stress Test
These tests are just simulations for extreme use cases, such as strict hrt deadlines with preemption (simulation A) or forced deadlines misses (simulation B).
Simulation C is not a stress test but is made to be easily editable for manual testing of the scheduler and as a baseline for other simulations.

### Advanced Tests
These tests target specific edge cases, timing precision, and robustness requirements of the Time-Triggered architecture.

#### SRT Chaining
Verifies that multiple Soft Real-Time (SRT) tasks execute sequentially (in a chain) within the idle time of a single sub-frame. It ensures that after one SRT task finishes, the scheduler correctly identifies and launches the next pending SRT task.

#### Timing Jitter & Drift
Checks the precision of the global clock, specifically during empty sub-frames. It ensures the scheduler does not "fast-forward" through empty slots but waits for the exact sub-frame duration to elapse, preventing timing drift.

#### HRT Overrun (Robustness)
Simulates a critical failure where a Hard Real-Time (HRT) task enters an infinite while(1) loop and refuses to yield the CPU. Verifies that the scheduler successfully interrupts, terminates (or suspends), and marks the task as DEADLINE_MISSED exactly when the time slot expires.


## How to run
Inside the testing directory there is another Makefile that runs the root Makefile to start qemu and then can run each test separately using the main() function defined in 'test_common.c'.

Use 'make clean' to clean any compiled objects. \
Use 'make list' to see the list of all tests. \
Use 'make all' to run all tests. \
Use 'make test TEST=/example/test_example.c' to run a single test.
Use 'make debug TEST=/example/test_example.c' to run a single test in qemu-debug mode (gdb must be opened and connected on another terminal).
