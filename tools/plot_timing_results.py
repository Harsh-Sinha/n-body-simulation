import os
import re
import argparse
import matplotlib.pyplot as plt

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
            if ":" in line:
                name, value = line.split(":")
                data[name.strip()] = float(value.strip())
    return data

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

    # Sort for neat plots
    records.sort(key=lambda r: (r[1], r[0]))

    particleValues = sorted(set(r[0] for r in records))
    threadValues = sorted(set(r[1] for r in records))

    timingFields = [
        "octree creation",
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
            y = [r[2][field] for r in subset]

            plt.figure(figsize=(10, 6))
            plt.plot(x, y, marker="o")

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
            y = [r[2][field] for r in subset]

            plt.figure(figsize=(10, 6))
            # skip plotting the serial timing
            plt.plot(x[1:], y[1:], marker="o")

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
