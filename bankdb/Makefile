CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
DEBUG_CFLAGS = -Wall -Wextra -pthread -O1 -g -fsanitize=thread
INCLUDES = -Iinclude
TARGET = bankdb

SRCS = \
	src/main.c \
	src/core/bank.c \
	src/core/timer.c \
	src/core/transaction.c \
	src/system/parser.c \
	src/system/metrics.c \
	src/concurrency/lock_mgr.c \
	src/concurrency/buffer_pool.c \
	src/utils/utils.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

debug:
	$(CC) $(DEBUG_CFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET)-debug

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET)-debug

.PHONY: all clean debug
