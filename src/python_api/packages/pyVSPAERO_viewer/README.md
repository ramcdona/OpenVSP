# pyVSPAERO Viewer

Desktop GUI for interactively exploring VSPAERO `*.adb` output files.

The viewer displays the aircraft surface colored by any scalar field (Cp,
steady Cp, unsteady Cp, or vorticity), overlaid with the computational mesh,
trailing wakes, and propulsion elements (rotor disks and engine nozzles).
Multi-case sweeps can be stepped through interactively.  When companion
`*.quad.cases` cutting-plane data is present a Cut Planes tab provides
interactive control over which planes are shown and how they are colored.

## Installation

```bash
pip install pyvspaero_viewer
```

Dependencies: PySide6, pyvistaqt, pyvista, pyvspaero.

From the OpenVSP source tree (editable install):

```bash
pip install -e 'src/python_api/packages/pyVSPAERO_viewer'
```

## Running

**Command line:**

```bash
python -m pyvspaero_viewer path/to/file.adb
python -m pyvspaero_viewer path/to/file.adb 3            # open at case 3
python -m pyvspaero_viewer path/to/file.adb --output out.png  # headless PNG
```

**Programmatic (embed in a larger application):**

```python
from PySide6.QtWidgets import QApplication
import sys

qt_app = QApplication(sys.argv)

from pyvspaero_viewer import VSPAEROViewerApp
viewer = VSPAEROViewerApp('path/to/file.adb', case_index=1)
viewer.run()

sys.exit(qt_app.exec())
```

## Interface

### 3-D View

| Interaction | Action |
|-------------|--------|
| Left drag | Rotate |
| Right drag | Zoom |
| Middle drag | Pan |
| Scroll wheel | Zoom |

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| F9 | Isometric view |
| F1 | Top view |
| F2 | Bottom view |
| F3 | Front view |
| F4 | Rear view |
| F5 | Left view |
| F6 | Right view |
| + or = | Next coarser mesh level |
| - | Next finer mesh level |
| Ctrl+O | Open ADB file |
| Ctrl+S | Save PNG |
| Ctrl+Q | Quit |

### Menus

**File**
- Open ADB -- open a different `*.adb` file.
- Save PNG -- save the current 3-D view to a PNG file.
- Save Animation Frames -- step through all cases and write one numbered PNG
  per case to a chosen directory.  After saving, a ready-to-use `ffmpeg`
  command is shown for assembling the frames into a video.

**View**
- View presets (Iso, Top, Bottom, Front, Rear, Left, Right) -- same as the
  F1-F9 keyboard shortcuts.
- Dark Background -- toggle between white and dark grey backgrounds.
- Show Annotations -- toggle the Mach/Alpha/Beta text overlay.

**Aero**
- Scalar field -- choose which quantity to display on the surface: Off, Shaded
  (no scalar), Cp, Steady Cp, Unsteady Cp, or Vorticity (Gamma).
- Triangulation -- show the fine input triangulation instead of (or in addition
  to) the coarse computational mesh.
- Computational Mesh -- show the coarse VLM/panel mesh edges.
- Trailing Wakes -- show wake filaments.
- Propulsion Elements -- show rotor disk and nozzle geometry.
- Reflect Geometry -- mirror the half-model about the symmetry plane.

**Options**
- Smooth / Flat Shading.
- Solid / Transparent / Wireframe surface style.
- Mesh Level -- select which coarse multi-grid level to display (1 = finest
  coarse level; higher numbers are coarser).  Also controlled by +/- keys.
- Wake Style -- Lines, Tubes, Points, or Surface.
- Wake Color -- Uniform (single color), By Wing (one color per trailing
  surface), or By Span (span-wise gradient).
- Extend Wake to Infinity -- include the far-field wake extension appended
  by the solver.
- Line Contours -- overlay iso-contour lines on the surface.
- Line Weight -- global line width for mesh edges and wake lines.

**Legend**
- Colorbar -- show or hide the scalar color scale bar.
- Auto Color Range -- when checked, the colormap limits track the current
  scalar field's data range automatically.  Uncheck to lock the range.
- Contour Settings -- dialog to set colormap min/max and number of contour
  levels manually.
- Colormap -- choose from Red-Blue, Cool-Warm, Seismic, Viridis, Plasma,
  Jet, or Rainbow.
- CG Marker -- show the center-of-gravity location from the file header.
- Axis Triad -- show the XYZ axis indicator.

### Solution Panel (right sidebar)

- Navigate cases with the **<** and **>** buttons or the case drop-down.
- The current case's Mach, Alpha, and Beta are displayed below the controls.
- Toggle individual surface component visibility with the checklist at the
  bottom of the panel.

### Cut Planes Tab (right sidebar, when quad data is present)

Visible only when the `*.quad.cases` and `*.case.N.quad.M.dat` files are found
alongside the `*.adb` file.

- Enable or disable individual cutting planes with the checklist.
- **Select All / Deselect All** buttons for bulk toggling.
- Choose the scalar field: Cp or velocity magnitude (Vmag).
- Toggle the quadtree mesh edges (Show Mesh).
- Adjust surface opacity with the slider.
- Smooth / Flat shading toggle.
- Overlay iso-contour lines on the quad planes (Show Contours).
- Show velocity vector arrows (Show Vectors) with a scale factor slider.

## Saving Output

**PNG** -- File > Save PNG (or Ctrl+S) saves the current 3-D view at the
window resolution.

**Animation frames** -- File > Save Animation Frames steps through all cases
and writes `frame_0001.png`, `frame_0002.png`, ... to a directory you choose.
After saving, the dialog shows an `ffmpeg` command you can run to assemble the
frames:

```bash
ffmpeg -framerate 24 -i "frames/frame_%04d.png" -c:v libx264 -pix_fmt yuv420p animation.mp4
```
