CC = gcc

SRCS := $(wildcard src/*.c)
BINS := $(SRCS:%.c=%.o)

all: main

%: %.o
	${CC} -g $< -o $@

%.o: %.c
	${CC} -g -c $< -o $@

clean:
	rm -r ./src/*.o
	rm main

main: ${BINS}
	gcc src/*.o -o main
