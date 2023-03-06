test: test.c
	gcc -o test test.c

run: test
	./test

.PHONY: clean

clean:
	rm -rf ./test
