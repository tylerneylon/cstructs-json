# temp makefile during development

all: out/test1

out/test1: test1.c out/cjson.o out/CArray.o | out
	clang test1.c -o out/test1 out/cjson.o out/CArray.o

out/cjson.o: cjson.c cjson.h | out
	clang -c cjson.c -o out/cjson.o

out/CArray.o: cstructs/CArray.h cstructs/CArray.c | out
	clang -c cstructs/CArray.c -o out/CArray.o

out:
	mkdir out

clean:
	rm -rf out/
