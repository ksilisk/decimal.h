OS := $(shell uname -s)
CC = gcc
FLAGS = -Wall -Werror -Wextra
FILES = helpers.c convertors.c arithmetic.c other.c comparison.c
FILES_O = helpers.o convertors.o arithmetic.o other.o comparison.o
FLAGS_PLATFORM = $(shell pkg-config --cflags --libs check)
GCOVFLAGS=-fprofile-arcs -ftest-coverage

all: clean s21_decimal.a

s21_decimal.a:
	$(CC) $(FLAGS) -c $(FILES)
	ar rcs s21_decimal.a $(FILES_O)

test: s21_decimal.a
	gcc tests.c s21_decimal.a $(FLAGS_PLATFORM) -o tests.o
	./tests.o

gcov_report:
	$(CC) $(CFLAGS) $(FLAGS_PLATFORM) $(GCOVFLAGS) *.c -o gcov_main
	./gcov_main
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory gcov_report

clean:
	-rm -rf *.o *.html *.gcda *.gcno *.css *.a *.gcov *.info *.out *.cfg *.txt
