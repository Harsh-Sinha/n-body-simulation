import argparse
import struct
import numpy as np
import plotly.graph_objects as go
import json

def parse_simulation_file(filename):
    with open(filename, "rb") as file:
        header = file.read(16)
        n = struct.unpack("<Q", header[0:8])[0]
        dt = struct.unpack("<d", header[8:16])[0]

        mass_list = np.fromfile(file, dtype="<f8", count=n)
        data = np.fromfile(file, dtype="<f8")

        per_particle = n * 3
        numFrames = data.size // per_particle
        positions = data.reshape((numFrames, n, 3))

        return n, dt, mass_list, positions

def mass_to_particle_size(mass_list, min=4, max=14):
    massMin = mass_list.min()
    massMax = mass_list.max()

    if (massMin == massMax):
        return np.full_like(mass_list, (min + max) / 2)

    norm = np.sqrt((mass_list - massMin) / (massMax - massMin))

    return min + norm * (max - min)

def generate_visualization(n, dt, mass_list, positions, filename):
    numFrames = positions.shape[0]
    particleSizes = mass_to_particle_size(mass_list)
    colors = (mass_list - mass_list.min()) / (mass_list.max() - mass_list.min() + 1e-12)

    x0 = positions[0, :, 0]
    y0 = positions[0, :, 1]
    z0 = positions[0, :, 2]

    # fixed axis limits for better visualization... otherwise axis changes and particles appear not to move
    mins = positions.min(axis=(0, 1))
    maxs = positions.max(axis=(0, 1))
    xMin, yMin, zMin = mins
    xMax, yMax, zMax = maxs

    px = 0.05 * (xMax - xMin if xMax > xMin else 1)
    py = 0.05 * (yMax - yMin if yMax > yMin else 1)
    pz = 0.05 * (zMax - zMin if zMax > zMin else 1)

    fig = go.Figure(
        data=[
            go.Scatter3d(
                x=x0,
                y=y0,
                z=z0,
                mode="markers",
                marker=dict(
                    size=particleSizes,
                    color=colors,
                    colorscale="Viridis",
                    opacity=0.9,
                )
            )
        ]
    )

    fig.update_layout(
        title="n body simulation of " + str(n) + " particles",
        width=900,
        height=700,
        scene=dict(
            xaxis=dict(range=[xMin - px, xMax + px]),
            yaxis=dict(range=[yMin - py, yMax + py]),
            zaxis=dict(range=[zMin - pz, zMax + pz]),
            aspectmode="manual",
            aspectratio=dict(x=1, y=1, z=1)
        )
    )

    framesJson = positions.tolist()

    html = fig.to_html(full_html=True, include_plotlyjs=True, div_id="viewer")

    js = f"""
<script>
    const frames = {json.dumps(framesJson)};
    let frame = 0;
    const nframes = frames.length;
    const interval = {int(dt * 1000)};
    const dt = {dt};

    let animInterval = null;
    let isPlaying = false;

    function updateFrame(i) {{
        const pos = frames[i];
        const x = pos.map(p => p[0]);
        const y = pos.map(p => p[1]);
        const z = pos.map(p => p[2]);

        // update visualization of particles
        Plotly.animate('viewer', {{
            data: [{{ x: x, y: y, z: z }}]
        }}, {{
            transition: {{ duration: 0 }},
            frame: {{ duration: 0, redraw: true }}
        }});

        // handle slider
        const slider = document.getElementById("frameslider");
        slider.value = i;

        // update time label
        const t = i * dt;
        const timeLabel = document.getElementById("timelabel");
        if (timeLabel) {{
            timeLabel.textContent = t.toFixed(3) + " s";
        }}
    }}

    function togglePlayPause() {{
        const btn = document.getElementById("playpause");

        if (!isPlaying) {{
            btn.innerHTML = "Pause";
            isPlaying = true;

            animInterval = setInterval(() => {{
                frame = (frame + 1) % nframes;
                updateFrame(frame);
            }}, interval);

        }} else {{
            btn.innerHTML = "Play";
            isPlaying = false;

            clearInterval(animInterval);
            animInterval = null;
        }}
    }}

    function sliderChanged() {{
        if (isPlaying) {{
            togglePlayPause();
        }}

        const slider = document.getElementById("frameslider");
        frame = parseInt(slider.value);
        updateFrame(frame);
    }}
</script>

<div style="padding:10px;">
    <button id="playpause" onclick="togglePlayPause()">Play</button>
</div>

<div style="padding:10px;">
    Time: <span id="timelabel">0.000 s</span>
</div>

<div style="padding:10px;">
    Frame:
    <input type="range"
           id="frameslider"
           min="0"
           max="{numFrames - 1}"
           value="0"
           style="width:600px;"
           oninput="sliderChanged()">
</div>
"""

    html = html.replace("</body>", js + "\n</body>")

    with open(filename, "w") as file:
        file.write(html)

def main():
    parser = argparse.ArgumentParser(description="n body simulation visualization generator")
    parser.add_argument("--input", help="simulation binary generated using barnes hut simulation")
    parser.add_argument("--out", help="output html file to save the visualization to")
    args = parser.parse_args()

    n, dt, mass_list, positions = parse_simulation_file(args.input)
    generate_visualization(n, dt, mass_list, positions, args.out)

if __name__ == "__main__":
    main()