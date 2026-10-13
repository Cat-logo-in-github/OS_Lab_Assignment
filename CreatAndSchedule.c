#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DOCTORS 2

typedef enum {
    NEW,
    READY,
    RUNNING,
    TERMINATED
} ProcessState;

typedef enum {
    FCFS,
    PRIORITY,
    PRIORITY_PREEMPTIVE
} Algorithm;

typedef struct {
    char *id;
    int arrival;
    int priority;
    int service;
    char *category;

    ProcessState state;
    int doctor;              /* -1 if not assigned */
    int64_t remaining;
    int64_t start;           /* First start; -1 until scheduled. */
    int64_t completion;
} Process;

typedef struct {
    Process **items;
    int count;
} ReadySet;

typedef struct {
    Process *patient;
} DoctorSlot;

/* ---------- 16.1: Process Creation and Arrival ---------- */

char *trim(char *text)
{
    while (isspace((unsigned char)*text))
        text++;
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]))
        *--end = '\0';
    return text;
}

int parse_integer(const char *text, int minimum, int *value)
{
    char *end;
    errno = 0;
    long parsed = strtol(text, &end, 10);
    if (text == end || *end != '\0' || errno == ERANGE ||
        parsed < minimum || parsed > INT_MAX)
        return 0;
    *value = (int)parsed;
    return 1;
}

/* Return 1 for a line, 0 for EOF, and -1 for a read error or binary data. */
int read_line(FILE *fp, char **line, size_t *capacity)
{
    ssize_t length = getline(line, capacity, fp);
    if (length < 0) {
        if (ferror(fp)) {
            fprintf(stderr, "Could not read input file\n");
            return -1;
        }
        return 0;
    }
    if (memchr(*line, '\0', (size_t)length) != NULL) {
        fprintf(stderr, "Input contains a NUL byte\n");
        return -1;
    }
    return 1;
}

void free_processes(Process processes[], int n)
{
    for (int i = 0; i < n; i++) {
        free(processes[i].id);
        free(processes[i].category);
    }
    free(processes);
}

int load_processes(const char *filename, Process **processes, int *n)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Could not open input file");
        return 0;
    }

    char *line = NULL;
    size_t capacity = 0;
    int count;
    int quantum;
    int created = 0;
    Process *loaded = NULL;

    if (read_line(fp, &line, &capacity) != 1 ||
        !parse_integer(trim(line), 1, &count) ||
        read_line(fp, &line, &capacity) != 1 ||
        !parse_integer(trim(line), 1, &quantum)) {
        fprintf(stderr, "N and Q must be positive integers no greater than %d\n",
                INT_MAX);
        goto failure;
    }

    loaded = calloc((size_t)count, sizeof(*loaded));
    if (loaded == NULL) {
        fprintf(stderr, "Out of memory while creating patients\n");
        goto failure;
    }

    while (created < count) {
        if (read_line(fp, &line, &capacity) != 1) {
            fprintf(stderr, "Missing patient records\n");
            goto failure;
        }

        char *record = trim(line);
        if (*record == '\0')
            continue;

        char *fields[5];
        char *cursor = record;
        for (int field = 0; field < 4; field++) {
            char *comma = strchr(cursor, ',');
            if (comma == NULL) {
                fprintf(stderr, "Patient record %d must have five fields\n",
                        created + 1);
                goto failure;
            }
            *comma = '\0';
            fields[field] = trim(cursor);
            cursor = comma + 1;
        }
        fields[4] = trim(cursor);

        if (*fields[0] == '\0' || *fields[4] == '\0' ||
            strchr(fields[4], ',') != NULL ||
            strchr(fields[0], '\r') != NULL || strchr(fields[4], '\r') != NULL) {
            fprintf(stderr, "Invalid patient ID or category in record %d\n",
                    created + 1);
            goto failure;
        }

        Process *p = &loaded[created];
        if (!parse_integer(fields[1], 0, &p->arrival) ||
            !parse_integer(fields[2], 1, &p->priority) ||
            !parse_integer(fields[3], 1, &p->service)) {
            fprintf(stderr, "Invalid numeric values in patient record %d\n",
                    created + 1);
            goto failure;
        }

        for (int j = 0; j < created; j++) {
            if (strcmp(loaded[j].id, fields[0]) == 0) {
                fprintf(stderr, "Duplicate patient ID: %s\n", fields[0]);
                goto failure;
            }
        }

        p->id = strdup(fields[0]);
        p->category = strdup(fields[4]);
        created++;
        if (p->id == NULL || p->category == NULL) {
            fprintf(stderr, "Out of memory while reading patients\n");
            goto failure;
        }
        p->state = NEW;
        p->doctor = -1;
        p->remaining = p->service;
        p->start = -1;
        p->completion = -1;
    }

    /* Extra nonblank records usually indicate an incorrect N. */
    int status;
    while ((status = read_line(fp, &line, &capacity)) == 1) {
        if (*trim(line) != '\0') {
            fprintf(stderr, "Unexpected data after %d patient records\n", count);
            goto failure;
        }
    }
    if (status == -1)
        goto failure;

    free(line);
    fclose(fp);
    *processes = loaded;
    *n = count;
    return 1;

failure:
    free(line);
    free_processes(loaded, created);
    fclose(fp);
    return 0;
}

/* Move patients into READY when their arrival time is reached. */
void admit_arrivals(Process processes[],
                    int n,
                    int64_t time,
                    ReadySet *ready)
{
    for (int i = 0; i < n; i++) {
        if (processes[i].state == NEW &&
            processes[i].arrival <= time) {

            processes[i].state = READY;
            ready->items[ready->count++] = &processes[i];
        }
    }
}

/* ---------- Scheduling helpers ---------- */

int compare_id(const Process *a, const Process *b)
{
    return strcmp(a->id, b->id);
}

/* FCFS ordering: arrival time, then lexicographic ID. */
int fcfs_better(const Process *a, const Process *b)
{
    if (a->arrival != b->arrival)
        return a->arrival < b->arrival;

    return compare_id(a, b) < 0;
}

/* Priority ordering: priority, arrival time, then ID. */
int priority_better(const Process *a, const Process *b)
{
    if (a->priority != b->priority)
        return a->priority < b->priority;

    if (a->arrival != b->arrival)
        return a->arrival < b->arrival;

    return compare_id(a, b) < 0;
}

/* Remove a patient from READY and return the pointer. */
Process *ready_remove(ReadySet *ready, int index)
{
    if (index < 0 || index >= ready->count)
        return NULL;

    Process *p = ready->items[index];

    for (int i = index; i < ready->count - 1; i++)
        ready->items[i] = ready->items[i + 1];

    ready->count--;
    return p;
}

/* Select the best READY patient according to the policy. */
int select_ready(const ReadySet *ready, Algorithm algorithm)
{
    if (ready->count == 0)
        return -1;

    int best = 0;

    for (int i = 1; i < ready->count; i++) {
        int better;

        if (algorithm == FCFS)
            better = fcfs_better(ready->items[i],
                                 ready->items[best]);
        else
            better = priority_better(ready->items[i],
                                     ready->items[best]);

        if (better)
            best = i;
    }

    return best;
}

/* ---------- 16.2: Scheduling ---------- */

void start_process(Process *p,
                   DoctorSlot doctors[DOCTORS],
                   int doctor,
                   int64_t time)
{
    if (p->start == -1)
        p->start = time;

    p->state = RUNNING;
    p->doctor = doctor;
    doctors[doctor].patient = p;
}

/*
 * Fill vacant doctor slots using FCFS or non-preemptive Priority.
 * The caller invokes this whenever arrivals or completions
 * change scheduling eligibility.
 */
void schedule_non_preemptive(ReadySet *ready,
                             DoctorSlot doctors[DOCTORS],
                             Algorithm algorithm,
                             int64_t time)
{
    while (ready->count > 0) {
        int free_doctor = -1;

        for (int d = 0; d < DOCTORS; d++) {
            if (doctors[d].patient == NULL) {
                free_doctor = d;
                break;
            }
        }

        if (free_doctor == -1)
            return;

        int index = select_ready(ready, algorithm);
        Process *p = ready_remove(ready, index);

        start_process(p, doctors, free_doctor, time);
    }
}

/*
 * Choose the running patient to preempt:
 * 1. Lowest priority (largest numerical priority)
 * 2. Later arrival
 * 3. Lexicographically larger ID
 */
int select_preemption_victim(DoctorSlot doctors[DOCTORS])
{
    int victim = -1;

    for (int d = 0; d < DOCTORS; d++) {
        Process *p = doctors[d].patient;

        if (p == NULL)
            continue;

        if (victim == -1) {
            victim = d;
            continue;
        }

        Process *v = doctors[victim].patient;

        if (p->priority > v->priority ||
            (p->priority == v->priority &&
             p->arrival > v->arrival) ||
            (p->priority == v->priority &&
             p->arrival == v->arrival &&
             compare_id(p, v) > 0)) {
            victim = d;
        }
    }

    return victim;
}

/*
 * Preempt only when the best waiting patient has strictly
 * higher priority than a running patient.
 *
 * The event loop accounts for elapsed service before scheduling,
 * so returning a patient to READY preserves its remaining time.
 */
void schedule_preemptive_priority(
    ReadySet *ready,
    DoctorSlot doctors[DOCTORS],
    int64_t time)
{
    while (ready->count > 0) {
        int best = select_ready(ready, PRIORITY);
        Process *waiting = ready->items[best];

        int free_doctor = -1;

        for (int d = 0; d < DOCTORS; d++) {
            if (doctors[d].patient == NULL) {
                free_doctor = d;
                break;
            }
        }

        /* Always use a vacant doctor before preempting. */
        if (free_doctor != -1) {
            waiting = ready_remove(ready, best);
            start_process(waiting, doctors, free_doctor, time);
            continue;
        }

        int victim = select_preemption_victim(doctors);

        if (victim == -1)
            return;

        Process *running = doctors[victim].patient;

        /* Equal priority never causes preemption. */
        if (waiting->priority >= running->priority)
            return;

        /* Return interrupted patient to READY. */
        running->state = READY;
        running->doctor = -1;
        ready->items[ready->count++] = running;

        /* Assign the freed doctor to the higher-priority patient. */
        waiting = ready_remove(ready, best);
        start_process(waiting, doctors, victim, time);
    }
}

/*
 * Scheduling entry point.
 *
 * Call after admitting arrivals and whenever a scheduling
 * decision is needed. Completion handling is external:
 * when a patient finishes, mark it TERMINATED and set the
 * corresponding doctors[d].patient to NULL before calling
 * this function again.
 */
void schedule(ReadySet *ready,
              DoctorSlot doctors[DOCTORS],
              Algorithm algorithm,
              int64_t time)
{
    if (algorithm == PRIORITY_PREEMPTIVE) {
        schedule_preemptive_priority(ready, doctors, time);
    } else {
        schedule_non_preemptive(ready, doctors, algorithm, time);
    }
}

/* ---------- 16.3: Discrete-event execution ---------- */

int simulate(Process processes[], int n, Algorithm algorithm)
{
    ReadySet ready = {.items = calloc((size_t)n, sizeof(*ready.items)), .count = 0};
    if (ready.items == NULL) {
        fprintf(stderr, "Out of memory while creating the ready set\n");
        return 0;
    }
    DoctorSlot doctors[DOCTORS] = {{NULL}, {NULL}};
    int64_t time = 0;
    int completed = 0;

    while (completed < n) {
        /* Completions at this timestamp were handled on the last step.
         * Admit every simultaneous arrival before selecting new patients. */
        admit_arrivals(processes, n, time, &ready);
        schedule(&ready, doctors, algorithm, time);

        int64_t next = INT64_MAX;

        for (int i = 0; i < n; i++) {
            if (processes[i].state == NEW && processes[i].arrival < next)
                next = processes[i].arrival;
        }

        for (int d = 0; d < DOCTORS; d++) {
            Process *p = doctors[d].patient;
            if (p != NULL && time + p->remaining < next)
                next = time + p->remaining;
        }

        /* With positive service times, each step reaches a later event.
         * Idle intervals jump straight to the next arrival. */
        int64_t elapsed = next - time;
        for (int d = 0; d < DOCTORS; d++) {
            Process *p = doctors[d].patient;
            if (p == NULL)
                continue;

            p->remaining -= elapsed;
            if (p->remaining == 0) {
                p->state = TERMINATED;
                p->completion = next;
                p->doctor = -1;
                doctors[d].patient = NULL;
                completed++;
            }
        }
        time = next;
    }
    free(ready.items);
    return 1;
}

/* ---------- 16.4: Metrics and standardized output ---------- */

int result_id_compare(const void *left, const void *right)
{
    return compare_id(left, right);
}

void print_results(Process processes[], int n)
{
    double total_waiting = 0;
    double total_turnaround = 0;
    double total_response = 0;
    int64_t busy = 0;
    int64_t first_arrival = INT64_MAX;
    int64_t last_completion = 0;

    /* Sorting only after execution keeps presentation independent of scheduling. */
    qsort(processes, (size_t)n, sizeof(processes[0]), result_id_compare);

    for (int i = 0; i < n; i++) {
        const Process *p = &processes[i];
        int64_t turnaround = p->completion - p->arrival;
        int64_t waiting = turnaround - p->service;
        int64_t response = p->start - p->arrival;

        printf("%s,%d,%d,%d,%" PRId64 ",%" PRId64 ",%" PRId64
               ",%" PRId64 ",%" PRId64 "\n",
               p->id, p->arrival, p->priority, p->service, p->start,
               p->completion, waiting, turnaround, response);

        total_waiting += waiting;
        total_turnaround += turnaround;
        total_response += response;
        busy += p->service;
        if (p->arrival < first_arrival)
            first_arrival = p->arrival;
        if (p->completion > last_completion)
            last_completion = p->completion;
    }

    /* The observation interval starts at the first arrival, includes idle gaps,
     * and ends when the final patient completes. Positive service makes it > 0. */
    double elapsed = (double)(last_completion - first_arrival);
    printf("SUMMARY\n");
    printf("AVERAGE_WAITING,%.6f\n", total_waiting / n);
    printf("AVERAGE_TURNAROUND,%.6f\n", total_turnaround / n);
    printf("AVERAGE_RESPONSE,%.6f\n", total_response / n);
    printf("THROUGHPUT,%.15f\n", n / elapsed);
    printf("UTILIZATION,%.6f\n", 100.0 * busy / (DOCTORS * elapsed));
}

/* ---------- Command-line entry point ---------- */

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s INPUT_FILE FCFS|PRIORITY|PRIORITY_PREEMPTIVE\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    Algorithm algorithm;

    if (strcmp(argv[2], "FCFS") == 0)
        algorithm = FCFS;
    else if (strcmp(argv[2], "PRIORITY") == 0)
        algorithm = PRIORITY;
    else if (strcmp(argv[2], "PRIORITY_PREEMPTIVE") == 0)
        algorithm = PRIORITY_PREEMPTIVE;
    else {
        fprintf(stderr, "Unknown scheduling algorithm\n");
        return EXIT_FAILURE;
    }

    Process *processes = NULL;
    int n = 0;

    if (!load_processes(argv[1], &processes, &n))
        return EXIT_FAILURE;

    if (!simulate(processes, n, algorithm)) {
        free_processes(processes, n);
        return EXIT_FAILURE;
    }
    print_results(processes, n);
    free_processes(processes, n);

    return EXIT_SUCCESS;
}
