# cstrucst-json Makefile
#
# This library is meant to be used by including "json/json.h", and optionally
# "json/jsonutil.h" as well, followed by linking with their respective .o files.
#
# The primary make rules are:
#
# * all   -- Builds all object files and tests.
# * test  -- Builds everything and then runs the tests.
# * clean -- Removes the entire out directory.
#


#################################################################################
# Variables for targets.

tests = out/json_test
testenv = DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib MALLOC_LOG_FILE=/dev/null
cstructs_obj = out/array.o out/map.o out/list.o
ifeq ($(shell uname -s), Darwin)
	cflags = $(includes) -std=c99
else
	cflags = $(includes) -std=c99 -D _GNU_SOURCE
endif
lflags = -lm
cc = gcc $(cflags)


#################################################################################
# Primary rules; meant to be used directly.

all: out/json.o out/jsonutil.o out/json_test

test: out/json_test
	@echo Running tests:
	@echo -
	@for test in $(tests); do $(testenv) $$test || exit 1; done
	@echo -
	@echo All tests passed!

clean:
	rm -rf out/


#################################################################################
# Internal rules; meant to only be used indirectly by the above rules.

out/json_test: test/json_test.c $(cstructs_obj) out/ctest.o out/json_debug.o | out
	$(cc) -o $@ $^ -I. $(lflags)

out/ctest.o: test/ctest.c test/ctest.h
	$(cc) -o $@ -c $<

out/json.o: json/json.c json/json.h | out
	$(cc) -o $@ -c $<

out/jsonutil.o: json/jsonutil.c json/jsonutil.h | out
	$(cc) -o $@ -c $<

out/json_debug.o: json/json.c json/json.h json/debug_hooks.h | out
	$(cc) -c $< -DDEBUG -o $@

$(cstructs_obj) : out/%.o: cstructs/%.c cstructs/%.h | out
	$(cc) -o $@ -c $<

out:
	mkdir out

# The PHONY rule tells `make` to ignore directories with the same name as a rule.
.PHONY: test
