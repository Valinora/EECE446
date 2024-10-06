FLAGS = -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-sign-conversion
NAME = peer

main: main.o utilities.o
	gcc $(FLAGS) -o $(NAME) main.o utilities.o

main.o: main.c
	gcc $(FLAGS) -c main.c

utilities.o: utilities.c utilities.h
	gcc $(FLAGS) -c utilities.c

clean:
	rm $(NAME) *.o
