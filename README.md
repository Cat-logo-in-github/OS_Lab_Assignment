# Emergency Hospital Management System

The **Emergency Hospital Management System** is an Operating Systems project that simulates the management of patients, medical staff, hospital beds, and shared medical resources in a small emergency hospital.

The system demonstrates how Operating Systems concepts can be applied to real-world situations involving process scheduling, limited-resource allocation, concurrency, synchronization, and deadlock prevention.

Patients arrive with different medical conditions, priorities, and treatment requirements. The system coordinates their treatment while ensuring that limited hospital resources are allocated efficiently and safely.

## Objectives

- Manage patients according to their urgency and treatment requirements.
- Implement process scheduling algorithms to determine treatment order.
- Allocate and release hospital beds and medical resources efficiently.
- Coordinate multiple doctors and nurses.
- Prevent race conditions when accessing shared medical equipment.
- Identify and prevent potential deadlocks.
- Minimize patient waiting time and maximize resource utilization.
- Demonstrate the practical application of Operating Systems principles.

## Hospital Resources

The hospital operates with the following limited resources:

| Resource | Available Quantity |
| --- | --- |
| Emergency Beds | 6 |
| Doctors | 2 |
| Nurses | 3 |
| Treatment Rooms | 2 |
| X-ray Machines | 1 |
| MRI Machines | 1 |
| Medicines | Limited |
| Blood Bags | Limited |

Patients may require multiple resources during treatment. If a required resource is unavailable, the patient must wait until it becomes available.

## Core Features (To be reviewed and updated)

### 1\. Patient Management

Each patient is represented as a process in the Operating Systems model.

Patient information may include:

- Patient ID and name
- Medical condition
- Arrival time
- Priority level
- Estimated treatment time
- Required medical resources
- Current treatment status

The patient lifecycle follows this sequence:

`Arrival → Waiting → Doctor Consultation → Treatment → Recovery → Discharge`

### 2\. Process Scheduling

The system can implement different scheduling algorithms to decide which patient should receive treatment next.

- **First Come, First Served (FCFS):** Patients are treated in arrival order.
- **Priority Scheduling:** Patients with more urgent conditions receive higher priority.
- **Shortest Job First (SJF):** Patients with shorter estimated treatment times are selected first.
- **Preemptive Priority Scheduling (optional):** A critical patient may interrupt a lower-priority process when the simulation permits it.

The system can compare scheduling algorithms using average waiting time, turnaround time, and resource utilization.

### 3\. Resource Allocation

The hospital has a fixed number of beds, staff members, treatment rooms, and diagnostic machines.

The system manages resource allocation and deallocation as patients enter, receive treatment, and leave the hospital.

Allocation strategies such as First Fit, Best Fit, and Worst Fit can be explored in a simulated resource-space model.

When a patient is discharged, their occupied resources are released for other patients.

### 4\. Concurrency and Synchronization

Multiple patients may request the same medical resource at the same time. For example, several patients may require the X-ray machine, but only one can use it at once.

The system models:

- Mutual exclusion
- Critical sections
- Locks and synchronization
- Race-condition prevention
- Safe access to shared resources

Synchronization ensures that two processes cannot simultaneously claim or modify the same exclusive resource.

### 5\. Deadlock Prevention

A deadlock can occur when processes hold resources while waiting indefinitely for resources held by one another.

For example:

- Patient process P1 holds a doctor resource and waits for the X-ray machine.
- Patient process P2 holds the X-ray machine and waits for the doctor.

The system can explore strategies such as consistent resource-acquisition ordering, requesting resources together where feasible, and deadlock detection.

### 6\. Patient Monitoring and Resource Status

The simulation can track:

- Patients waiting for treatment
- Patients currently being treated
- Available and occupied beds
- Available doctors and nurses
- Medical equipment in use
- Remaining medicine and blood supplies
- Completed treatments and discharged patients

## Operating Systems Concepts Covered

| Concept | Application |
| --- | --- |
| Process Management | Each patient is modeled as a process. |
| CPU Scheduling | Scheduling algorithms determine the next patient to be treated. |
| Waiting Time | Time spent waiting before treatment begins. |
| Turnaround Time | Total time from arrival to completion. |
| Resource Allocation | Beds, staff, rooms, and equipment are allocated to patients. |
| Synchronization | Coordinates access to shared medical resources. |
| Mutual Exclusion | Prevents simultaneous use of exclusive equipment. |
| Race Conditions | Demonstrates errors caused by unsynchronized access. |
| Deadlock | Models circular waiting for resources. |
| Starvation | Examines whether low-priority patients wait indefinitely. |

## Performance Metrics

The simulation can evaluate its effectiveness using the following metrics:

- **Average Waiting Time:** Average time patients spend waiting for treatment.
- **Turnaround Time:** Time between patient arrival and treatment completion or discharge, according to the chosen model.
- **Throughput:** Number of patients whose treatment is completed per unit of simulated time.
- **Resource Utilization:** Percentage of time a resource is occupied.
- **Starvation Analysis:** Identifies patients who experience excessive delays due to scheduling policies.

## System Workflow

1. A patient arrives at the hospital.
2. The system records the patient's details and urgency.
3. The patient enters the appropriate waiting queue.
4. The scheduling algorithm selects an eligible patient.
5. Required beds, staff, rooms, and equipment are checked.
6. Available resources are allocated safely.
7. Treatment is performed in the simulation.
8. Resources are released when no longer needed.
9. The patient's status is updated, and discharge is recorded.
10. Performance statistics are calculated.

## Technologies Used

Update this section according to the technologies actually used in the implementation.

- **Programming Language:** C
- **Version Control:** Git
- **Repository Hosting:** GitHub
- **Core Subject:** Operating Systems

## Getting Started

### Prerequisites

Install the programming language runtime and any dependencies required by the project.

### Installation

Clone the repository:

```
git clone <repository-url>
```

Navigate to the project directory:

```
cd <project-directory>
```

Follow the build and execution instructions for the language used in the implementation.

## Testing Scenarios

The system can be tested using scenarios such as:

1. A critical patient arrives while a lower-priority patient is waiting.
2. All six emergency beds are occupied.
3. Multiple patients request the X-ray machine simultaneously.
4. Medicine or blood supplies become insufficient.
5. Two processes request resources in conflicting orders.
6. A low-priority patient waits while higher-priority patients continue arriving.
7. A patient completes treatment and releases their allocated resources.

These tests help verify scheduling decisions, resource safety, synchronization, and deadlock handling.

## Academic Purpose

This project is developed for educational purposes as part of an Operating Systems course. It demonstrates how theoretical concepts can be translated into a practical simulation of a resource-constrained emergency hospital.

The system is intended for academic demonstration and is not designed for real-world medical diagnosis, clinical decision-making, or hospital operations.

## Team Members

- yesy01 (guys, just replace this if you are ok with adding real names online)
- Sud-oat 
- Parshwa Doshi (cat-in-github-logo)

**Ownership:** This project is jointly developed and equally owned by all team members.
