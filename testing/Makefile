ROOT_DIR := $(abspath ..)

# QEMU target from root Makefile
PARENT_TARGET := qemu

# Find all test_*.c recursively (exclude commons)
TEST_SOURCES := $(shell find . -type f -name 'test_*.c' -not -path './commons/*' | sed 's|^\./||' | sort)


.PHONY: all test list clean

all:
	@set -e; \
	fail=0; \
	results=""; \
	for test in $(TEST_SOURCES); do \
		echo "===================================="; \
		echo "Running $$test"; \
		$(MAKE) -s --no-print-directory -C $(ROOT_DIR) clean; \
		if $(MAKE) -s --no-print-directory -C $(ROOT_DIR) qemu USE_APP_MAIN=0 \
			EXTRA_SOURCES="testing/commons/test_common.c testing/$$test"; then \
			echo "PASS: $$test"; \
			results="$$results\nPASS  $$test"; \
		else \
			echo "FAIL: $$test"; \
			results="$$results\nFAIL  $$test"; \
			fail=1; \
		fi; \
	done; \
	echo "===================================="; \
	echo "Test summary:"; \
	echo "------------------------------------"; \
	printf "$$results\n"; \
	echo "------------------------------------"; \
	if [ $$fail -eq 0 ]; then \
		echo "ALL TESTS PASSED"; \
	else \
		echo "$$fail TESTS FAILED"; \
	fi; \
	exit 0

test:
	@set -e; \
	if [ -z "$(TEST)" ]; then \
		echo "ERROR: Please specify a test with TEST=<test_file>"; \
		exit 1; \
	fi; \
	echo "===================================="; \
	echo "Running $(TEST)"; \
	$(MAKE) -s --no-print-directory -C $(ROOT_DIR) clean; \
	if $(MAKE) -s --no-print-directory -C $(ROOT_DIR) qemu USE_APP_MAIN=0 \
		EXTRA_SOURCES="testing/commons/test_common.c testing/$(TEST)"; then \
		echo "PASS: $(TEST)"; \
	else \
		echo "FAIL: $(TEST)"; \
	fi; \
	echo "===================================="
	exit 0

list:
	@echo "Discovered tests:"
	@for test in $(TEST_SOURCES); do echo "  $$test"; done

clean:
	$(MAKE) -C $(ROOT_DIR) clean
	find . -type f \( -name '*.o' -o -name '*.elf' -o -name '*.map' \) -delete





