CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
DEBUG_CFLAGS = -Wall -Wextra -pthread -O1 -g -fsanitize=thread
INCLUDES = -Iinclude
TARGET = bankdb

SRCS := $(shell find src -name '*.c' | sort)
OBJS = $(SRCS:.c=.o)
TEST_TRACES = \
	tests/trace_simple.txt \
	tests/trace_readers.txt \
	tests/trace_deadlock.txt \
	tests/trace_abort.txt \
	tests/trace_buffer.txt \
	tests/trace_buffer_blocking.txt \
	tests/trace_stress_buffer.txt \
	tests/trace_stress_mix.txt

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

debug:
	$(CC) $(DEBUG_CFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET)-debug

test: $(TARGET)
	@for trace in $(TEST_TRACES); do \
		echo "Running $$trace"; \
		./$(TARGET) --accounts=tests/accounts.txt --trace=$$trace --deadlock=prevention --tick-ms=10; \
	done

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET)-debug

.PHONY: all clean debug test
