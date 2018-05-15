CC ?= gcc

main.out: main.c preprocessor_output.h
	$(CC) -g $< -o $@

preprocessor_output.h: preprocessor.out main.c
	./preprocessor.out main.c > $@

preprocessor.out: preprocessor.c
	$(CC) -g $< -o $@

.PHONY: all clean
clean:
	rm -f *.out preprocessor_output.h
