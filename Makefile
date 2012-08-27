CFLAGS = -g3 -Wall -Werror -pedantic

all: grep

clean:
	rm -f *.o

distclean: clean
	rm -f grep *~ *.gcov *.gcda *.gcno

check: grep
	awk -f check.awk check.tests

grep: grep.o vm.o compiler.o parser.o debug.o
	$(CC) -o $@ $^
