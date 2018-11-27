
cc = g++
flags = -std=c++17 -pedantic -Wall -Wextra -g -O0
defines = -DOUTPUT_LOG
linkings = -lm -lsimlib

src = $(wildcard *.cpp)
head = $(wildcard *.h)
obj = $(src:.cpp=.o)

#output settings
output = model
all: $(output)

# linking
$(output) : $(obj)
	@echo "Linking project into $@.";\
	$(cc) $(flags) $(defines) $(obj) -o $@ $(linkings)
  
# compiling
%.o : %.cpp
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
	@echo "Cleaning compilation files.";\
	rm -rf *~ *.o *.gch $(output) *.out
	