# Assignment 1 demonstration and viva guide

Allow 10-15 minutes for the complete group demonstration and questions.
Every member should understand all components, even those written by a teammate.

## Demonstration sequence

Run these commands from the repository root:

```sh
make
./hospital samples/hospital.txt FCFS
./hospital samples/hospital.txt PRIORITY
./hospital samples/hospital.txt PRIORITY_PREEMPTIVE
make compare
./hospital samples/idle_and_ties.txt PRIORITY_PREEMPTIVE
make test
```

1. Explain the scenario: patients are processes and two doctors are identical
   parallel service slots. Other hospital resources are outside Assignment 1.
2. Show the five input fields and explain that lower numbers mean higher
   priority; category is descriptive.
3. Run all three policies. For the Hospital sample, compare P3's first start:
   time 9 under FCFS/PRIORITY versus time 2 under preemptive Priority.
4. Explain one preemption: at time 2 both doctors are busy with P1 and P2.
   P3 has priority 1, so P1, whose priority is 3, is interrupted. P1 has received
   two service units and returns to READY with eight remaining. It resumes at
   time 14 and completes at 22; its original first start stays 0.
5. Walk through P1's preemptive metrics: turnaround 22, response 0, waiting
   22 - 10 = 12. This shows why waiting and response differ after interruption.
6. Use the comparison tables to show that preemption improves average response
   on the Hospital sample but increases average waiting. In the urgent-burst
   sample it improves both. No policy is best for every metric on every input.
7. Show the tie/idle example: lexicographic order is P1, P10, P2; the next
   patient group arrives at time 20, so the clock jumps across the idle gap.
8. Show passing tests and describe the independent unit-time reference model.

## Code walkthrough

Read `CreatAndSchedule.c` in this order:

- `Process`, `ReadySet`, `DoctorSlot`: state and ownership representation.
- `load_processes`, `parse_integer`, `read_line`: validated input and allocation.
- `admit_arrivals`: NEW to READY transitions.
- `fcfs_better`, `priority_better`, `select_ready`: ready-patient ordering.
- `schedule_non_preemptive`, `select_preemption_victim`,
  `schedule_preemptive_priority`: dispatch and strict-priority interruption.
- `start_process`: first-start preservation and doctor assignment.
- `simulate`: next-event selection, elapsed execution, completion, and idle gaps.
- `print_results`: output sorting and metric definitions after execution ends.
- `free_processes` and simulation cleanup: memory ownership and error cleanup.

## Questions to prepare for

- **Why not sleep or launch real processes?** A1 requires deterministic simulated
  time. The C program models parallel doctors without real concurrency.
- **Why track remaining separately from service?** Original service is needed
  for waiting/utilization calculations; remaining changes as treatment proceeds.
- **Can equal priority preempt?** No. Only a strictly smaller priority number can
  interrupt a running patient.
- **How are victims chosen?** Largest priority number, later arrival, larger ID.
  A free doctor is always used first.
- **What happens at simultaneous arrival and completion?** Advance both doctors,
  terminate every completed patient, admit all arrivals, and then schedule.
- **Can a patient use both doctors?** No. One process runs on at most one doctor;
  two different patients can run at once.
- **How are waiting and response different?** Response ends at the first start;
  waiting includes all later periods spent READY after preemption.
- **Why is utilization sometimes below 100%?** Idle gaps and a final single-doctor
  tail reduce busy time relative to two doctors over the observation interval.
- **Why is output sorting separate?** Required ID ordering should not alter
  scheduling decisions. Sorting happens only after all patients finish.
- **What prevents starvation?** Nothing adds aging. Finite workloads terminate,
  but an unending high-priority stream could starve a low-priority patient.
- **Why these structures?** Arrays and scans are simple, deterministic, and
  sufficient for assignment-sized inputs; worst-case runtime is O(N^2).
- **What remains for later assignments?** Memory allocation, synchronization,
  race conditions, equipment/resource management, and deadlock handling.

Live code modification is not required by the specification. Be ready to
explain exact code paths and assumptions rather than only reading output.
