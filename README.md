# Emergency Hospital Management - Assignment 1

CS-3710 Assignment 1: a deterministic process-management simulation in C.
Patients are processes, and two identical doctors are parallel execution slots.
The program implements FCFS, non-preemptive Priority, and preemptive Priority.

## Team

- Yesirat (GitHub: Yesy01)
- Parshwa Doshi
- Sudyotman Sudershan

The project is jointly developed and equally owned by all team members.

## Assignment scope

Assignment 1 covers patient arrival, scheduling, execution, completion,
preemption, scheduling metrics, and comparison of all three required policies.
Beds, nurses, medicines, equipment allocation, memory management,
synchronization, and deadlock handling belong to later assignments.

## Build and run

Requirements: a C11 compiler, POSIX interfaces (`getline` and `strdup`), and Make.
Linux and macOS are supported. Python 3 is needed only for tests and comparisons;
all Python tooling uses the standard library.

Run these commands from the repository root:

```sh
make
./hospital samples/hospital.txt FCFS
./hospital samples/hospital.txt PRIORITY
./hospital samples/hospital.txt PRIORITY_PREEMPTIVE
```

Without Make:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -O2 CreatAndSchedule.c -o hospital
```

The interface is `./hospital INPUT_FILE ALGORITHM`. The first argument is the
input file path. The second is exactly `FCFS`, `PRIORITY`, or
`PRIORITY_PREEMPTIVE`. An invalid invocation or input produces a diagnostic on
stderr, a nonzero exit status, and no result rows on stdout.

## Input

The first line contains the patient count N. The second contains a positive
quantum Q, which is validated but unused for Hospital scheduling. The remaining
N records have five comma-separated fields:

`ID,ARRIVAL,PRIORITY,SERVICE,CATEGORY`

`samples/hospital.txt` is the Hospital example from the assignment specification:

```text
5
2
P1,0,3,10,FEVER
P2,1,2,8,FRACTURE
P3,2,1,12,CRITICAL
P4,3,3,5,MINOR
P5,4,1,7,SEVERE
```

- N, Q, priority, and service must be positive integers; arrival can also be zero.
- Each integer must fit in `int`: at most `INT_MAX`, which is 2147483647 on the
  tested platform. Simulation timestamps use signed 64-bit integers.
- IDs must be nonempty and unique. Categories must be nonempty and contain no
  commas. IDs and categories cannot contain embedded carriage returns or NULs.
- Surrounding whitespace is trimmed. Blank lines between records and after the
  final record are ignored. CRLF files and a missing final newline are accepted.
- Missing records, extra nonblank records, extra fields, numeric suffixes, and
  overflowing integers are rejected. Quoted CSV fields are not supported.
- Patient storage, ready-set storage, IDs, and categories are allocated
  dynamically; there is no fixed 1000-patient or string-length limit.
- Input records can be in any order. Scheduling uses the numeric priority, not
  the descriptive category.

## Process model and execution flow

Each `Process` stores ID, arrival, priority, total service, category, state,
assigned doctor, remaining service, first start, and completion time.

```text
NEW -> READY -> RUNNING -> TERMINATED
                 |
                 +------> READY (preemption)
```

1. `load_processes` validates input and initializes each patient as NEW, with
   remaining service equal to the original service and start/completion unset.
2. `simulate` admits all arrivals at the current timestamp into a `ReadySet`
   containing pointers to the patient records.
3. `schedule` fills vacant doctor slots or preempts eligible running patients.
   `start_process` records only the first start; resuming does not reset it.
4. The clock jumps to the earliest future arrival or running-patient completion.
   Both running doctors advance by the same elapsed amount.
5. Patients whose remaining time reaches zero become TERMINATED, record their
   completion, and release their doctor. All completions at a timestamp are
   processed before the next scheduling decision at that timestamp.
6. The loop continues until every patient has completed. Only then does
   `print_results` sort records by ID and produce results and metrics.

### Decisions and assumptions

- Simulated time is integer time; there are no real-time sleeps, actual patient
  threads, or operating-system child processes.
- A patient uses one doctor at a time. Doctors are identical; a resumed patient
  may use a different doctor. The first vacant slot is assigned deterministically.
- All simultaneous arrivals and completions are considered before scheduling.
  There is no artificial ordering delay between events at the same timestamp.
- Switching doctors and preemption have zero modeled overhead.
- Idle periods jump directly to the next arrival. Service already received is
  never lost when a patient is preempted.
- Comparisons use lexicographic string order, so `P10` comes before `P2`.
- The measurement interval begins at the first arrival and ends at the last
  completion. It includes idle gaps within that interval, but excludes time
  before the first arrival.
- Arrays and scans keep the implementation readable: duplicate detection and
  the simulation take O(N^2) time in the worst case; storage is O(N), plus strings.

## Scheduling algorithms

| CLI value | Selection order | Preemption |
| --- | --- | --- |
| FCFS | Earlier arrival, then smaller lexicographic ID | None |
| PRIORITY | Smaller numeric priority, earlier arrival, smaller ID | None |
| PRIORITY_PREEMPTIVE | Same ready-patient ordering as PRIORITY | Only for strictly higher priority |

FCFS and PRIORITY allow a patient to finish once treatment starts. Preemptive
Priority reconsiders scheduling on every arrival and completion. It always uses
a free doctor before interrupting a running patient.

If both doctors are busy, the best ready patient can replace a running patient
with a strictly larger numeric priority. The victim is chosen by largest
numeric priority, then later arrival, then larger lexicographic ID. Equal
priority never triggers preemption. Interrupted patients return to READY with
remaining service and first start preserved. Both doctors can be preempted at
the same timestamp if two sufficiently urgent patients arrive.

## Metrics and output

For each patient, let A be arrival, S first start, C final completion, and B the
original total service:

| Metric | Definition |
| --- | --- |
| Completion | C |
| Turnaround | C - A |
| Response | S - A |
| Waiting | C - A - B, including every wait after preemption |
| Average waiting / turnaround / response | Arithmetic mean across patients |
| Throughput | N / (last completion - first arrival) |
| Doctor utilization (%) | 100 * sum(B) / (2 * elapsed time) |

With zero switching overhead, total doctor busy time equals the sum of all
original service times, including service received before preemption. Positive
service and N >= 1 guarantee a positive observation interval.

Normal stdout is machine-readable CSV, with no explanatory prose or column
header. Each completed patient has one row in lexicographic ID order, with:

`PROCESS_ID,ARRIVAL,PRIORITY,SERVICE,START,COMPLETION,WAITING,TURNAROUND,RESPONSE`

For `./hospital samples/hospital.txt FCFS`, the complete output is:

```text
P1,0,3,10,0,10,0,10,0
P2,1,2,8,1,9,0,8,0
P3,2,1,12,9,21,7,19,7
P4,3,3,5,10,15,7,12,7
P5,4,1,7,15,22,11,18,11
SUMMARY
AVERAGE_WAITING,5.000000
AVERAGE_TURNAROUND,13.400000
AVERAGE_RESPONSE,5.000000
THROUGHPUT,0.227272727272727
UTILIZATION,95.454545
```

Summary averages and utilization use six decimal places. Throughput uses fifteen
decimal places to preserve very small values without scientific notation.

## Actual algorithm comparison

Reproduce both tables with:

```sh
make compare
```

The comparison script runs the compiled simulator for each policy. Average
completion is the mean of absolute completion timestamps; last completion is
the final finishing time. Times are simulation units, throughput is patients
per time unit, and utilization is a percentage. Tables round presentation to
three decimals, except throughput, which uses six.

### Specification Hospital sample

| Algorithm | Avg waiting | Avg turnaround | Avg response | Avg completion | Last completion | Throughput | Utilization (%) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| FCFS | 5.000 | 13.400 | 5.000 | 15.400 | 22 | 0.227273 | 95.455 |
| PRIORITY | 5.400 | 13.800 | 5.400 | 15.800 | 22 | 0.227273 | 95.455 |
| PRIORITY_PREEMPTIVE | 6.400 | 14.800 | 2.600 | 16.800 | 22 | 0.227273 | 95.455 |

P3 first starts at time 9 under either non-preemptive policy, but at time 2 under
preemptive Priority. P5 starts at 15 under FCFS, 10 under PRIORITY, and 4 under
PRIORITY_PREEMPTIVE. Urgent patients respond sooner with preemption, while
interrupted P1 and P2 wait longer. Consequently, average response improves but
average waiting and turnaround increase. All three finish at time 22 and have
the same throughput and utilization on this input.

### Urgent burst behind long treatments

`samples/urgent_burst.txt` begins with two long, low-priority treatments, followed
by another long patient and three short, urgent patients.

| Algorithm | Avg waiting | Avg turnaround | Avg response | Avg completion | Last completion | Throughput | Utilization (%) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| FCFS | 12.167 | 22.667 | 12.167 | 24.333 | 40 | 0.150000 | 78.750 |
| PRIORITY | 12.000 | 22.500 | 12.000 | 24.167 | 41 | 0.146341 | 76.829 |
| PRIORITY_PREEMPTIVE | 3.667 | 14.167 | 3.167 | 15.833 | 40 | 0.150000 | 78.750 |

Preemptive Priority immediately serves P4, P5, and P6 at their arrival times
instead of making them wait for a long treatment to finish. Here it reduces
average waiting and response. Non-preemptive Priority delays the remaining
long treatment until time 21, so its last completion is 41 instead of 40. A
policy that improves average waiting can still reduce throughput and
utilization by leaving a longer final tail on one doctor.

### Fairness and starvation

FCFS avoids overtaking among waiting patients, but long treatments can delay
urgent arrivals. Both priority policies favor urgency and can repeatedly delay
low-priority patients. Preemptive Priority can additionally interrupt a patient
multiple times, making its waiting time larger than its response time.

Every patient eventually completes for the supported finite input files. In a
hypothetical unending stream of higher-priority arrivals, priority policies can
starve low-priority patients. Aging could mitigate that, but is optional and is
not implemented because it would change the required scheduling semantics.

## Tests and validation

```sh
make test
make test-ubsan
```

The 21-test suite covers the specified sample for all policies, delayed first
arrival, idle gaps, simultaneous arrivals/completions, lexicographic ties,
equal priorities, each preemption-victim tie-break, repeated and simultaneous
preemption, free doctors, numeric priority versus category, determinism,
input-order independence, malformed input, CLI errors, long strings, more than
1000 patients, and completion times exceeding 32-bit integers.

It also checks 120 reproducible randomized workloads under all three policies
against an independent Python reference that advances one time unit at a time.
The C implementation instead jumps between events. Output schema and summary
metrics are checked on every successful case. Expected full output for the
Hospital sample is stored in `samples/expected/`.

Both `make test` and `make test-ubsan` passed all 21 tests during development.
Clang static analysis also completed without findings. Optional combined
AddressSanitizer/UndefinedBehaviorSanitizer checking is available with:

```sh
make test-sanitize
```

AddressSanitizer execution remains unverified on the development host: its
runtime stalled even for an empty C `main`, including when the simulator was
run outside the workspace sandbox. The UBSan-only runtime completed normally.

## Limitations and optional features

- Input integers are bounded by `INT_MAX`; allocations are limited by available
  memory. The array-based implementation is intended for assignment-sized data.
- Native Windows builds need a POSIX environment such as WSL.
- No GUI, Gantt chart, aging, context-switch overhead, or additional scheduling
  policies are implemented. These are optional for A1.
- The comparison helper and regression suite supplement the required simulator;
  neither is needed to run the compiled C program.
- This is an educational scheduling model, not a clinical hospital system.

## Submission and demonstration

The specification lists the deadline as October 16, end of day. Submit the
complete source, Makefile, README, and supporting documentation/samples/tests.
Build executables locally; generated binaries and debug symbols are ignored.

See [A1 completion checklist](docs/A1_CHECKLIST.md) and
[demonstration and viva guide](docs/DEMO_VIVA.md). All three members should
understand the complete implementation and attend the 10-15 minute session.

To remove generated build artifacts:

```sh
make clean
```
