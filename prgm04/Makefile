RELEASE_FLAGS = -Wall -Wextra -O3
DEBUG_FLAGS = -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-sign-conversion -fsanitize=address -fsanitize=undefined
NAME = registry

CXX = g++
CFLAGS = $(RELEASE_FLAGS)
CXXFLAGS = $(RELEASE_FLAGS)

all: main

debug: CFLAGS = $(DEBUG_FLAGS)
debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: main

main: main.cpp
	$(CXX) $(CXXFLAGS) -o $(NAME) main.cpp

clean:
	rm $(NAME) $(OBJECTS)
