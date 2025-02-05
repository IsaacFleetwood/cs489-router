CC = gcc

SRCS := $(wildcard src/*.c)
BINS := $(SRCS:%.c=%.o)

all: main

%: %.o
  ${CC} $< -o $@

%.o: %.c
  ${CC} -c $< -o $@

clean:
  rm -r ./src/*.o
  rm main

main: ${BINS}
  gcc src/*.o -o main
