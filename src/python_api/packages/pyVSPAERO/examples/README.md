# pyVSPAERO Examples

Python post-processing examples for VSPAERO using the
[pyvspaero](../) package.

Scripts fall into two categories:

- **Batch scripts** generate PNG images (and animation frame sequences) in
  the same directory as the input `*.adb` file.  Pass `--interactive` to
  open a live pyvista window instead.
- **Interactive utilities** (`view_case.py`) open a live pyvista window by
  default; pass `--output` to write a PNG instead.

## Setup

Install pyvspaero with the optional plotting dependencies:

```bash
pip install 'pyvspaero[plot]'
```

Or, if working from the OpenVSP source tree, install directly from the
package directory:

```bash
pip install -e 'src/python_api/packages/pyVSPAERO[plot]'
```

## Common interface

Most batch scripts share this argument convention:

```
<file.adb>      Path to the VSPAERO *.adb output file.
[case_index]    1-based case number to visualize (default: 1).
[--interactive] Open a live pyvista window instead of writing PNG(s).
```

`mesh_view.py` does not take `case_index` (mesh topology is identical across
all cases in a steady sweep, so case 1 is always loaded).  `quad_plot.py`
accepts additional `--scalar`, `--planes`, and `--transparent` options; see
its section below.

## Scripts

### `view_case.py` -- Interactive quick-look viewer

Quick-look utility for any `*.adb` file.  Opens a live pyvista window by
default showing the surface colored by Cp with tube wake and computational
mesh overlay.

```bash
python view_case.py <file.adb> [case_index] [--output FILE]
```

| Argument | Description |
|----------|-------------|
| `file.adb` | VSPAERO *.adb output file |
| `case_index` | 1-based case to display (default: 1) |
| `--output FILE` | Write a PNG to FILE instead of opening an interactive window |

**Examples:**
```bash
python view_case.py Wing.adb              # interactive, case 1
python view_case.py Wing.adb 3            # interactive, case 3
python view_case.py Wing.adb --output out.png        # save PNG, case 1
python view_case.py Wing.adb 2 --output case2.png    # save PNG, case 2
```

**Features:**
- Smooth-shaded surface colored by Cp, tube wake, mesh edges, Cp colorbar
- Global mean +/- 1 sigma colormap limits (consistent across all cases in the file)
- Symmetric half-models reflected automatically
- Interactive: left-drag to rotate, right-drag to zoom, middle-drag to pan

---

### `cp_plot.py` -- Cp surface with mesh overlay

Isometric and top-view Cp plots with the computational mesh drawn on top.
Demonstrates basic surface Cp visualization, colormap auto-ranging, and
handling of symmetric models and propulsion elements.

```bash
python cp_plot.py Wing.adb
python cp_plot.py Wing.adb 2 --interactive
```

**Export output** (written alongside `<file>.adb`):

| File | Description |
|------|-------------|
| `{stem}_case{N}_cp_iso.png` | Isometric view with mesh overlay |
| `{stem}_case{N}_cp_top.png` | Top (plan) view |
| `{stem}_case{N}_propulsion.png` | Propulsion elements (only if present) |

---

### `wake_plot.py` -- Cp surface with wake filament overlay

Cp surface plot with the near-wake vortex filaments drawn as lines.
Demonstrates wake line visualization and the extend_to_infinity option.

```bash
python wake_plot.py Wing.adb
python wake_plot.py Wing.adb 3 --interactive
```

**Export output:**

| File | Description |
|------|-------------|
| `{stem}_case{N}_wake.png` | Isometric Cp + wake lines |

---

### `wake_modes.py` -- Side-by-side wake style comparison

2x2 subplot comparing all four wake rendering styles (lines, tubes, points,
surface) with a shared Cp colormap.  Demonstrates `plot_wake_modes()`.

```bash
python wake_modes.py Wing.adb
python wake_modes.py WingBody.adb 2 --interactive
```

**Export output:**

| File | Description |
|------|-------------|
| `{stem}_case{N}_wake_modes.png` | 2x2 wake style comparison |

---

### `sweep_plots.py` -- Per-case Cp plots with shared colormap

Writes one Cp PNG per case, all using the same global mean +/- 1 sigma colormap
so colors are directly comparable across the sweep.  Also prints a sweep
summary table and per-case auto ranges to stdout.
Pass `--interactive` to open a live window for a single case instead.

```bash
python sweep_plots.py Wing.adb
python sweep_plots.py Wing.adb 2 --interactive
```

**Export output:**

| File | Description |
|------|-------------|
| `{stem}_sweep_case{N}_cp.png` | One file per case, shared colormap |

---

### `animate_sweep.py` -- Animation frame sequence

Writes one numbered PNG frame per case into a `{stem}_frames/` directory.
All frames share the global mean +/- 1 sigma colormap.  Pass `--interactive` to
preview a single case instead of rendering all frames.

```bash
python animate_sweep.py Wing.adb
python animate_sweep.py Wing.adb 2 --interactive
```

**Export output:**

| File | Description |
|------|-------------|
| `{stem}_frames/frame_*.png` | One frame per case |

Assemble frames into a video with ffmpeg:

```bash
ffmpeg -r 24 -i Wing_frames/frame_%04d.png -c:v libx264 animation.mp4
```

---

### `mesh_view.py` -- Multi-panel mesh level comparison

Renders one panel per mesh representation available in the file: the full
surface triangle mesh followed by each coarse polygon mesh level from finest
to coarsest.  The subplot grid is sized automatically.  All 3-D views are
linked -- rotating or zooming in any panel moves all others in sync.

No Cp solution or colorbar is shown.  Always loads case 1 (mesh topology is
identical across all cases in a steady sweep).

```bash
python mesh_view.py Wing.adb
python mesh_view.py WingBody.adb --interactive
```

**Export output:**

| File | Description |
|------|-------------|
| `{stem}_mesh_levels.png` | All mesh levels in a grid |

---

### `quad_plot.py` -- Cp surface with quad cutting-plane flow-field overlay

Renders the aircraft surface colored by Cp overlaid with the VSPAERO
adaptive quad cutting-plane slices.  Each slice shows the flow-field scalar
distribution on the quadtree mesh computed by VSPAERO.  Requires
`*.quad.cases` and `*.case.N.quad.M.dat` files alongside the `*.adb`
file (generated by VSPAERO when cutting planes are configured).

```bash
python quad_plot.py Wing.adb
python quad_plot.py WingBody.adb 3 --interactive
python quad_plot.py Wing.adb 2 --scalar Vmag
python quad_plot.py Wing.adb --planes 1,3,5
python quad_plot.py Wing.adb --planes 1-4,6 --transparent
```

| Argument | Description |
|----------|-------------|
| `file.adb` | VSPAERO *.adb output file |
| `case_index` | 1-based case to display (default: 1) |
| `--scalar FIELD` | Scalar for quad slices: `Cp` (default) or `Vmag` |
| `--planes SPEC` | Comma/range selection of cutting planes, e.g. `1,3` or `1-4,6` (default: all) |
| `--transparent` | Render the surface at reduced opacity so slices beneath are visible |
| `--interactive` | Open a live window instead of writing a PNG |

**Export output:**

| File | Description |
|------|-------------|
| `{stem}_case{N}_quad_{scalar}.png` | Surface Cp + quad slices |

---

*More examples will be added covering unsteady time-accurate analyses.*
