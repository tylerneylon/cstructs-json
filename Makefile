# temp makefile during development

tests = out/cjson_test
testenv = DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib MALLOC_LOG_FILE=/dev/null
cstructs_obj = out/CArray.o out/CMap.o out/CList.o

all: out/cjson.o out/cjson_test

test: out/cjson_test
	@echo Running tests:
	@echo -
	@for test in $(tests); do $(testenv) $$test || exit 1; done
	@echo -
	@echo All tests passed!

out/cjson_test: test/cjson_test.c $(cstructs_obj) out/ctest.o out/cjson_debug.o | out
	clang -o $@ $^ -I.

out/ctest.o: test/ctest.c test/ctest.h
	clang -o $@ -c $<

out/cjson.o: cjson.c cjson.h | out
	clang -c cjson.c -o out/cjson.o

out/cjson_debug.o: cjson.c cjson.h debug_hooks.c | out
	clang -c cjson.c -DDEBUG -o $@

out/C%.o: cstructs/C%.c cstructs/C%.h | out
	clang -o $@ -c $<

out:
	mkdir out

clean:
	rm -rf out/

.PHONY: test
