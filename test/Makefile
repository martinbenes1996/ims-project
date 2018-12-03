

flags = -Wall -Werror -pedantic -std=c++11
linkings = -lm -lpthread -lsimlib

all: test_inputlimiter

test_inputlimiter:
	g++ $(flags) test_inputlimiter.cpp -o $@ $(linkings)

.PHONY: clean
clean:
	rm -rf *.o test_inputlimiter > /dev/null 2> /dev/null