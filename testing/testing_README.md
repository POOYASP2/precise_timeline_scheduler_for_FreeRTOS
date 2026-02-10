## Commons
Defines some common utilities for all tests.
TEST_PASS = 0 and TEST_FAIL = 1 are the values used for indicating the status of a test, inside the variable test_result_t.


Every test should define a 'run_test' function where they use TEST_ASSERT to verify conditions, if any condition is false then the test ends with a TEST_FAIL status. At the end of every test 'run_test' should return TEST_PASS, which will only be executed if none of the assertions failed.

Once the test is over, a special function is called automatically to exit qemu.

In the dummy_tests directory there are some very simple examples.

## Tests


## How to run
Inside the testing directory there is another Makefile that runs the root Makefile to start qemu and then runs each test separately using the main() function defined in 'test_common.c'.

Use 'make clean' to clean any compiled objects.
Use 'make list' to see the list of all tests.
Use 'make all' to run all tests.
Use 'make test TEST=/example/test_example.c to run a single test.
