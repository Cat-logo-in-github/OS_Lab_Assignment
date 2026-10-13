CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
PYTHON = python3

.PHONY: all test test-ubsan test-sanitize compare clean

all: hospital

hospital: CreatAndSchedule.c
	$(CC) $(CFLAGS) $< -o $@

test: hospital
	$(PYTHON) tests/test_assignment1.py --binary ./hospital

hospital-ubsan: CreatAndSchedule.c
	$(CC) -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 \
		-fsanitize=undefined -fno-sanitize-recover=all $< -o $@

test-ubsan: hospital-ubsan
	$(PYTHON) tests/test_assignment1.py --binary ./hospital-ubsan

hospital-sanitized: CreatAndSchedule.c
	$(CC) -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 \
		-fsanitize=address,undefined -fno-omit-frame-pointer $< -o $@

test-sanitize: hospital-sanitized
	$(PYTHON) tests/test_assignment1.py --binary ./hospital-sanitized --failfast

compare: hospital
	$(PYTHON) tools/compare_algorithms.py samples/hospital.txt
	$(PYTHON) tools/compare_algorithms.py samples/urgent_burst.txt

clean:
	$(RM) hospital hospital-ubsan hospital-sanitized
