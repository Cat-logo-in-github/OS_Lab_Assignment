"""Run every hospital policy and print a Markdown comparison of actual results."""

import argparse
import csv
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input_file", type=Path)
    parser.add_argument("--binary", type=Path,
                        default=Path(__file__).resolve().parents[1] / "hospital")
    args = parser.parse_args()
    print(f"Input: {args.input_file.name}\n")
    print("| Algorithm | Avg waiting | Avg turnaround | Avg response | "
          "Avg completion | Last completion | Throughput | Utilization (%) |")
    print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")

    for algorithm in ("FCFS", "PRIORITY", "PRIORITY_PREEMPTIVE"):
        try:
            result = subprocess.run(
                [str(args.binary.resolve()), str(args.input_file), algorithm],
                capture_output=True, text=True, check=True,
            )
        except (OSError, subprocess.CalledProcessError) as error:
            message = error.stderr if isinstance(error, subprocess.CalledProcessError) else str(error)
            parser.exit(1, f"{algorithm}: {message}\n")

        rows = list(csv.reader(result.stdout.splitlines()))
        marker = rows.index(["SUMMARY"])
        completions = [int(row[5]) for row in rows[:marker]]
        summary = {key: float(value) for key, value in rows[marker + 1:]}
        print(f"| {algorithm} | {summary['AVERAGE_WAITING']:.3f} | "
              f"{summary['AVERAGE_TURNAROUND']:.3f} | "
              f"{summary['AVERAGE_RESPONSE']:.3f} | "
              f"{sum(completions) / len(completions):.3f} | "
              f"{max(completions)} | {summary['THROUGHPUT']:.6f} | "
              f"{summary['UTILIZATION']:.3f} |")


if __name__ == "__main__":
    main()
