import os
import re
import argparse

def parse_filename(filename):
    particleMap = {
        "ten_thousand": 10_000,
        "hundred_thousand": 100_000,
        "five_hundred_thousand": 500_000,
        "million": 1_000_000
    }

    m = re.match(r"(.+)_p(\d+)\.perf\.txt$", filename)
    if not m:
        return None

    label = m.group(1)
    threads = int(m.group(2))

    particles = particleMap.get(label)
    return None if particles is None else (particles, threads)


def parse_file(path):
    data = {}
    currentSection = None

    with open(path) as f:
        for line in f:
            line = line.strip()

            if line.startswith("Section:"):
                currentSection = line.split("Section:")[1].strip()
                data[currentSection] = {}
                continue

            if currentSection is None:
                continue

            if ":" in line:
                name, value = line.split(":")
                name = name.strip()
                value = value.strip()

                if name in ("cache-miss %", "cache-misses / instructions", "IPC"):
                    data[currentSection][name] = float(value)

    return data


def particle_label(n):
    if n == 10_000:
        return "10k"
    if n == 100_000:
        return "100k"
    if n == 500_000:
        return "500k"
    if n == 1_000_000:
        return "1M"
    return str(n)


def write_latex_table(path, rows, firstHeader, caption):
    with open(path, "w") as out:
        out.write("\\begin{table}[H]\n")
        out.write("\\centering\n")
        out.write("\\begin{tabular}{c|c|c|c}\n")

        out.write(
            f"\\textbf{{{firstHeader}}} & "
            f"\\textbf{{IPC}} & "
            f"\\textbf{{cache-miss \\%}} & "
            "\\begin{tabular}{c}\n"
            "\\textbf{cache-misses} \\\\\n"
            "\\textbf{/ instructions}\n"
            "\\end{tabular} \\\\\n"
        )

        for r in rows:
            out.write("\\hline\n")
            out.write(f"{r[0]} & {r[1]} & {r[2]} & {r[3]} \\\\\n")

        out.write("\\hline\n")
        out.write("\\end{tabular}\n")
        out.write(f"\\caption{{{caption}}}\n")
        out.write("\\end{table}\n")


def main():
    parser = argparse.ArgumentParser(description="plot perf data.")
    parser.add_argument("--input", help="directory containing .perf.txt files")
    parser.add_argument("--output", help="directory to save output tables")
    args = parser.parse_args()

    os.makedirs(args.output, exist_ok=True)

    records = []
    for fname in os.listdir(args.input):
        if not fname.endswith(".perf.txt"):
            continue

        parsed = parse_filename(fname)
        if parsed is None:
            continue

        particles, threads = parsed
        perfData = parse_file(os.path.join(args.input, fname))

        records.append((particles, threads, perfData))

    if not records:
        print("No valid perf files found.")
        return

    records.sort(key=lambda r: (r[1], r[0]))
    particleValues = sorted(set(r[0] for r in records))
    threadValues = sorted(set(r[1] for r in records))

    metrics = [
        "IPC",
        "cache-miss %",
        "cache-misses / instructions"
    ]

    # fix thread count and write tables
    for thread in threadValues:

        threadDir = os.path.join(args.output, f"thread_{thread}")
        os.makedirs(threadDir, exist_ok=True)

        subset = [r for r in records if r[1] == thread]
        if not subset:
            continue

        sectionNames = set()
        for (_, _, perf) in subset:
            sectionNames.update(perf.keys())

        for section in sectionNames:
            rows = []
            for (particles, _, perf) in subset:
                sectionData = perf.get(section, {})
                vals = [sectionData.get(m, 0.0) for m in metrics]
                rows.append((
                    particle_label(particles),
                    f"{vals[0]:.6f}",
                    f"{vals[1]:.6f}",
                    f"{vals[2]:.6f}"
                ))

            caption = f"{section.replace('_', ' ')} metrics vs particle count (threads = {thread})"
            tablePath = os.path.join(threadDir, f"{section}.txt")

            write_latex_table(tablePath, rows, "Particles", caption)

    # fix particle count and write tables
    for particles in particleValues:

        particleDir = os.path.join(args.output, f"particle_{particles}")
        os.makedirs(particleDir, exist_ok=True)

        subset = [r for r in records if r[0] == particles]
        if not subset:
            continue

        sectionNames = set()
        for (_, _, perf) in subset:
            sectionNames.update(perf.keys())

        for section in sectionNames:
            rows = []
            for (_, threads, perf) in subset:
                sectionData = perf.get(section, {})
                vals = [sectionData.get(m, 0.0) for m in metrics]
                rows.append((
                    threads,
                    f"{vals[0]:.6f}",
                    f"{vals[1]:.6f}",
                    f"{vals[2]:.6f}"
                ))

            caption = f"{section.replace('_', ' ')} metrics vs thread count (particles = {particle_label(particles)})"
            tablePath = os.path.join(particleDir, f"{section}.txt")

            write_latex_table(tablePath, rows, "Threads", caption)


if __name__ == "__main__":
    main()
