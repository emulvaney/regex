CFLAGS = -g3 -Wall -Werror -pedantic

all: grep

clean:
	rm -f *.o

distclean: clean
	rm -f grep *~

grep: grep.o vm.o compiler.o parser.o
	$(CC) -o $@ $^