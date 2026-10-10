
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 1000
#define MAX_ID 32
#define MAX_CATEGORY 64
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
    char id[MAX_ID];
    int arrival;
    int priority;
    int service;
    char category[MAX_CATEGORY];

    ProcessState state;
    int doctor;              /* -1 if not assigned */
} Process;

typedef struct {
    Process *items[MAX_PROCESSES];
    int count;
} ReadySet;

typedef struct {
    Process *patient;
} DoctorSlot;

/* ---------- 16.1: Process Creation and Arrival ---------- */

int load_processes(const char *filename,
                   Process processes[],
                   int *n)
{
    FILE *fp = fopen(filename, "r");

    if (fp == NULL) {
        perror("Could not open input file");
        return 0;
    }

    int count;
    int quantum;

    if (fscanf(fp, "%d", &count) != 1 ||
        fscanf(fp, "%d", &quantum) != 1 ||
        count < 1 || count > MAX_PROCESSES) {
        fprintf(stderr, "Invalid input header\n");
        fclose(fp);
        return 0;
    }

    /* Read each patient record. */
    for (int i = 0; i < count; i++) {
        char line[256];

        if (fgets(line, sizeof(line), fp) == NULL) {
            /* Skip leftover newline after the header. */
            i--;
            if (feof(fp)) {
                fprintf(stderr, "Missing patient records\n");
                fclose(fp);
                return 0;
            }
            continue;
        }

        if (line[0] == '\n' || line[0] == '\r')
        {
            i--;
            continue;
        }

        Process *p = &processes[i];

        if (sscanf(line, " %31[^,],%d,%d,%d,%63[^\r\n]",
                   p->id,
                   &p->arrival,
                   &p->priority,
                   &p->service,
                   p->category) != 5) {
            fprintf(stderr, "Invalid patient record: %s", line);
            fclose(fp);
            return 0;
        }

        if (p->arrival < 0 ||
            p->priority <= 0 ||
            p->service <= 0) {
            fprintf(stderr, "Invalid values for patient %s\n", p->id);
            fclose(fp);
            return 0;
        }

        /* IDs must be unique. */
        for (int j = 0; j < i; j++) {
            if (strcmp(processes[j].id, p->id) == 0) {
                fprintf(stderr, "Duplicate patient ID: %s\n", p->id);
                fclose(fp);
                return 0;
            }
        }

        p->state = NEW;
        p->doctor = -1;
    }

    fclose(fp);
    *n = count;
    return 1;
}

/* Move patients into READY when their arrival time is reached. */
void admit_arrivals(Process processes[],
                    int n,
                    int time,
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

/*
 * Fill vacant doctor slots using FCFS or non-preemptive Priority.
 * The caller invokes this whenever arrivals or completions
 * change scheduling eligibility.
 */
void schedule_non_preemptive(ReadySet *ready,
                             DoctorSlot doctors[DOCTORS],
                             Algorithm algorithm)
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

        p->state = RUNNING;
        p->doctor = free_doctor;
        doctors[free_doctor].patient = p;
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
 * Execution and remaining-service-time updates belong to
 * the execution module. This function only changes states
 * and assignments.
 */
void schedule_preemptive_priority(
    ReadySet *ready,
    DoctorSlot doctors[DOCTORS])
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
            waiting->state = RUNNING;
            waiting->doctor = free_doctor;
            doctors[free_doctor].patient = waiting;
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
        waiting->state = RUNNING;
        waiting->doctor = victim;
        doctors[victim].patient = waiting;
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
              Algorithm algorithm)
{
    if (algorithm == PRIORITY_PREEMPTIVE) {
        schedule_preemptive_priority(ready, doctors);
    } else {
        schedule_non_preemptive(ready, doctors, algorithm);
    }
}

/* ---------- Minimal demonstration ---------- */

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

    Process processes[MAX_PROCESSES];
    int n = 0;

    if (!load_processes(argv[1], processes, &n))
        return EXIT_FAILURE;

    ReadySet ready = {.count = 0};
    DoctorSlot doctors[DOCTORS] = {{NULL}, {NULL}};

    /*
     * This module deliberately stops at 16.1 and 16.2.
     * It creates processes, admits arrivals, and selects
     * which patients should run. It does not simulate
     * elapsed service time or calculate metrics.
     */
    int time = 0;
    int admitted = 0;

    while (admitted < n) {
        admit_arrivals(processes, n, time, &ready);

        for (int i = 0; i < n; i++) {
            if (processes[i].state != NEW)
                admitted++;
        }

        schedule(&ready, doctors, algorithm);

        /*
         * The execution module must advance time and clear
         * doctor slots on completion. This standalone driver
         * therefore exits after demonstrating the initial
         * scheduling decision.
         */
        break;
    }

    printf("Initial scheduling decision at time %d:\n", time);

    for (int d = 0; d < DOCTORS; d++) {
        if (doctors[d].patient != NULL) {
            printf("Doctor %d: Patient %s\n",
                   d + 1, doctors[d].patient->id);
        } else {
            printf("Doctor %d: Idle\n", d + 1);
        }
    }

    printf("Patients in READY set: %d\n", ready.count);

    return EXIT_SUCCESS;
}
