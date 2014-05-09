# temp makefile during development

tests = out/json_test
testenv = DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib MALLOC_LOG_FILE=/dev/null
cstructs_obj = out/CArray.o out/CMap.o out/CList.o
ifeq ($(shell uname -s), Darwin)
	cflags = $(includes) -std=c99
else
	cflags = $(includes) -std=c99 -D _GNU_SOURCE
endif
lflags = -lm
cc = gcc $(cflags)


all: out/json.o out/jsonutil.o out/json_test

test: out/json_test
	@echo Running tests:
	@echo -
	@for test in $(tests); do $(testenv) $$test || exit 1; done
	@echo -
	@echo All tests passed!

out/json_test: test/json_test.c $(cstructs_obj) out/ctest.o out/json_debug.o | out
	$(cc) -o $@ $^ -I. $(lflags)

out/ctest.o: test/ctest.c test/ctest.h
	$(cc) -o $@ -c $<

out/json.o: json.c json.h | out
	$(cc) -o $@ -c $<

out/jsonutil.o: jsonutil.c jsonutil.h | out
	$(cc) -o $@ -c $<

out/json_debug.o: json.c json.h debug_hooks.h | out
	$(cc) -c json.c -DDEBUG -o $@

out/C%.o: cstructs/C%.c cstructs/C%.h | out
	$(cc) -o $@ -c $<

out:
	mkdir out

clean:
	rm -rf out/

.PHONY: test
