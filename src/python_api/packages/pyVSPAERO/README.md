# pyVSPAERO

Python library for reading and post-processing VSPAERO output files.

pyVSPAERO parses VSPAERO's binary `*.adb` (Aerothermal Database) output files
and provides structured Python access to the mesh geometry, flow conditions,
and surface pressure data for every solved case in a steady sweep or
time-accurate run.  A separate visualization module (`pyvspaero.adb_plot`,
requires `pyvista`) builds interactive 3-D scenes and renders
publication-quality PNGs.

## Installation

Core library (no visualization):

```bash
pip install pyvspaero
```

With plotting support:

```bash
pip install 'pyvspaero[plot]'
```

From the OpenVSP source tree (editable install):

```bash
pip install -e 'src/python_api/packages/pyVSPAERO[plot]'
```

## Quick Start

```python
from pyvspaero import ADBFile

# Open a VSPAERO *.adb output file.
adb = ADBFile('Wing.adb')
print(adb)                    # ADBFile('Wing.adb', version=3, cases=3)
print(adb.NumberOfCases)      # 3

# Inspect the file header (reference geometry, surface names).
hdr = adb.Header
print(f'Sref={hdr.Sref:.4f}  Cref={hdr.Cref:.4f}  Bref={hdr.Bref:.4f}')
print(f'Surfaces: {hdr.SurfaceNameList[1:hdr.NumberOfSurfaces + 1]}')

# Load one case.
geom, soln = adb.LoadCase(1)
print(f'M={soln.Mach:.4f}  Alpha={soln.Alpha:.2f} deg  Beta={soln.Beta:.2f} deg')
print(f'Cp range: {soln.Cp[1:].min():.3f} to {soln.Cp[1:].max():.3f}')

# Iterate over all cases without holding all data in memory at once.
for geom, soln in adb:
    print(f'  M={soln.Mach:.4f}  Alpha={soln.Alpha:.2f} deg')
```

If VSPAERO also wrote a `*.adb.cases` companion file, per-case flow conditions
are available from `adb.CaseList`:

```python
for i in range(1, adb.NumberOfCases + 1):
    cr = adb.CaseList[i]    # CaseRecord
    print(f'Case {i}: M={cr.Mach}  Alpha={cr.Alpha} deg  Beta={cr.Beta} deg')
```

## Key Classes

### `ADBFile`

The top-level entry point.  Opens a `*.adb` file, reads its header once, then
provides random access to any case via `LoadCase()`.

```
ADBFile(filename)
    .Header                   -> ADBHeader
    .NumberOfCases            -> int
    .CaseList                 -> list[CaseRecord]   (1-based; None if *.adb.cases absent)
    .CaseCpAutoRange          -> list[(lo, hi)]     (1-based; per-case display ranges)
    .QuadData                 -> QuadFile           (None if no cutting-plane data)
    .LoadCase(n)              -> (ADBGeometry, ADBSolution)
    .global_cp_auto_range()   -> (lo, hi)   outlier-robust colormap limits across all cases
    .global_cp_data_bounds()  -> (lo, hi)   actual Cp data envelope across all cases
    .__iter__()               iterate over all (geom, soln) pairs in case order
    .__len__()                number of cases
```

### `ADBHeader`

File-level metadata: reference geometry, symmetry flag, and surface name table.
Read from `adb.Header`.

| Attribute | Type | Description |
|-----------|------|-------------|
| `Sref` | float | Reference area |
| `Cref` | float | Reference chord |
| `Bref` | float | Reference span |
| `Xcg`, `Ycg`, `Zcg` | float | Center of gravity coordinates |
| `SymmetryFlag` | int | 1 if a half-model was run with symmetry, 0 otherwise |
| `NumberOfSurfaces` | int | Number of named surfaces |
| `SurfaceNameList` | list | 1-based; `SurfaceNameList[i]` is the name of surface *i* |
| `ModelType` | int | 1 = VLM (Vortex Lattice), 2 = Panel method |
| `FILE_VERSION` | int | 2 or 3; version 3 adds nozzle support |

### `ADBGeometry`

Mesh geometry for one case.  Contains the fine triangulated surface mesh,
the multi-grid (coarse) mesh levels used by the solver, Kutta trailing-edge
data, control surface hinge geometry, and propulsion element positions.

| Attribute | Type | Description |
|-----------|------|-------------|
| `NumberOfTris` | int | Surface triangles on the fine mesh |
| `NumberOfNodes` | int | Mesh nodes on the fine mesh |
| `NodeList_x/y/z` | ndarray (float32) | Node coordinates; 1-based, shape `(N+1,)` |
| `TriList_node1/2/3` | ndarray (int32) | Triangle vertex indices; 1-based |
| `TriList_surface_id` | ndarray (int32) | Surface ID per triangle |
| `TriList_area` | ndarray (float32) | Triangle area |
| `NumberOfMeshLevels` | int | Number of coarse multi-grid levels |
| `CoarseMeshLevels` | list | 1-based list of `CoarseMeshLevel` |
| `NumberOfPropulsionElements` | int | Total rotors + nozzles |
| `PropulsionElementList` | list | 1-based list of `PropulsionElement` |
| `NumberOfControlSurfaces` | int | Number of control surfaces |
| `ControlSurface` | list | 1-based list of `ControlSurface` |

### `ADBSolution`

Flow solution for one case.  Surface pressure coefficients are on the fine
triangulated mesh; vortex loop strengths and velocities are on the coarse
computational mesh.

| Attribute | Type | Description |
|-----------|------|-------------|
| `Mach` | float | Mach number |
| `Alpha` | float | Angle of attack (degrees) |
| `Beta` | float | Sideslip angle (degrees) |
| `Cp` | ndarray (float32) | Total delta-Cp per triangle; 1-based, shape `(N+1,)` |
| `CpSteady` | ndarray (float32) | Steady component of Cp (`Cp - CpUnsteady`) |
| `CpUnsteady` | ndarray (float32) | Unsteady component of Cp |
| `Gamma` | ndarray (float32) | Circulation interpolated to the fine mesh |
| `GammaN` | ndarray (float64) | Vortex loop circulation on the computational mesh |
| `WakeFilaments` | list | 1-based list of `WakeFilament` |
| `NumberOfTrailingVortexEdges` | int | Number of wake filaments |

### `CaseRecord`

One entry from the `*.adb.cases` companion text file.

| Attribute | Type | Description |
|-----------|------|-------------|
| `Mach` | float | Mach number |
| `Alpha` | float | Angle of attack (degrees) |
| `Beta` | float | Sideslip angle (degrees) |
| `CommentLine` | str | Case label string from the solver |

### `CoarseMeshLevel`

One coarse multi-grid mesh level.  Used to render the computational mesh
(the panel grid seen in post-processing views).

| Attribute | Type | Description |
|-----------|------|-------------|
| `NumberOfNodes` | int | Node count at this level |
| `NumberOfEdges` | int | Edge count at this level |
| `NodeList_x/y/z` | ndarray (float32) | Node coordinates; 1-based |
| `EdgeList_node1/2` | ndarray (int32) | Edge endpoint node indices; 1-based |
| `EdgeList_SurfaceID` | ndarray (int32) | Surface ID per edge |
| `EdgeList_IsBoundaryEdge` | ndarray | Non-zero for boundary/Kutta edges |
| `EdgeList_IsKuttaEdge` | ndarray | Non-zero for trailing-edge (Kutta) edges |

### `PropulsionElement`

One rotor disk or engine nozzle.

| Attribute | Type | Description |
|-----------|------|-------------|
| `Type` | str | `'PROP_ROTOR'` or `'ENGINE_NOZZLE'` |
| `xyz` | ndarray (float64) | Center location, shape `(3,)` |
| `normal` | ndarray (float64) | Disk/face normal, shape `(3,)` |
| `radius` | float | Outer radius |
| `hub_radius` | float | Inner (hub) radius; zero for nozzles |
| `rpm` | float | Rotational speed (RPM); zero for nozzles |
| `ct` | float | Thrust coefficient; zero for nozzles |
| `cp` | float | Power coefficient; zero for nozzles |

### `WakeFilament`

One trailing vortex filament.

| Attribute | Type | Description |
|-----------|------|-------------|
| `te_node` | int | 1-based Kutta (trailing-edge) node index |
| `s_over_b` | float | Non-dimensional span location (s/b) |
| `NumberOfNodes` | int | Number of nodes along the filament |
| `x`, `y`, `z` | ndarray (float64) | Filament node coordinates; 1-based |

### `QuadFile` and `QuadSlicePlane`

Reader for the adaptive quadtree cutting-plane flow-field output written
alongside the `*.adb` file.  `ADBFile` automatically loads a `QuadFile`
when it finds a sibling `*.quad.cases` file.

```python
# QuadData is None if no *.quad.cases file was found.
qf = adb.QuadData
print(qf.NumberOfPlanes, qf.NumberOfCases)

planes = qf.LoadCase(1)     # list of QuadSlicePlane (1-based; [0] is None)
p = planes[1]
print(p)                    # QuadSlicePlane(Y=0.0, nodes=1234, cells=567)
print(p.Cp[1:].min(), p.Cp[1:].max())
print(p.velocity_magnitude[1:].max())
```

`QuadSlicePlane` attributes:

| Attribute | Type | Description |
|-----------|------|-------------|
| `direction` | int | Cutting-plane orientation: 1=X, 2=Y, 3=Z |
| `value` | float | Coordinate of the cutting plane along its axis |
| `Vref` | float | Freestream reference velocity |
| `Vmax` | float | Maximum velocity magnitude on this plane |
| `NumberOfNodes` | int | Node count; valid indices 1..N |
| `xyz` | ndarray (float64) | Node positions, shape `(N+1, 3)` |
| `velocity` | ndarray (float64) | Velocity vectors `[u, v, w]`, shape `(N+1, 3)` |
| `Cp` | ndarray (float64) | Pressure coefficient per node, shape `(N+1,)` |
| `NumberOfCells` | int | Quad cell count; valid indices 1..M |
| `cells` | ndarray (int32) | Quad connectivity, shape `(M+1, 4)`; 1-based node indices |
| `velocity_magnitude` | property | Per-node speed, shape `(N+1,)` |

## Indexing Convention

All per-element arrays throughout pyVSPAERO use **1-based indexing**, matching
the VSPAERO C++ source code.  Arrays have shape `(N+1,)` and index `[0]` is
always unused zero-padding.  Valid data starts at index `[1]`.

```python
geom, soln = adb.LoadCase(1)
n = geom.NumberOfTris

# Access all triangles by slicing from [1].
cp_values = soln.Cp[1:n + 1]   # shape (n,)  -- explicit
cp_values = soln.Cp[1:]         # shape (n,)  -- equivalent shorthand
```

Similarly, 1-based lists such as `PropulsionElementList`, `CoarseMeshLevels`,
and `WakeFilaments` have `None` at index `[0]`; real entries begin at `[1]`.

## Colormap Range Utilities

`ADBFile` pre-computes per-case and global Cp statistics during the initial
scan pass so you don't need to load every case yourself:

```python
# Global outlier-robust range -- best choice for sweep animations.
lo, hi = adb.global_cp_auto_range()

# Global tight range (actual data min/max, no outlier trimming).
lo, hi = adb.global_cp_data_bounds()

# Per-case auto range (1-based list).
lo, hi = adb.CaseCpAutoRange[1]
```

The auto range uses a mean +/- 1 standard deviation heuristic on the surface
Cp values, matching the GL_VIEWER default display range in the VSPAERO C++
viewer.

## Visualization

The `pyvspaero.adb_plot` module (install with `pip install 'pyvspaero[plot]'`)
provides `ADBPlotter` for building pyvista scenes:

```python
from pyvspaero import ADBFile
from pyvspaero.adb_plot import ADBPlotter

adb  = ADBFile('Wing.adb')
geom, soln = adb.LoadCase(1)
clim = adb.global_cp_auto_range()

p = ADBPlotter(adb.Header, geom, soln, clim=clim)
p.add_surface(scalar='Cp')
p.add_mesh_edges()
p.add_wake(mode='tubes')
p.view_isometric()
p.show()               # interactive pyvista window
p.screenshot('out.png')
```

## Examples

The `examples/` directory contains eight ready-to-run scripts:

| Script | Description |
|--------|-------------|
| `view_case.py` | Quick-look interactive viewer (opens a live window by default) |
| `cp_plot.py` | Cp surface + mesh overlay, isometric and top view |
| `wake_plot.py` | Cp surface + wake filament overlay |
| `wake_modes.py` | 2x2 comparison of all four wake rendering styles |
| `sweep_plots.py` | One Cp PNG per case with a shared colormap |
| `animate_sweep.py` | Animation frame sequence for all cases |
| `mesh_view.py` | Multi-panel comparison of all mesh levels |
| `quad_plot.py` | Cp surface + adaptive quadtree cutting-plane overlay |

Run any script with `--help` for full argument details, or see
`examples/README.md` for descriptions and usage examples.
