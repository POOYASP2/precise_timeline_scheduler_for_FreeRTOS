ROOT_DIR := $(abspath ..)

# QEMU target from root Makefile
PARENT_TARGET := qemu

# Find all test_*.c recursively (exclude commons)
TEST_SOURCES := $(shell find . -type f -name 'test_*.c' -not -path './commons/*' | sed 's|^\./||' | sort)


.PHONY: tests list clean

tests:
	@set -e; \
	fail=0; \
	for test in $(TEST_SOURCES); do \
		echo "===================================="; \
		echo "Running $$test"; \
		$(MAKE) -s --no-print-directory -C $(ROOT_DIR) clean; \
		if $(MAKE) -s --no-print-directory -C $(ROOT_DIR) qemu USE_APP_MAIN=0 \
			EXTRA_SOURCES="testing/commons/test_common.c testing/$$test"; then \
			echo "PASS: $$test"; \
		else \
			echo "FAIL: $$test"; \
			fail=1; \
		fi; \
	done; \
	echo "===================================="; \
	if [ $$fail -eq 0 ]; then \
		echo "ALL TESTS PASSED"; \
	else \
		echo "SOME TESTS FAILED"; \
	fi; \
	exit $$fail


list:
	@echo "Discovered tests:"
	@for test in $(TEST_SOURCES); do echo "  $$test"; done

clean:
	$(MAKE) -C $(ROOT_DIR) clean
	find . -type f \( -name '*.o' -o -name '*.elf' -o -name '*.map' \) -delete





