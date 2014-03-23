# temp makefile during development

all: out/test1

out/test1: test1.c out/cjson.o out/CArray.o out/CMap.o out/CList.o | out
	clang -o out/test1 $^

out/cjson.o: cjson.c cjson.h | out
	clang -c cjson.c -o out/cjson.o

out/C%.o: cstructs/C%.c cstructs/C%.h | out
	clang -o $@ -c $<

out:
	mkdir out

clean:
	rm -rf out/
