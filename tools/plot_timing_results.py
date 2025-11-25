import os
import re
import argparse
import matplotlib.pyplot as plt
import math

def parse_filename(filename):
    particleMap = {
        "ten_thousand": 10_000,
        "hundred_thousand": 100_000,
        "five_hundred_thousand": 500_000,
        "million": 1_000_000
    }

    m = re.match(r"(.+)_p(\d+)\.txt", filename)
    if not m:
        return None

    label = m.group(1)
    threads = int(m.group(2))

    particles = particleMap.get(label)
    return None if particles is None else (particles, threads)


def parse_file(path):
    data = {}
    with open(path) as f:
        for line in f:
            if (line.startswith("    ") or line.startswith("\t")) and ":" not in line:
                parts = line.strip().split()
                if len(parts) >= 2:
                    name = " ".join(parts[:-1])
                    value = float(parts[-1])
                    data[name] = value
                continue
            if line.startswith("    ") or line.startswith("\t"):
                if ":" in line:
                    subname, subvalue = line.strip().split(":")
                    data[subname.strip()] = float(subvalue.strip())
                continue

            if ":" in line:
                name, value = line.split(":")
                data[name.strip()] = float(value.strip())
    return data

def determine_big_o_coeff(field, x, y):
    denom = 0.0
    if field == "octree creation" or field == "overall" or field == "applying forces calculation" or field == "insert points":
        # O(nlogn) algorithms
        denom = x * math.log(x, 10)
    elif field == "center of mass calculation":
        # O(8^logn) algorithm
        denom = 8.0 ** (math.log(x, 10))
    else: # field == "update pos/vel/acc" or field == "compute bounding box" or field == "generate leaf nodes"
        # O(n) algorithms
        denom = x
    return y / denom

def main():
    parser = argparse.ArgumentParser(description="plot profiling data.")
    parser.add_argument("--input", help="directory containing profiling .txt files")
    parser.add_argument("--output", help="directory to save output plots")
    args = parser.parse_args()

    os.makedirs(args.output, exist_ok=True)

    records = []
    for fname in os.listdir(args.input):
        if not fname.endswith(".txt"):
            continue

        parsed = parse_filename(fname)
        if parsed is None:
            continue

        particles, threads = parsed
        timings = parse_file(os.path.join(args.input, fname))

        records.append((particles, threads, timings))

    if not records:
        print("Nn valid profiling files found.")
        return

    records.sort(key=lambda r: (r[1], r[0]))

    particleValues = sorted(set(r[0] for r in records))
    threadValues = sorted(set(r[1] for r in records))

    timingFields = [
        "octree creation",
        "compute bounding box",
        "insert points",
        "generate leaf nodes",
        "center of mass calculation",
        "applying forces calculation",
        "update pos/vel/acc",
        "overall"
    ]

    # fix thread count and plot scaling
    for thread in threadValues:

        # create folder: output/thread_X
        threadDir = os.path.join(args.output, f"thread_{thread}")
        os.makedirs(threadDir, exist_ok=True)

        subset = [r for r in records if r[1] == thread]
        if not subset:
            continue

        x = [r[0] for r in subset]  # particle counts

        for field in timingFields:
            y = [r[2].get(field, 0.0) for r in subset] 

            plt.figure(figsize=(10, 6))
            plt.plot(x, y, marker="o")

            coeffs = ''

            # Add point labels
            for xi, yi in zip(x, y):
                plt.text(
                    xi,
                    yi,
                    f"({xi}, {yi:.1f})",
                    fontsize=16,
                    ha="left",
                    va="bottom"
                )
                c = determine_big_o_coeff(field, xi, yi)
                coeffs += f"n={xi} coeff={c:.4f}\n"

            plt.xlabel("Particle Count")
            plt.ylabel("Time (ms)")
            plt.title(f"{field} vs Particle Count (Threads = {thread})")
            plt.grid(True)
            plt.xscale("log")
            plt.tight_layout()

            safeField = field.replace(" ", "_").replace("/", "_")
            filename = f"{safeField}.png"

            plt.savefig(os.path.join(threadDir, filename))
            plt.close()

            filename = f"{safeField}.txt"
            with open(os.path.join(threadDir, filename), "w") as file:
                file.write(coeffs)

    # fix particle count and plot speedup
    for particles in particleValues:

        # create folder: output/particle_X
        particleDir = os.path.join(args.output, f"particle_{particles}")
        os.makedirs(particleDir, exist_ok=True)

        subset = [r for r in records if r[0] == particles]
        if not subset:
            continue

        x = [r[1] for r in subset]  # thread counts

        for field in timingFields:
            y = [r[2].get(field, 0.0) for r in subset]

            plt.figure(figsize=(10, 6))
            # skip plotting the serial timing
            plt.plot(x[1:], y[1:], marker="o")

            # Add point labels
            for xi, yi in zip(x[1:], y[1:]):
                plt.text(
                    xi,
                    yi,
                    f"({xi}, {yi:.1f})",
                    fontsize=16,
                    ha="left",
                    va="bottom"
                )

            plt.xlabel("Thread Count")
            plt.ylabel("Time (ms)")
            plt.title(f"{field} vs Thread Count (Particles = {particles}, Serial = {y[0]})")
            plt.grid(True)
            plt.tight_layout()

            safeField = field.replace(" ", "_").replace("/", "_")
            filename = f"{safeField}.png"

            plt.savefig(os.path.join(particleDir, filename))
            plt.close()

if __name__ == "__main__":
    main()
