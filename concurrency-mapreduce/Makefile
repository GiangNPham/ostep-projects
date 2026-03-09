CC = gcc
CFLAGS = -Wall -Werror -g -pthread

SRCS = main.c mapreduce.c
OBJS = $(SRCS:.c=.o)
TARGET = mapreduce

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c mapreduce.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
