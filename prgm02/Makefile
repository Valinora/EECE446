RELEASE_FLAGS = -Wall -Wextra -O3
DEBUG_FLAGS = -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-sign-conversion
NAME = peer

FLAGS = $(RELEASE_FLAGS)
all: main

debug: FLAGS = $(DEBUG_FLAGS)
debug: main

main: main.o utilities.o
	gcc $(FLAGS) -o $(NAME) main.o utilities.o

main.o: main.c
	gcc $(FLAGS) -c main.c

utilities.o: utilities.c utilities.h
	gcc $(FLAGS) -c utilities.c

clean:
	rm $(NAME) *.o
