RELEASE_FLAGS = -Wall -Wextra -O3
DEBUG_FLAGS = -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-sign-conversion
NAME = registry
SOURCES = main.cpp connectionfactory.cpp packet.cpp
OBJECTS = $(SOURCES:.cpp=.o)

CXX = clang++
CFLAGS = $(RELEASE_FLAGS)
CXXFLAGS = $(RELEASE_FLAGS)

all: $(NAME)

debug: CFLAGS = $(DEBUG_FLAGS)
debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: $(NAME)

$(NAME): $(OBJECTS)
	$(CXX) $(CFLAGS) $(OBJECTS) -o $@


%.o: %.c
	$(CXX) -c $(CXXFLAGS) $< -o $@


clean:
	rm $(NAME) $(OBJECTS)
