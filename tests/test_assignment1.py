"""CLI regressions and an independent, unit-time scheduling reference model."""

import argparse
import csv
import math
from pathlib import Path
import random
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
ALGORITHMS = ("FCFS", "PRIORITY", "PRIORITY_PREEMPTIVE")
SUMMARY_KEYS = ("AVERAGE_WAITING", "AVERAGE_TURNAROUND", "AVERAGE_RESPONSE",
                "THROUGHPUT", "UTILIZATION")
BINARY = ROOT / "hospital"


def input_text(patients):
    return f"{len(patients)}\n2\n" + "".join(
        f"{pid},{arrival},{priority},{service},{category}\n"
        for pid, arrival, priority, service, category in patients
    )


def invoke(data, algorithm="FCFS"):
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "patients.txt"
        path.write_text(data)
        return subprocess.run([str(BINARY), str(path), algorithm],
                              capture_output=True, text=True, timeout=10)


def reference(patients, algorithm):
    """Advance one time unit at a time; unlike the C event-based clock."""
    jobs = {p[0]: p for p in patients}
    remaining = {p[0]: p[3] for p in patients}
    starts, completions = {}, {}
    ready, active = set(), set()
    time = min(p[1] for p in patients)

    def priority_key(pid):
        return jobs[pid][2], jobs[pid][1], pid

    def ready_key(pid):
        return (jobs[pid][1], pid) if algorithm == "FCFS" else priority_key(pid)

    while len(completions) < len(patients):
        finished = {pid for pid in active if remaining[pid] == 0}
        for pid in finished:
            completions[pid] = time
        active -= finished
        ready.update(p[0] for p in patients if p[1] == time)

        while ready:
            candidate = min(ready, key=ready_key)
            if len(active) == 2:
                if algorithm != "PRIORITY_PREEMPTIVE":
                    break
                victim = max(active, key=priority_key)
                if jobs[candidate][2] >= jobs[victim][2]:
                    break
                active.remove(victim)
                ready.add(victim)
            ready.remove(candidate)
            active.add(candidate)
            starts.setdefault(candidate, time)

        if len(completions) == len(patients):
            break
        for pid in active:
            remaining[pid] -= 1
        time += 1

    results = {}
    for pid, arrival, priority, service, _ in patients:
        turnaround = completions[pid] - arrival
        results[pid] = (arrival, priority, service, starts[pid], completions[pid],
                        turnaround - service, turnaround, starts[pid] - arrival)
    return results


class Assignment1Tests(unittest.TestCase):
    def results(self, patients, algorithm, data=None):
        result = invoke(input_text(patients) if data is None else data, algorithm)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        rows = list(csv.reader(result.stdout.splitlines()))
        self.assertEqual(len(rows), len(patients) + 6)
        self.assertEqual(rows[len(patients)], ["SUMMARY"])
        self.assertEqual([r[0] for r in rows[:len(patients)]],
                         sorted(p[0] for p in patients))
        actual = {}
        for row in rows[:len(patients)]:
            self.assertEqual(len(row), 9)
            actual[row[0]] = tuple(int(value) for value in row[1:])

        summary_rows = rows[len(patients) + 1:]
        self.assertTrue(all(len(row) == 2 for row in summary_rows))
        self.assertEqual(tuple(row[0] for row in summary_rows), SUMMARY_KEYS)
        # Decimal notation only; scientific notation is outside the output format.
        for _, value in summary_rows:
            self.assertRegex(value, r"^\d+\.\d+$")
        summary = {key: float(value) for key, value in summary_rows}
        self.assertTrue(all(math.isfinite(value) for value in summary.values()))

        elapsed = max(r[4] for r in actual.values()) - min(p[1] for p in patients)
        expected_summary = (
            sum(r[5] for r in actual.values()) / len(patients),
            sum(r[6] for r in actual.values()) / len(patients),
            sum(r[7] for r in actual.values()) / len(patients),
            len(patients) / elapsed,
            100 * sum(p[3] for p in patients) / (2 * elapsed),
        )
        for key, expected in zip(SUMMARY_KEYS, expected_summary):
            tolerance = 1e-15 if key == "THROUGHPUT" else 0.00000051
            self.assertAlmostEqual(summary[key], expected, delta=tolerance)
        self.assertGreater(summary["THROUGHPUT"], 0)
        self.assertLessEqual(summary["UTILIZATION"], 100)
        for arrival, _, service, start, completion, waiting, turnaround, response in actual.values():
            self.assertGreaterEqual(start, arrival)
            self.assertGreaterEqual(completion, start + service)
            self.assertGreaterEqual(waiting, response)
            self.assertEqual(turnaround, completion - arrival)
            self.assertEqual(waiting, turnaround - service)
            self.assertEqual(response, start - arrival)
        return actual, result.stdout

    def timings(self, patients, algorithm, expected):
        actual, _ = self.results(patients, algorithm)
        self.assertEqual({pid: (r[3], r[4]) for pid, r in actual.items()}, expected)

    def test_specification_sample(self):
        patients = [("P1",0,3,10,"FEVER"), ("P2",1,2,8,"FRACTURE"),
                    ("P3",2,1,12,"CRITICAL"), ("P4",3,3,5,"MINOR"),
                    ("P5",4,1,7,"SEVERE")]
        expected = {
            "FCFS": {"P1":(0,10), "P2":(1,9), "P3":(9,21), "P4":(10,15), "P5":(15,22)},
            "PRIORITY": {"P1":(0,10), "P2":(1,9), "P3":(9,21), "P4":(17,22), "P5":(10,17)},
            "PRIORITY_PREEMPTIVE": {"P1":(0,22), "P2":(1,16), "P3":(2,14), "P4":(16,21), "P5":(4,11)},
        }
        for algorithm in ALGORITHMS:
            with self.subTest(algorithm=algorithm):
                self.timings(patients, algorithm, expected[algorithm])
                _, output = self.results(patients, algorithm)
                fixture = ROOT / "samples" / "expected" / f"hospital.{algorithm}.csv"
                self.assertEqual(output, fixture.read_text())

    def test_single_late_arrival(self):
        for algorithm in ALGORITHMS:
            self.timings([("P1",5,1,3,"FEVER")], algorithm, {"P1":(5,8)})

    def test_idle_gaps(self):
        patients = [("P1",5,1,1,"A"), ("P2",20,1,2,"B")]
        for algorithm in ALGORITHMS:
            self.timings(patients, algorithm, {"P1":(5,6), "P2":(20,22)})

    def test_lexicographic_ties(self):
        patients = [("P2",0,2,2,"A"), ("P10",0,2,2,"B"), ("P1",0,2,2,"C")]
        for algorithm in ALGORITHMS:
            self.timings(patients, algorithm, {"P1":(0,2), "P10":(0,2), "P2":(2,4)})

    def test_priority_uses_numbers_not_categories(self):
        patients = [("P1",0,3,2,"CRITICAL"), ("P2",0,2,2,"FEVER"), ("P3",0,1,2,"MINOR")]
        self.timings(patients, "FCFS", {"P1":(0,2), "P2":(0,2), "P3":(2,4)})
        for algorithm in ALGORITHMS[1:]:
            self.timings(patients, algorithm, {"P1":(2,4), "P2":(0,2), "P3":(0,2)})

    def test_equal_priority_never_preempts(self):
        patients = [("P1",0,1,5,"A"), ("P2",0,1,5,"B"), ("P3",1,1,1,"C")]
        self.timings(patients, "PRIORITY_PREEMPTIVE", {"P1":(0,5), "P2":(0,5), "P3":(5,6)})

    def test_preemption_victim_later_arrival(self):
        patients = [("P1",0,3,10,"A"), ("P2",1,3,10,"B"), ("P3",2,1,2,"C")]
        self.timings(patients, "PRIORITY_PREEMPTIVE", {"P1":(0,10), "P2":(1,13), "P3":(2,4)})

    def test_preemption_victim_larger_id(self):
        patients = [("P2",0,3,10,"B"), ("P1",0,3,10,"A"), ("P3",1,1,2,"C")]
        self.timings(patients, "PRIORITY_PREEMPTIVE", {"P1":(0,10), "P2":(0,12), "P3":(1,3)})

    def test_preemption_victim_lowest_priority(self):
        patients = [("P1",0,5,10,"A"), ("P2",1,3,10,"B"), ("P3",2,1,2,"C")]
        self.timings(patients, "PRIORITY_PREEMPTIVE", {"P1":(0,12), "P2":(1,11), "P3":(2,4)})

    def test_free_doctor_before_preemption(self):
        patients = [("P1",0,5,10,"A"), ("P2",1,1,2,"B")]
        self.timings(patients, "PRIORITY_PREEMPTIVE", {"P1":(0,10), "P2":(1,3)})

    def test_two_simultaneous_preemptions(self):
        patients = [("P1",0,5,10,"A"), ("P2",0,5,10,"B"),
                    ("P3",1,1,2,"C"), ("P4",1,2,2,"D")]
        self.timings(patients, "PRIORITY_PREEMPTIVE", {"P1":(0,12), "P2":(0,12), "P3":(1,3), "P4":(1,3)})

    def test_repeated_preemption_preserves_first_start(self):
        patients = [("P1",0,5,10,"A"), ("P2",0,5,10,"B"),
                    ("P3",1,3,10,"C"), ("P4",2,1,1,"D"), ("P5",4,1,1,"E")]
        self.timings(patients, "PRIORITY_PREEMPTIVE", {"P1":(0,12), "P2":(0,20), "P3":(1,11), "P4":(2,3), "P5":(4,5)})

    def test_arrivals_and_completions_at_same_time(self):
        patients = [("P1",0,3,5,"A"), ("P2",0,3,5,"B"),
                    ("P3",1,3,10,"C"), ("P4",1,3,10,"D"), ("P5",5,1,1,"E")]
        self.timings(patients, "FCFS", {"P1":(0,5), "P2":(0,5), "P3":(5,15), "P4":(5,15), "P5":(15,16)})
        for algorithm in ALGORITHMS[1:]:
            self.timings(patients, algorithm, {"P1":(0,5), "P2":(0,5), "P3":(5,15), "P4":(6,16), "P5":(5,6)})

    def test_large_times_do_not_overflow(self):
        maximum = 2147483647
        patients = [(f"P{i}",maximum,1,maximum,"A") for i in range(1,4)]
        for algorithm in ALGORITHMS:
            self.timings(patients, algorithm, {"P1":(maximum,2*maximum),
                         "P2":(maximum,2*maximum), "P3":(2*maximum,3*maximum)})

    def test_more_than_one_thousand_patients(self):
        patients = [(f"P{i:04}",0,1,1,"A") for i in range(1001)]
        actual, _ = self.results(patients, "FCFS")
        self.assertEqual(actual["P1000"][3:5], (500,501))

    def test_long_ids_and_categories(self):
        patients = [("P" * 300,0,1,1,"A" * 400)]
        self.timings(patients, "FCFS", {"P" * 300:(0,1)})

    def test_crlf_whitespace_and_missing_final_newline(self):
        patients = [("P1",0,1,1,"A")]
        data = " 1 \r\n 2 \r\n\r\n P1 , 0 , 1 , 1 , A "
        actual, _ = self.results(patients, "FCFS", data=data)
        self.assertEqual(actual["P1"][3:5], (0,1))

    def test_invalid_inputs_fail_without_result_output(self):
        invalid = [
            "", "0\n2\n", "-1\n2\n", "1\n0\nP1,0,1,1,A\n",
            "1\n-2\nP1,0,1,1,A\n", "1x\n2\nP1,0,1,1,A\n",
            "1\n2x\nP1,0,1,1,A\n", "1\n2\nP1,-1,1,1,A\n",
            "1\n2\nP1,0,0,1,A\n", "1\n2\nP1,0,1,0,A\n",
            "1\n2\nP1,0,1,1,A,B\n", "1\n2\n,0,1,1,A\n",
            "1\n2\nP1,0,1,1,\n", "1\n2\nP1,0,1,1x,A\n",
            "1\n2\nP1,2147483648,1,1,A\n",
            "1\n2\nP1,0,1,999999999999999999999999999,A\n",
            "2147483648\n2\n", "1\n2147483648\nP1,0,1,1,A\n",
            "2\n2\nP1,0,1,1,A\nP1,1,1,1,B\n",
            "2\n2\nP1,0,1,1,A\n", "1\n2\nP1,0,1,1,A\nP2,1,1,1,B\n",
            "1\n2\nP1,0,1,1,A\x00B\n", "1\n2\nP1,0,1,A\n",
        ]
        for data in invalid:
            with self.subTest(data=data):
                result = invoke(data)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, "")
                self.assertTrue(result.stderr)

    def test_cli_errors(self):
        missing_file = ROOT / "samples" / "file-that-does-not-exist.txt"
        for arguments in [[], [str(missing_file)], [str(missing_file), "FCFS"],
                          [str(missing_file), "RR"], [str(missing_file), "SJF"],
                          [str(missing_file), "FCFS", "extra"]]:
            with self.subTest(arguments=arguments):
                result = subprocess.run([str(BINARY), *arguments],
                                        capture_output=True, text=True, timeout=10)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, "")
                self.assertTrue(result.stderr)

    def test_determinism_and_input_order_independence(self):
        patients = [("P2",0,3,7,"A"), ("P10",0,3,7,"B"),
                    ("P1",1,1,2,"C"), ("P3",2,2,3,"D")]
        for algorithm in ALGORITHMS:
            _, first = self.results(patients, algorithm)
            _, repeated = self.results(patients, algorithm)
            _, reversed_input = self.results(list(reversed(patients)), algorithm)
            self.assertEqual(first, repeated)
            self.assertEqual(first, reversed_input)

    def test_randomized_against_unit_time_reference(self):
        rng = random.Random(3710)
        for case in range(120):
            patients = [(f"P{i+1}",rng.randrange(21),rng.randrange(1,6),
                         rng.randrange(1,13),"TEST") for i in range(rng.randrange(1,13))]
            rng.shuffle(patients)
            for algorithm in ALGORITHMS:
                with self.subTest(case=case, algorithm=algorithm):
                    actual, _ = self.results(patients, algorithm)
                    self.assertEqual(actual, reference(patients, algorithm))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=BINARY)
    parser.add_argument("--failfast", action="store_true")
    args = parser.parse_args()
    BINARY = args.binary.resolve()
    unittest.main(argv=[__file__], verbosity=2, failfast=args.failfast)
