
cc = g++
flags = -std=c++17 -pedantic -Wall -Wextra -g -O0
defines = -DDEBUG_MODE
linkings = -lm -lsimlib

src = $(wildcard *.cpp)
head = $(wildcard *.h)
obj = $(src:.c=.o)

#output settings
output = model
all: $(output)

# linking
$(output) : $(obj)
	@echo "Linking project into $@.";\
	$(cc) $(flags) $(obj) -o $@ $(linkings)
  
# compiling
%.o : %.c
	@echo "Compiling $@.";\
	$(cc) $(flags) $(defines) -c $< -o $@

# run
.PHONY: run
run:
	@printf "";\
	./model

# clean
.PHONY: clean
clean:
	@echo "Cleaning generated files.";\
	rm -rf *~ *.o *.gch *.dep $(output) $(output).tar.gz *.out