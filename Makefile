# temp makefile during development

tests = out/json_test
testenv = DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib MALLOC_LOG_FILE=/dev/null
cstructs_obj = out/CArray.o out/CMap.o out/CList.o

all: out/json.o out/json_test

test: out/json_test
	@echo Running tests:
	@echo -
	@for test in $(tests); do $(testenv) $$test || exit 1; done
	@echo -
	@echo All tests passed!

out/json_test: test/json_test.c $(cstructs_obj) out/ctest.o out/json_debug.o | out
	clang -o $@ $^ -I.

out/ctest.o: test/ctest.c test/ctest.h
	clang -o $@ -c $<

out/json.o: json.c json.h | out
	clang -c json.c -o out/json.o

out/json_debug.o: json.c json.h debug_hooks.c | out
	clang -c json.c -DDEBUG -o $@

out/C%.o: cstructs/C%.c cstructs/C%.h | out
	clang -o $@ -c $<

out:
	mkdir out

clean:
	rm -rf out/

.PHONY: test
