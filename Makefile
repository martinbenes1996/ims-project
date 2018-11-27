


#output settings
output = model
all: $(output)

$(output): 
	@printf "";\
	$(MAKE) -C src/ -s
	@printf "";\
	cp src/$@ .


# run
.PHONY: run
run:
	@printf "";\
	./model

# clean
.PHONY: clean
clean:
	@echo "Cleaning output.";\
	rm -rf $(output) $(output).tar.gz
	@printf "";\
	$(MAKE) clean -C src/ -s