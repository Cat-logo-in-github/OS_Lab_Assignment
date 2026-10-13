CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

.PHONY: all clean

all: hospital

hospital: CreatAndSchedule.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) hospital
