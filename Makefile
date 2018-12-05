


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

# zip
.PHONY: zip
zip:
	@printf "";\
	rm -rf 01_xbenes49_xpolan09.zip
	@echo "Zipping files into 01_xbenes49_xpolan09.zip";\
	zip 01_xbenes49_xpolan09.zip src/*.h src/*.cpp src/Makefile Makefile documentation.pdf

# clean
.PHONY: clean
clean:
	@echo "Cleaning output.";\
	rm -rf $(output) $(output).tar.gz
	@printf "";\
	$(MAKE) clean -C src/ -s