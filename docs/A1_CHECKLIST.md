# Assignment 1 completion checklist

Selected scenario: Emergency Hospital. Required policies: FCFS, non-preemptive
Priority, and preemptive Priority. Service units: two identical doctors.

Completed in implementation order:

1. [x] **Execution and preemption (Sections 16.1-16.3):** admit arrivals, advance
   by event times, update both doctors, preserve remaining service on
   interruption, record first start/completion, release doctors, and finish all
   patients. Simultaneous events and idle gaps are handled deterministically.
2. [x] **Metrics and standardized output (Sections 10, 14, 15, 16.4):** required
   CLI algorithm names, sorted patient rows, completion/waiting/turnaround/
   response, summary averages, throughput, and two-doctor utilization.
3. [x] **Input validation (Section 12):** positive N/Q/priority/service,
   nonnegative arrivals, unique IDs, exactly five fields per record, and exactly
   N nonblank records. Numeric overflow and binary NUL data are rejected;
   storage is dynamic rather than silently truncated.
4. [x] **Verification (Sections 17, 19):** 21 regression tests, 120 randomized
   workloads checked across all policies against an independent reference,
   expected sample output, and passing UBSan checks. Optional AddressSanitizer
   execution remains unverified because its runtime stalls on this host.
5. [x] **Documentation and measured comparison (Sections 11, 16.5, 20):** team
   names, build/run instructions, process model, policies/tie-breaks, design
   assumptions, metrics, two actual comparison tables, fairness, limitations,
   and optional-feature status.
6. [x] **Demonstration preparation (Section 21):** runnable examples and an
   explanation/viva guide in `DEMO_VIVA.md`.

No required simulator feature remains unimplemented. The following are human
submission activities, not additional code features:

- [ ] Submit the complete source and documentation through the course's
  designated submission channel by October 16, end of day.
- [ ] Rehearse the demonstration together and review the full codebase.
- [ ] Attend the scheduled demonstration-cum-viva with every group member.

Memory allocation strategies, locks, synchronization, medical-equipment
coordination, and deadlocks belong to later assignments. Round Robin and SJF
are not required for the Hospital scenario.
