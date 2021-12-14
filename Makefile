CC := gcc
CFLAGS := -Wall -Wextra -g -pedantic -std=c99

SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=%.o)

chip8: $(OBJS)
	$(CC) $^ -o $@

$(OBJS) : %.o: %.c
	$(CC) $(CFLAGS) -c $(OUTPUT_OPTION) $<

.PHONY: clean
clean: ; $(RM) $(OBJS) chip8
