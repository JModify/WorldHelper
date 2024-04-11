CC = gcc
CFLAGS = -pedantic -Wall -std=gnu99 -o $@ $<
LDFLAGS = -L/local/courses/csse2310/lib -lcsse2310a1
CPPFLAGS = -I/local/courses/csse2310/include

all: wordle-helper
wordle-helper: wordleHelper.c
	$(CC) $(LDFLAGS) $(CPPFLAGS) $(CFLAGS) 

clean:
	rm wordle-helper
