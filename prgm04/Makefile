RELEASE_FLAGS = -Wall -Wextra -O3
DEBUG_FLAGS = -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-sign-conversion
NAME = registry
OBJECTS = main.o

CFLAGS = $(RELEASE_FLAGS)

all: main

debug: CFLAGS = $(DEBUG_FLAGS)
debug: main

main: $(OBJECTS)
	gcc $(CFLAGS) -o $(NAME) $(OBJECTS)

clean:
	rm $(NAME) $(OBJECTS)
