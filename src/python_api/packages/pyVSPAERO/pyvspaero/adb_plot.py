# Copyright (c) 2026 Rob McDonald

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""
Visualization tools for VSPAERO *.adb solution data.

This module provides the ADBPlotter class and the animate() function for
creating publication-quality plots and animation frames from VSPAERO output.
The primary rendering back-end is pyvista (VTK), which supports headless
off-screen rendering -- no display server is required.

Primary API::

    from pyvspaero import ADBFile
    from pyvspaero.adb_plot import ADBPlotter

    adb = ADBFile('mycase.adb')
    geom, soln = adb.LoadCase(1)

    (ADBPlotter(geom, soln, adb.Header)
        .add_surface(scalar='Cp')
        .add_mesh_edges(level=1)
        .add_wake()
        .set_view('iso')
        .save('cp_iso.png'))

Convenience factory::

    ADBPlotter.from_file(adb, case=1).add_surface().save('out.png')

Animation::

    from pyvspaero.adb_plot import animate
    animate(adb, output_dir='frames/', scalar='Cp', clim=[-2.0, 0.5])

    # Assemble with ffmpeg:
    # ffmpeg -r 24 -i frames/frame_%04d.png -c:v libx264 output.mp4

Mesh hierarchy
--------------
VSPAERO maintains two mesh representations in the *.adb file:

  Fine (input) mesh
      Triangulated surface stored in ADBGeometry.TriList* and NodeList*.
      Used to visualize surface scalar fields (Cp, Gamma, ...).

  Coarse mesh levels (CoarseMeshLevel objects in ADBGeometry)
      Each level stores the node coordinates and edges of the n-gon
      computational mesh at one agglomeration level.  Level 1 is the
      primary MG (computational) mesh; higher levels are coarser.
      Used to draw the computational mesh outline on top of the triangle
      surface as a wireframe overlay.

Colormap convention
-------------------
The default colormap CMAP_VIEWER ('RdBu_r') matches the blue-white-red
diverging scale used by the VSPAERO Viewer application.  Blue represents
low (negative) values; red represents high (positive) values.  This makes
side-by-side comparison and validation against the Viewer straightforward.
Pass any matplotlib colormap name as the `cmap` argument to override.
"""

import os

import numpy as np

try:
    import pyvista as pv
except ImportError as exc:
    raise ImportError(
        "pyvista is required for visualization.  "
        "Install it with:  pip install 'pyvspaero[plot]'"
    ) from exc

from .adb_data import ADBGeometry, ADBSolution, ADBHeader
from .adb_format import PANEL_MODEL


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

# Default colormap -- matches the VSPAERO Viewer blue-white-red diverging scale.
# Blue = low (negative Cp = suction); red = high (positive Cp = stagnation).
CMAP_VIEWER = 'RdBu_r'

# Scalar fields available on the triangle (fine) mesh.
_TRI_SCALARS = {'Cp', 'CpSteady', 'CpUnsteady', 'Gamma'}

# Default window resolution for saved images.
_DEFAULT_WINDOW_SIZE = (1920, 1080)


# ---------------------------------------------------------------------------
# View-direction look-up table (shared by ADBPlotter.set_view and plot_wake_modes)
# ---------------------------------------------------------------------------
#
# Each entry maps a view name to (view_vector, viewup).  view_vector points
# FROM the scene TOWARD the camera (pyvista convention for view_vector()).
# Axis convention: +Y nose-to-tail, +X starboard, +Z up (VSPAERO static-plot
# convention; the desktop viewer uses the OpenVSP convention +X nose-to-tail).
_VIEW_VECTORS = {
    'top':    (( 0,  0, -1), ( 1, 0, 0)),
    'bottom': (( 0,  0,  1), ( 1, 0, 0)),
    'front':  (( 0, -1,  0), ( 0, 0, 1)),
    'back':   (( 0,  1,  0), ( 0, 0, 1)),
    'left':   ((-1,  0,  0), ( 0, 0, 1)),
    'right':  (( 1,  0,  0), ( 0, 0, 1)),
}

# ---------------------------------------------------------------------------
# Private scalar-range helper
# ---------------------------------------------------------------------------

def _scalar_auto_range(arr):
    """Compute a robust default color range from a scalar array.

    Uses mean +/- 1 standard deviation, clamped to the actual data range.
    Mirrors GL_VIEWER::FindSolutionMinMax() in the C++ Viewer: the +/- 1 sigma window
    trims statistical outliers (isolated high-stagnation or spurious Cp > 1
    triangles) so the colormap captures the majority of the meaningful data
    variation rather than being dominated by extreme values.

    Args:
        arr (np.ndarray): 1-D float array of scalar values (surface tris only).

    Returns:
        tuple: (lo, hi) color range.
    """
    avg = float(arr.mean())
    std = float(arr.std())
    return (max(avg - std, float(arr.min())),
            min(avg + std, float(arr.max())))


# ---------------------------------------------------------------------------
# Private mesh-builder helpers
# ---------------------------------------------------------------------------

def _build_surface_mesh(geom):
    """Build a pyvista PolyData for the wing surface triangles only.

    The TriList in a VSPAERO ADB file contains two categories of triangles:
      - surface_type > 0 : wing / body surface triangles (what we want)
      - surface_type == 0: wake sheet triangles (extend far downstream; excluded)

    Only surface triangles are included.  Because the full NodeList also
    contains wake nodes, the included nodes are compacted and the face
    connectivity re-indexed to match.

    Args:
        geom (ADBGeometry): Geometry for one case.

    Returns:
        tuple: (pv.PolyData, t_idx) where t_idx is a 1-based int64 array of
               the TriList indices that were included.  Use t_idx to look up
               the corresponding per-triangle scalars in ADBSolution.
    """
    n_tris = geom.NumberOfTris

    # 1-based indices of surface-only triangles (surface_type > 0)
    surf_mask = geom.TriList_surface_type[1:n_tris + 1] > 0
    t_idx = np.where(surf_mask)[0].astype(np.int64) + 1   # 1-based

    n1_raw = geom.TriList_node1[t_idx]
    n2_raw = geom.TriList_node2[t_idx]
    n3_raw = geom.TriList_node3[t_idx]

    # Compact the node set to only those referenced by surface triangles
    used = np.unique(np.concatenate([n1_raw, n2_raw, n3_raw]))

    points = np.column_stack([
        geom.NodeList_x[used].astype(np.float64),
        geom.NodeList_y[used].astype(np.float64),
        geom.NodeList_z[used].astype(np.float64),
    ])

    # Mapping: 1-based global node index -> 0-based local index in `points`
    node_map = np.zeros(geom.NumberOfNodes + 1, dtype=np.int64)
    node_map[used] = np.arange(len(used), dtype=np.int64)

    n_surf = len(t_idx)
    faces = np.column_stack([
        np.full(n_surf, 3, dtype=np.int64),
        node_map[n1_raw],
        node_map[n2_raw],
        node_map[n3_raw],
    ]).ravel()

    return pv.PolyData(points, faces), t_idx


def _build_surface_meshes(geom):
    """Build one pyvista PolyData per VSPAERO surface (ComponentID).

    VSPAERO stores the surface/component index for each triangle in
    TriList_surface_type (0 = wake sheet; > 0 = surface, value is the
    1-based ComponentID).  Within a single surface the scalar data
    (Cp for thick/panel bodies, deltaCp for thin/VLM wings) is single-
    valued at shared nodes, so smooth Gouraud shading is physically
    meaningful.  Across surface boundaries the interpretation of the
    scalar is different (Cp vs deltaCp at a body-wing junction, or
    simply discontinuous at a hard edge), so cross-surface node averaging
    would corrupt the result.

    Splitting by ComponentID and building a compact node set per surface
    ensures that nodes at surface boundaries are not shared between meshes,
    so smooth shading averages within each surface only.

    Args:
        geom (ADBGeometry): Geometry for one case.

    Returns:
        list of tuples: Each element is ``(pv.PolyData, t_idx, component_id)``
            where ``t_idx`` is a 1-based int64 array of the TriList indices
            belonging to that surface and ``component_id`` is the integer
            ComponentID.  Sorted by component_id ascending.
    """
    n_tris = geom.NumberOfTris

    surf_type_arr = np.array(geom.TriList_surface_type[1:n_tris + 1], dtype=np.int32)

    # Unique component IDs for surface triangles only (surface_type > 0)
    unique_ids = np.unique(surf_type_arr[surf_type_arr > 0])

    result = []
    for cid in unique_ids:
        tri_mask = (surf_type_arr == cid)
        t_idx = np.where(tri_mask)[0].astype(np.int64) + 1   # 1-based

        n1_raw = geom.TriList_node1[t_idx]
        n2_raw = geom.TriList_node2[t_idx]
        n3_raw = geom.TriList_node3[t_idx]

        # Compact the node set for this surface only.  Nodes at the boundary
        # between two surfaces are independent points in each mesh so they are
        # never averaged together during smooth shading.
        used = np.unique(np.concatenate([n1_raw, n2_raw, n3_raw]))

        points = np.column_stack([
            geom.NodeList_x[used].astype(np.float64),
            geom.NodeList_y[used].astype(np.float64),
            geom.NodeList_z[used].astype(np.float64),
        ])

        node_map = np.zeros(geom.NumberOfNodes + 1, dtype=np.int64)
        node_map[used] = np.arange(len(used), dtype=np.int64)

        n_surf = len(t_idx)
        faces = np.column_stack([
            np.full(n_surf, 3, dtype=np.int64),
            node_map[n1_raw],
            node_map[n2_raw],
            node_map[n3_raw],
        ]).ravel()

        result.append((pv.PolyData(points, faces), t_idx, int(cid)))

    return result


def _build_mesh_edges(level, surface_only=True, visible_ids=None):
    """Build a pyvista PolyData containing the n-gon edges for one mesh level.

    The CoarseMeshLevel EdgeList contains both wing-surface edges
    (EdgeList_SurfaceID > 0) and wake edges (EdgeList_SurfaceID == 0).
    By default only surface edges are included, matching the VSPAERO Viewer
    behaviour of drawing the computational mesh on the wing.

    Because only a subset of edges (and therefore nodes) may be included,
    the node set is compacted and connectivity re-indexed.

    Args:
        level (CoarseMeshLevel): One entry from ADBGeometry.CoarseMeshLevels.
        surface_only (bool): If True (default), include only edges whose
            SurfaceID > 0, excluding wake edges.
        visible_ids (set or None): If provided, further restrict to edges
            whose SurfaceID is in this set (component visibility filter).

    Returns:
        pv.PolyData: Line mesh (edges only).
    """
    n_ce = level.NumberOfEdges
    edge_sids = level.EdgeList_SurfaceID[1:n_ce + 1]

    if surface_only:
        edge_mask = edge_sids > 0
    else:
        edge_mask = np.ones(n_ce, dtype=bool)

    if visible_ids is not None:
        edge_mask &= np.isin(edge_sids, list(visible_ids))

    e_idx = np.where(edge_mask)[0] + 1   # 1-based

    if len(e_idx) == 0:
        return pv.PolyData()

    en1_raw = level.EdgeList_node1[e_idx]
    en2_raw = level.EdgeList_node2[e_idx]

    # Compact nodes to those referenced by the selected edges
    used = np.unique(np.concatenate([en1_raw, en2_raw]))

    points = np.column_stack([
        level.NodeList_x[used].astype(np.float64),
        level.NodeList_y[used].astype(np.float64),
        level.NodeList_z[used].astype(np.float64),
    ])

    node_map = np.zeros(level.NumberOfNodes + 1, dtype=np.int64)
    node_map[used] = np.arange(len(used), dtype=np.int64)

    n_edges = len(e_idx)
    lines = np.column_stack([
        np.full(n_edges, 2, dtype=np.int64),
        node_map[en1_raw],
        node_map[en2_raw],
    ]).ravel()

    return pv.PolyData(points, lines=lines)


def _build_wake(soln, extend_to_infinity=False, filament_ids=None):
    """Build a pyvista PolyData containing all wake filament polylines.

    VSPAERO appends one extra "infinity" node to each filament that extends
    the wake to a far-field point (typically x ~ 10 000 chord lengths away).
    By default that final node is dropped so the rendered wake stays in the
    physical neighbourhood of the wing.  Pass extend_to_infinity=True to
    include it.

    Args:
        soln (ADBSolution): Solution for one case.
        extend_to_infinity (bool): If False (default), omit the last node of
            each filament (the far-field extension point).  If True, include
            all nodes.
        filament_ids (set or None): If provided, only include filaments whose
            1-based index is in this set (component visibility filter).

    Returns:
        pv.PolyData or None: Polyline mesh (one cell per filament),
                             or None if there are no wake filaments.
    """
    n_filaments = soln.NumberOfTrailingVortexEdges
    if n_filaments == 0:
        return None

    all_points = []
    all_lines  = []
    offset = 0

    for i in range(1, n_filaments + 1):
        if filament_ids is not None and i not in filament_ids:
            continue
        wf = soln.WakeFilaments[i]
        n  = wf.NumberOfNodes

        # Drop the last (infinity) node unless the caller wants it.
        n_use = n if extend_to_infinity else n - 1

        # Skip degenerate filaments (body wake stubs) that have fewer than
        # 2 nodes after truncation -- they cannot form a line segment.
        if n_use < 2:
            continue

        pts = np.column_stack([
            wf.x[1:n_use + 1],
            wf.y[1:n_use + 1],
            wf.z[1:n_use + 1],
        ])
        all_points.append(pts)

        # Polyline cell: [n_use, global_idx_0, ..., global_idx_{n_use-1}]
        all_lines.append(
            np.concatenate([[n_use], np.arange(offset, offset + n_use)])
        )
        offset += n_use

    if not all_points:
        return None

    wake_points = np.vstack(all_points)
    wake_lines  = np.concatenate(all_lines).astype(np.int64)

    return pv.PolyData(wake_points, lines=wake_lines)


def _build_wake_surface(geom, extend_to_infinity=False):
    """Build a pyvista PolyData for the wake sheet triangles.

    The VSPAERO TriList contains two kinds of triangles: surface triangles
    (TriList_surface_type > 0) and wake sheet triangles (surface_type == 0).
    This function builds a mesh from the wake sheet triangles, which form
    the planar vortex-sheet surface trailing downstream from lifting edges.

    Like the filament wake, VSPAERO extends each wake sheet column to a
    far-field "infinity" point (x ~ 10 000 chord lengths).  By default those
    triangles are removed so the rendered sheet stays in the physical
    neighbourhood of the wing.  The far-field triangles are identified
    robustly as any triangle containing a node whose x coordinate is an
    extreme outlier (> Q3 + 100 x IQR of the near-wake x distribution).

    Args:
        geom (ADBGeometry): Geometry for one case.
        extend_to_infinity (bool): If False (default), exclude far-field
            extension triangles.  If True, include them.

    Returns:
        pv.PolyData or None: Triangle mesh of the wake sheet, or None if
            no wake triangles are present.
    """
    n_tris = geom.NumberOfTris

    surf_type = np.asarray(geom.TriList_surface_type[1:n_tris + 1], dtype=np.int32)
    wake_mask = surf_type == 0
    t_idx = np.where(wake_mask)[0].astype(np.int64) + 1   # 1-based

    if len(t_idx) == 0:
        return None

    n1_raw = geom.TriList_node1[t_idx]
    n2_raw = geom.TriList_node2[t_idx]
    n3_raw = geom.TriList_node3[t_idx]

    if not extend_to_infinity:
        # Identify far-field extension nodes as extreme x outliers.
        # Near-wake x values are O(chord); far-field is O(10 000 chord).
        # Using Q3 + 100 x IQR gives a robust, geometry-scale-free cutoff.
        all_nodes = np.unique(np.concatenate([n1_raw, n2_raw, n3_raw]))
        node_x = np.asarray(geom.NodeList_x, dtype=np.float64)[all_nodes]
        q25, q75 = np.percentile(node_x, [25, 75])
        iqr = max(q75 - q25, 1e-9)
        x_cut = q75 + 100.0 * iqr
        far = set(all_nodes[node_x > x_cut].tolist())
        if far:
            keep = ~(np.isin(n1_raw, list(far)) |
                     np.isin(n2_raw, list(far)) |
                     np.isin(n3_raw, list(far)))
            t_idx  = t_idx[keep]
            n1_raw = n1_raw[keep]
            n2_raw = n2_raw[keep]
            n3_raw = n3_raw[keep]

    if len(t_idx) == 0:
        return None

    used = np.unique(np.concatenate([n1_raw, n2_raw, n3_raw]))

    points = np.column_stack([
        np.asarray(geom.NodeList_x, dtype=np.float64)[used],
        np.asarray(geom.NodeList_y, dtype=np.float64)[used],
        np.asarray(geom.NodeList_z, dtype=np.float64)[used],
    ])

    node_map = np.zeros(geom.NumberOfNodes + 1, dtype=np.int64)
    node_map[used] = np.arange(len(used), dtype=np.int64)

    n_wake = len(t_idx)
    faces = np.column_stack([
        np.full(n_wake, 3, dtype=np.int64),
        node_map[n1_raw],
        node_map[n2_raw],
        node_map[n3_raw],
    ]).ravel()

    return pv.PolyData(points, faces)


def _circle_polydata(center, normal, radius, n_pts=64):
    """Build a pyvista PolyData closed polyline representing a circle.

    Used to visualize rotor disks and nozzle faces.

    Args:
        center (array-like): Shape (3,) center point.
        normal (array-like): Shape (3,) axis normal (disk face direction).
        radius (float): Circle radius.
        n_pts (int): Number of points around the circumference.

    Returns:
        pv.PolyData: Closed polyline.
    """
    center = np.asarray(center, dtype=np.float64)
    normal = np.asarray(normal, dtype=np.float64)
    normal = normal / np.linalg.norm(normal)

    # Two orthogonal vectors in the disk plane
    ref = np.array([0.0, 0.0, 1.0]) if abs(normal[2]) < 0.9 else np.array([1.0, 0.0, 0.0])
    perp1 = np.cross(normal, ref)
    perp1 /= np.linalg.norm(perp1)
    perp2 = np.cross(normal, perp1)

    angles = np.linspace(0.0, 2.0 * np.pi, n_pts, endpoint=False)
    pts = (center
           + radius * np.cos(angles)[:, None] * perp1
           + radius * np.sin(angles)[:, None] * perp2)

    # Closed polyline: connect back to first point
    lines = np.concatenate([
        [n_pts + 1],
        np.arange(n_pts),
        [0],
    ]).astype(np.int64)

    return pv.PolyData(pts, lines=lines)


def _build_disk(center, normal, outer_radius, inner_radius=0.0, n_pts=64):
    """Build a pyvista PolyData filled disk or annulus.

    For *inner_radius* == 0 a triangle fan from the center to the
    circumference is returned.  For *inner_radius* > 0 a quad strip
    connecting the inner and outer circles is returned (annulus).

    Args:
        center (array-like): Shape (3,) center point.
        normal (array-like): Shape (3,) axis normal (disk face direction).
        outer_radius (float): Outer circle radius.
        inner_radius (float): Inner circle radius (0 for a solid disk).
        n_pts (int): Number of points around each circle.

    Returns:
        pv.PolyData: Filled surface mesh.
    """
    center = np.asarray(center, dtype=np.float64)
    normal = np.asarray(normal, dtype=np.float64)
    normal = normal / np.linalg.norm(normal)

    ref   = np.array([0.0, 0.0, 1.0]) if abs(normal[2]) < 0.9 else np.array([1.0, 0.0, 0.0])
    perp1 = np.cross(normal, ref)
    perp1 /= np.linalg.norm(perp1)
    perp2 = np.cross(normal, perp1)

    angles = np.linspace(0.0, 2.0 * np.pi, n_pts, endpoint=False)
    cos_a  = np.cos(angles)
    sin_a  = np.sin(angles)

    outer_pts = (center
                 + outer_radius * cos_a[:, None] * perp1
                 + outer_radius * sin_a[:, None] * perp2)

    if inner_radius <= 0.0:
        # Triangle fan: center point (index 0) + outer ring (indices 1..n_pts)
        pts   = np.vstack([center[None, :], outer_pts])
        faces = []
        for i in range(n_pts):
            i1 = i + 1
            i2 = (i + 1) % n_pts + 1
            faces.extend([3, 0, i1, i2])
        return pv.PolyData(pts, np.array(faces, dtype=np.int64))
    else:
        # Quad strip: outer ring (0..n_pts-1), inner ring (n_pts..2*n_pts-1)
        inner_pts = (center
                     + inner_radius * cos_a[:, None] * perp1
                     + inner_radius * sin_a[:, None] * perp2)
        pts   = np.vstack([outer_pts, inner_pts])
        faces = []
        for i in range(n_pts):
            i_next = (i + 1) % n_pts
            o0, o1 = i,        i_next
            in0    = i + n_pts
            in1    = i_next + n_pts
            faces.extend([4, o0, o1, in1, in0])
        return pv.PolyData(pts, np.array(faces, dtype=np.int64))


def _reflect_polydata(mesh):
    """Reflect a pyvista PolyData about the XZ plane (Y -> -Y).

    Args:
        mesh (pv.PolyData): Mesh to reflect.

    Returns:
        pv.PolyData: Reflected copy.
    """
    reflected = mesh.copy()
    pts = reflected.points.copy()
    pts[:, 1] = -pts[:, 1]
    reflected.points = pts
    return reflected


def _build_quad_mesh(plane):
    """Build a pyvista PolyData quad mesh from a QuadSlicePlane.

    The returned mesh carries per-node (point) data arrays for all scalar
    and vector fields stored on the plane:

    - ``'Cp'``       -- pressure coefficient
    - ``'Velocity'`` -- velocity vectors (u, v, w), shape (N, 3)
    - ``'Vmag'``     -- velocity magnitude

    Args:
        plane (QuadSlicePlane): Parsed quad cutting-plane data.

    Returns:
        pv.PolyData: Quad mesh with point data arrays.
    """
    n = plane.NumberOfNodes
    m = plane.NumberOfCells

    # Points array -- 0-based (strip the unused [0] padding)
    pts = plane.xyz[1:].astype(np.float32)          # (n, 3)

    # Build VTK face array: [4, n0, n1, n2, n3, 4, ...] -- 0-based indices
    c = plane.cells[1:].astype(np.int64) - 1        # (m, 4), 0-based
    faces = np.empty(m * 5, dtype=np.int64)
    faces[0::5] = 4
    faces[1::5] = c[:, 0]
    faces[2::5] = c[:, 1]
    faces[3::5] = c[:, 2]
    faces[4::5] = c[:, 3]

    mesh = pv.PolyData(pts, faces)

    # Attach point data (strip padding row [0])
    mesh['Cp']       = plane.Cp[1:].astype(np.float32)
    vel              = plane.velocity[1:].astype(np.float32)
    mesh['Velocity'] = vel
    mesh['Vmag']     = np.linalg.norm(vel, axis=1).astype(np.float32)

    return mesh


def _add_propulsion_elements_to_plotter(plotter, geom, rotor_color='blue',
                                        nozzle_color='red', line_width=1.5,
                                        reflect=False):
    """Render propulsion element disks and outlines onto an existing pv.Plotter.

    Each element is drawn as a semi-transparent filled disk (or annulus if
    hub_radius > 0) with a black circumference outline.  For rotors with a
    non-zero hub radius an inner-circle outline is also added.

    This module-level helper is called by :meth:`ADBPlotter.add_propulsion_elements`
    and by the standalone :func:`plot_wake_modes` and :func:`animate` functions.

    Args:
        plotter (pv.Plotter): Plotter to add actors to.
        geom (ADBGeometry): Geometry object containing propulsion element data.
        rotor_color (str): Fill color for rotor disks.
        nozzle_color (str): Fill color for engine nozzle faces.
        line_width (float): Line width for circumference outlines.
        reflect (bool): If True, also add Y-reflected copies.
    """
    def _add_one(xyz, normal, radius, hub_radius, elem_type):
        color = rotor_color if elem_type == 'PROP_ROTOR' else nozzle_color
        inner = hub_radius if elem_type == 'PROP_ROTOR' else 0.0
        plotter.add_mesh(_build_disk(xyz, normal, radius, inner_radius=inner),
                         color=color, opacity=0.3, show_edges=False,
                         show_scalar_bar=False)
        plotter.add_mesh(_circle_polydata(xyz, normal, radius),
                         color='black', line_width=line_width, style='wireframe')
        if elem_type == 'PROP_ROTOR' and hub_radius > 0.0:
            plotter.add_mesh(_circle_polydata(xyz, normal, hub_radius),
                             color='black', line_width=line_width,
                             style='wireframe')

    for j in range(1, geom.NumberOfPropulsionElements + 1):
        elem = geom.PropulsionElementList[j]
        _add_one(elem.xyz, elem.normal, elem.radius, elem.hub_radius, elem.Type)
        if reflect:
            r_xyz        = elem.xyz.copy()
            r_xyz[1]     = -r_xyz[1]
            r_normal     = elem.normal.copy()
            r_normal[1]  = -r_normal[1]
            _add_one(r_xyz, r_normal, elem.radius, elem.hub_radius, elem.Type)


# ---------------------------------------------------------------------------
# ADBPlotter
# ---------------------------------------------------------------------------

class ADBPlotter:
    """Builds a pyvista scene from VSPAERO ADB data and renders it to PNG.

    Designed for headless (off-screen) rendering -- no display server required.
    Pass ``off_screen=False`` to the constructor for interactive viewing via
    show().

    Usage (method chaining)::

        plotter = ADBPlotter(geom, soln, header)
        (plotter
            .add_surface(scalar='Cp')
            .add_mesh_edges(level=1)
            .add_wake()
            .set_view('iso')
            .save('output.png'))

    Args:
        geom (ADBGeometry): Geometry for the case to visualize.
        soln (ADBSolution): Solution for the case to visualize.
        header (ADBHeader): File header (reference quantities, symmetry flag, ...).
        off_screen (bool): True (default) for headless rendering; False for
            interactive display via show().
        window_size (tuple): Pixel resolution (width, height).
        background_color (str): Background color.  Defaults to 'white'.
    """

    def __init__(self, geom, soln, header,
                 off_screen=True,
                 window_size=_DEFAULT_WINDOW_SIZE,
                 background_color='white'):
        self._geom   = geom
        self._soln   = soln
        self._header = header

        self._plotter = pv.Plotter(
            off_screen  = off_screen,
            window_size = list(window_size),
        )
        self._plotter.background_color = background_color

        # Combined surface mesh (all surfaces merged) -- used only for bounding-box
        # calculations in set_view().  Built on first call to _get_surface_mesh().
        self._surface_mesh    = None
        self._surface_tri_idx = None

        # Per-surface mesh list: [(pv.PolyData, t_idx, component_id), ...]
        # Built on first call to _get_surface_meshes().  Used by add_surface()
        # so that smooth shading averages scalars only within each surface.
        self._surface_meshes_list = None

        # Track the last surface scalar/actor so add_colorbar() can build a
        # colorbar that exactly matches the rendered surface (same mapper /
        # lookup table, so cmap and clim are guaranteed to agree).
        self._last_scalar = None
        self._last_actor  = None

    # ------------------------------------------------------------------
    # Factory
    # ------------------------------------------------------------------

    @classmethod
    def from_file(cls, adb_file, case=1, **kwargs):
        """Convenience factory: load a case from an ADBFile and build a plotter.

        Args:
            adb_file (ADBFile): Opened ADB file.
            case (int): 1-based case number to load.
            **kwargs: Forwarded to ADBPlotter.__init__().

        Returns:
            ADBPlotter
        """
        geom, soln = adb_file.LoadCase(case)
        return cls(geom, soln, adb_file.Header, **kwargs)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _get_surface_mesh(self):
        """Return the cached combined surface mesh (all surfaces), building it on first call.

        This mesh is used only for bounding-box calculations in set_view().
        For rendering, use _get_surface_meshes() which returns a per-surface list.
        """
        if self._surface_mesh is None:
            self._surface_mesh, self._surface_tri_idx = _build_surface_mesh(self._geom)
        return self._surface_mesh

    def _get_surface_meshes(self):
        """Return the cached per-surface mesh list, building it on first call.

        Each entry is (pv.PolyData, t_idx, component_id).  Nodes at surface
        boundaries are not shared between meshes, so smooth shading cannot
        average scalars across surface junctions.

        As a side effect, also ensures the combined surface mesh (used by
        set_view() for bounds) is built.
        """
        if self._surface_meshes_list is None:
            self._surface_meshes_list = _build_surface_meshes(self._geom)
            # Ensure the combined mesh is also ready for set_view() bounds.
            if self._surface_mesh is None:
                self._surface_mesh, self._surface_tri_idx = _build_surface_mesh(self._geom)
        return self._surface_meshes_list

    # ------------------------------------------------------------------
    # Scene-building methods
    # ------------------------------------------------------------------

    def add_surface(self, scalar='Cp', style='flat', cmap=None,
                    clim=None, opacity=1.0, reflect=False):
        """Add the triangulated surface mesh colored by a scalar field.

        Uses the fine (input) triangle mesh from TriList/NodeList.  Each
        VSPAERO surface (ComponentID) is rendered as a separate actor so that
        smooth shading averages scalar values only within each surface.  This
        matters at surface junctions -- e.g. a wing-body junction where the body
        carries Cp and the thin wing carries deltaCp -- where the scalar is
        physically discontinuous and cross-surface averaging would be wrong.

        For the computational mesh outline, call add_mesh_edges() separately.

        Args:
            scalar (str): Scalar field name.  One of:
                'Cp'          -- total pressure coefficient (steady + unsteady)
                'CpSteady'    -- steady component
                'CpUnsteady'  -- unsteady component
                'Gamma'       -- circulation (interpolated to fine mesh)
            style (str): Rendering style.  One of:
                'flat'        -- flat (per-face) shading, no edge lines (default).
                                Each triangle is a single color; accurately
                                represents per-face scalar data and avoids
                                vertex-averaging artifacts on PANEL meshes.
                'smooth'      -- smooth (Gouraud) shading, no edge lines.
                                Averages scalars at shared nodes within each
                                surface.  Because each surface is rendered
                                independently, nodes at surface boundaries are
                                never averaged across surfaces.
                'wireframe'   -- wireframe only
                'transparent' -- flat shading with opacity=0.3
            cmap (str or None): Matplotlib colormap name.  None uses
                CMAP_VIEWER ('RdBu_r'), which matches the VSPAERO Viewer scale.
            clim (tuple or None): (min, max) color limits.  None auto-ranges
                to [data_min, data_max] across all surfaces for this case.
            opacity (float): Surface opacity (0 - 1).  Overridden to 0.3 when
                style='transparent'.
            reflect (bool): If True, also add the Y-reflected mirror image
                (for half-model / symmetry cases).

        Returns:
            ADBPlotter: self (for method chaining).
        """
        if scalar not in _TRI_SCALARS:
            raise ValueError(
                f'scalar={scalar!r} is not a triangle-mesh scalar.  '
                f'Valid choices: {sorted(_TRI_SCALARS)}'
            )

        surface_meshes = self._get_surface_meshes()

        if clim is None:
            # Compute auto clim using mean +/- 1 sigma across all surface triangles.
            # This matches GL_VIEWER::FindSolutionMinMax() in the C++ Viewer
            # and trims outliers (e.g. spurious Cp > 1 stagnation spikes) so
            # the majority of the colormap captures meaningful data variation.
            all_scalars = np.concatenate([
                getattr(self._soln, scalar)[t_idx].astype(np.float64)
                for _, t_idx, _ in surface_meshes
            ])
            lo, hi = _scalar_auto_range(all_scalars)
            # Thick (PANEL) bodies always have a stagnation point where
            # Cp = 1.0.  Ensure the display range always includes +1 so
            # the stagnation region is never clipped below the colormap.
            if self._header.ModelType == PANEL_MODEL:
                hi = max(hi, 1.0)
            clim = (lo, hi)

        # Guard against degenerate range (min == max).
        # pyvista produces a blank / invisible surface when clim[0] == clim[1].
        if clim[0] == clim[1]:
            delta = max(abs(clim[0]) * 0.05, 0.1)
            clim = (clim[0] - delta, clim[1] + delta)

        if cmap is None:
            cmap = CMAP_VIEWER

        smooth_shading = False   # flat by default -- preserves per-face scalar accuracy
        use_wireframe  = False

        if style == 'smooth':
            smooth_shading = True
        elif style == 'wireframe':
            use_wireframe = True
        elif style == 'transparent':
            opacity = 0.3

        add_kwargs = dict(
            scalars         = scalar,
            clim            = list(clim),
            cmap            = cmap,
            smooth_shading  = smooth_shading,
            show_edges      = False,
            opacity         = opacity,
            show_scalar_bar = False,   # controlled by add_colorbar()
        )
        if use_wireframe:
            add_kwargs['style'] = 'wireframe'

        # Render each surface as a separate actor so pyvista's cell->point
        # data conversion (for smooth shading) is confined within each surface.
        for mesh, t_idx, _ in surface_meshes:
            m = mesh.copy()
            m.cell_data[scalar] = getattr(self._soln, scalar)[t_idx].astype(np.float64)
            self._last_actor = self._plotter.add_mesh(m, **add_kwargs)
            if reflect:
                self._plotter.add_mesh(_reflect_polydata(m), **add_kwargs)

        self._last_scalar = scalar
        return self

    # ------------------------------------------------------------------

    def add_mesh_edges(self, level=1, color='black', line_width=0.5,
                       reflect=False):
        """Add wireframe edges of a coarse mesh level as an overlay.

        The coarse mesh levels store the n-gon edges of the computational
        mesh at various agglomeration levels.  Level 1 is the primary MG
        (computational) mesh; higher levels are coarser.

        Typical use: call add_surface() first, then add_mesh_edges() to
        overlay the computational mesh structure on the smooth scalar field.

        Args:
            level (int): 1-based coarse mesh level index.
            color (str): Edge color.
            line_width (float): Edge line width in screen pixels.
            reflect (bool): If True, also add the Y-reflected mirror.

        Returns:
            ADBPlotter: self (for method chaining).
        """
        if level < 1 or level > self._geom.NumberOfMeshLevels:
            raise IndexError(
                f'level={level} out of range; file has '
                f'{self._geom.NumberOfMeshLevels} coarse mesh level(s).'
            )

        edge_mesh = _build_mesh_edges(self._geom.CoarseMeshLevels[level],
                                      surface_only=True)

        edge_kwargs = dict(
            color      = color,
            line_width = line_width,
            style      = 'wireframe',
        )
        def _add_edges(mesh):
            actor = self._plotter.add_mesh(mesh, **edge_kwargs)
            # Shift lines forward in the depth buffer so they render cleanly
            # on top of the coincident surface without z-fighting artifacts.
            # Mirrors glPolygonOffset / vtkMapper::SetResolveCoincidentTopology.
            actor.mapper.SetResolveCoincidentTopologyToPolygonOffset()

        _add_edges(edge_mesh)
        if reflect:
            _add_edges(_reflect_polydata(edge_mesh))

        return self

    # ------------------------------------------------------------------

    def add_wake(self, style='lines', color='gray', line_width=1.0,
                 tube_radius=None, opacity=0.4, reflect=False,
                 extend_to_infinity=False):
        """Add trailing wake geometry.

        Four rendering styles are available, from lightest to heaviest:
        'lines' and 'tubes' use the vortex filament polylines; 'points'
        renders individual filament nodes; 'surface' renders the wake sheet
        triangles from the TriList (TriList_surface_type == 0).

        VSPAERO appends one extra far-field "infinity" node/column to each
        filament / wake sheet column (typically x ~ 10 000 chord lengths).
        By default that extension is omitted so the rendered wake stays in the
        physical neighbourhood of the wing.

        Args:
            style (str): Rendering style.  One of:
                'lines'   -- polylines along vortex filaments (fast, default)
                'tubes'   -- tubes swept along the filaments (publication quality)
                'points'  -- individual filament nodes as spheres
                'surface' -- wake sheet triangle mesh (shows the vortex sheet
                            as a semi-transparent surface)
            color (str): Color for wake geometry.
            line_width (float): Line width in screen pixels (style='lines').
            tube_radius (float or None): Tube radius (style='tubes').
                None auto-sizes to 0.2 % of the surface bounding-box diagonal.
            opacity (float): Opacity for style='surface'.  Default 0.4.
            reflect (bool): If True, also add the Y-reflected wake.
            extend_to_infinity (bool): If True, include the far-field
                extension node/column at the end of each filament.  Default False.

        Returns:
            ADBPlotter: self (for method chaining).
        """
        if style == 'surface':
            wake_mesh = _build_wake_surface(self._geom,
                                            extend_to_infinity=extend_to_infinity)
        else:
            wake_mesh = _build_wake(self._soln, extend_to_infinity=extend_to_infinity)

        if wake_mesh is None:
            return self

        def _add_one(wmesh):
            if style == 'tubes':
                r = tube_radius
                if r is None:
                    bbox = self._get_surface_mesh().bounds
                    diag = np.sqrt(
                        (bbox[1] - bbox[0]) ** 2 +
                        (bbox[3] - bbox[2]) ** 2 +
                        (bbox[5] - bbox[4]) ** 2
                    )
                    r = 0.002 * diag
                self._plotter.add_mesh(wmesh.tube(radius=r), color=color)
            elif style == 'points':
                self._plotter.add_mesh(
                    wmesh,
                    color                    = color,
                    point_size               = 3,
                    render_points_as_spheres = True,
                    style                    = 'points',
                )
            elif style == 'surface':
                self._plotter.add_mesh(
                    wmesh,
                    color      = color,
                    opacity    = opacity,
                    show_edges = False,
                )
            else:  # 'lines' (default)
                self._plotter.add_mesh(
                    wmesh,
                    color      = color,
                    line_width = line_width,
                    style      = 'wireframe',
                )

        _add_one(wake_mesh)
        if reflect:
            _add_one(_reflect_polydata(wake_mesh))

        return self

    # ------------------------------------------------------------------

    def add_propulsion_elements(self, rotor_color='blue', nozzle_color='red',
                                line_width=1.5, reflect=False):
        """Add rotor disk and engine nozzle visualizations.

        Each element is drawn as a semi-transparent filled disk (or annulus
        if hub_radius > 0) with a black circumference outline.  Rotors with
        a non-zero hub radius also receive an inner-circle outline.

        Args:
            rotor_color (str): Fill color for rotor disks.
            nozzle_color (str): Fill color for engine nozzle faces.
            line_width (float): Line width for circumference outlines.
            reflect (bool): If True, also add Y-reflected copies.

        Returns:
            ADBPlotter: self (for method chaining).
        """
        _add_propulsion_elements_to_plotter(
            self._plotter, self._geom,
            rotor_color=rotor_color, nozzle_color=nozzle_color,
            line_width=line_width, reflect=reflect,
        )
        return self

    # ------------------------------------------------------------------

    def add_quad_slices(self, quad_planes, scalar='Cp', cmap=None, clim=None,
                        opacity=1.0, show_edges=False, edge_color='black',
                        edge_width=1.5, reflect=False):
        """Add quad cutting-plane flow-field slices to the scene.

        Renders each plane as a filled quad mesh colored by *scalar*.  The
        mesh is the adaptive quadtree grid computed by VSPAERO; cell size
        varies with flow-field gradients.

        When *show_edges* is ``True`` the quadtree cell edges are drawn as a
        separate explicit line mesh (same technique as the mesh-level views)
        to avoid z-fighting artefacts.

        Args:
            quad_planes (list): 1-based list of :class:`~pyvspaero.quad_data.QuadSlicePlane`
                objects as returned by :meth:`~pyvspaero.quad_reader.QuadFile.LoadCase`.
                Index [0] must be ``None`` (the standard pyvspaero 1-based
                convention).  Pass a single-element list for one plane.
            scalar (str): Point-data scalar field to color by.  Available
                fields on every plane mesh:

                - ``'Cp'``   -- pressure coefficient (default)
                - ``'Vmag'`` -- velocity magnitude

            cmap (str or None): Matplotlib colormap name.  ``None`` uses the
                default viewer colormap.
            clim (tuple or None): ``(min, max)`` color-axis limits.  ``None``
                auto-ranges from the combined data across all planes.
            opacity (float): Opacity of the filled quad surface (0-1).
                Default is 1.0 (fully opaque).
            show_edges (bool): If ``True``, draw the quadtree cell edges as
                explicit lines at *edge_width* thickness.  Default ``False``.
            edge_color (str): Color for the cell-edge lines.  Default ``'black'``.
            edge_width (float): Line width for cell edges.  Default ``1.5``.
            reflect (bool): If ``True``, also add Y-reflected copies for
                symmetric half-models.

        Returns:
            ADBPlotter: self (for method chaining).
        """
        if cmap is None:
            cmap = CMAP_VIEWER

        planes = [p for p in quad_planes if p is not None]
        if not planes:
            return self

        meshes = [_build_quad_mesh(p) for p in planes]

        if clim is None:
            all_vals = np.concatenate([m[scalar] for m in meshes])
            clim = _scalar_auto_range(all_vals)
            if clim[0] == clim[1]:
                delta = max(abs(clim[0]) * 0.05, 0.1)
                clim = (clim[0] - delta, clim[1] + delta)

        fill_kwargs = dict(
            scalars         = scalar,
            clim            = list(clim),
            cmap            = cmap,
            opacity         = opacity,
            show_edges      = False,
            show_scalar_bar = False,
        )

        for mesh in meshes:
            self._plotter.add_mesh(mesh.copy(), **fill_kwargs)
            if reflect:
                self._plotter.add_mesh(_reflect_polydata(mesh.copy()),
                                       **fill_kwargs)
            if show_edges:
                edges = mesh.extract_all_edges()
                self._plotter.add_mesh(edges, color=edge_color,
                                       line_width=edge_width)
                if reflect:
                    self._plotter.add_mesh(
                        _reflect_polydata(edges),
                        color=edge_color, line_width=edge_width,
                    )

        return self

    # ------------------------------------------------------------------

    def add_colorbar(self, title=None, position_x=0.87, position_y=0.1,
                     width=0.04, height=0.8, n_labels=5, fmt='%.2f'):
        """Add a scalar colorbar to the scene.

        Renders as a tall narrow vertical bar (color varies top-to-bottom)
        positioned on the right side of the image.

        Should be called after add_surface().

        Args:
            title (str or None): Colorbar label.  None uses the name of the
                last scalar added by add_surface().
            position_x (float): Left edge X position in window coords (0-1).
            position_y (float): Bottom edge Y position in window coords (0-1).
            width (float): Colorbar width in window coords.
            height (float): Colorbar height in window coords.
            n_labels (int): Number of labeled tick marks.
            fmt (str): Printf-style format string for tick labels.

        Returns:
            ADBPlotter: self (for method chaining).
        """
        if self._last_scalar is None:
            return self

        self._plotter.add_scalar_bar(
            title      = title if title is not None else self._last_scalar,
            mapper     = self._last_actor.mapper if self._last_actor is not None else None,
            n_labels   = n_labels,
            fmt        = fmt,
            position_x = position_x,
            position_y = position_y,
            width      = width,
            height     = height,
            vertical   = True,
        )
        return self

    # ------------------------------------------------------------------

    def set_view(self, name='iso'):
        """Set the camera to a standard orthographic view preset.

        Args:
            name (str): View name.  One of:
                'iso'    -- isometric (default)
                'top'    -- looking down -Z (plan view)
                'bottom' -- looking up +Z
                'front'  -- looking in -Y direction
                'back'   -- looking in +Y direction
                'left'   -- looking in -X direction
                'right'  -- looking in +X direction

        Returns:
            ADBPlotter: self (for method chaining).
        """
        _valid_views = {'iso', 'top', 'bottom', 'front', 'back', 'left', 'right'}
        if name not in _valid_views:
            raise ValueError(
                f'Unknown view {name!r}.  Valid views: {sorted(_valid_views)}'
            )

        # Set the view direction first (fitted to all actors by default).
        if name == 'iso':
            self._plotter.view_isometric()
        else:
            vec, up = _VIEW_VECTORS[name]
            self._plotter.view_vector(vec, viewup=up)

        # Re-fit camera to the wing surface only.  Without this, wake filaments
        # extending thousands of chord lengths downstream dominate the bounding
        # box and push the wing to a tiny speck.
        #
        # view_isometric(bounds=...) does not actually move the camera (pyvista
        # bug), so we call reset_camera(bounds=padded) explicitly afterward and
        # set camera_set=True to prevent pyvista from auto-resetting on render.
        if self._surface_mesh is not None:
            b = list(self._surface_mesh.bounds)
            # Add 15 % padding so the wing has breathing room in the frame
            span = max(b[1]-b[0], b[3]-b[2], max(b[5]-b[4], 1e-3))
            m = span * 0.15
            padded = [b[0]-m, b[1]+m, b[2]-m, b[3]+m, b[4]-m, b[5]+m]
            self._plotter.reset_camera(bounds=padded)
            self._plotter.camera_set = True   # lock: prevent auto-reset on render

        self._plotter.enable_parallel_projection()
        return self

    # ------------------------------------------------------------------

    def save(self, filename, window_size=None):
        """Render the scene and write a PNG image.

        Args:
            filename (str): Output path.  Should end in '.png'.
            window_size (tuple or None): Override the resolution (width, height)
                for this save only.

        Returns:
            ADBPlotter: self (for method chaining).
        """
        if window_size is not None:
            self._plotter.window_size = list(window_size)
        self._plotter.screenshot(filename)
        return self

    # ------------------------------------------------------------------

    def show(self, save_on_close=None):
        """Open an interactive pyvista window (requires a display server).

        Construct with ``off_screen=False`` to use this method; the
        off_screen flag cannot be changed after construction.

        Args:
            save_on_close (str or None): If provided, a PNG is written to this
                path when the window is closed.  The screenshot is taken via
                an ExitEvent observer so the OpenGL context is still valid.

        Returns:
            ADBPlotter: self (for method chaining).
        """
        if save_on_close is not None:
            path = save_on_close
            def _before_close(plotter):
                print(f'Writing {path}')
                plotter.screenshot(path)
            self._plotter.show(before_close_callback=_before_close)
        else:
            self._plotter.show()
        return self


# ---------------------------------------------------------------------------
# animate() -- frame-sequence renderer for unsteady (time-accurate) analyses
# ---------------------------------------------------------------------------

def animate(adb_file, output_dir, scalar='Cp', cmap=None, clim=None,
            add_wake=True, add_edges=False, reflect=False,
            extend_to_infinity=False,
            view='iso', window_size=_DEFAULT_WINDOW_SIZE,
            fmt='frame_{:04d}.png', **surface_kwargs):
    """Render every case in an ADB file to a numbered PNG frame sequence.

    Frames are named using the `fmt` template with a 1-based case number,
    e.g. ``frame_0001.png``, ``frame_0002.png``, ... and can be assembled
    into a video with ffmpeg::

        ffmpeg -r 24 -i frames/frame_%04d.png -c:v libx264 output.mp4

    Color limits default to the global data range across all cases
    (``adb_file.global_cp_data_bounds()`` for Cp) and are held fixed
    across all frames so the animation is consistent.  Supply an explicit
    ``clim`` to override.

    Args:
        adb_file (ADBFile): Opened ADB file to iterate over.
        output_dir (str): Directory for output PNG files (created if absent).
        scalar (str): Scalar field to color by ('Cp', 'CpSteady', etc.).
        cmap (str or None): Colormap name.  None uses CMAP_VIEWER.
        clim (tuple or None): (min, max) color limits applied to all frames.
            None auto-ranges from case 1 (recommended for animations).
        add_wake (bool): If True, add wake geometry to every frame.
        add_edges (bool): If True, add coarse mesh level-1 edges to every frame.
        reflect (bool): If True, add Y-reflection for symmetry cases.
        extend_to_infinity (bool): If True, include the far-field infinity
            node at the end of each wake filament.  Default False.
        view (str): Camera view preset (see ADBPlotter.set_view()).
        window_size (tuple): Image resolution (width, height).
        fmt (str): Python format string for frame filenames.  Must contain
            exactly one integer placeholder, e.g. ``'frame_{:04d}.png'``.
        **surface_kwargs: Additional keyword arguments forwarded to
            ADBPlotter.add_surface() for every frame.

    Returns:
        int: Number of frames written.
    """
    os.makedirs(output_dir, exist_ok=True)

    # Determine clim once so all frames share a consistent range.
    # For Cp, use the pre-computed global mean +/- 1 sigma auto range (no file I/O
    # needed -- calculated during the initial scan pass).  For other scalars,
    # load case 1 and compute the same outlier-robust range on the fly.
    if clim is None:
        if scalar == 'Cp' and hasattr(adb_file, 'global_cp_auto_range'):
            clim = adb_file.global_cp_auto_range()
        if clim is None:
            # Fallback: load case 1, compute mean +/- 1 sigma auto range
            geom1, soln1 = adb_file.LoadCase(1)
            n_tris1   = geom1.NumberOfTris
            surf_mask = geom1.TriList_surface_type[1:n_tris1 + 1] > 0
            t_idx1    = np.where(surf_mask)[0] + 1
            arr  = getattr(soln1, scalar)[t_idx1].astype(np.float64)
            clim = _scalar_auto_range(arr)

    for case in range(1, adb_file.NumberOfCases + 1):
        geom, soln = adb_file.LoadCase(case)

        p = ADBPlotter(geom, soln, adb_file.Header, window_size=window_size)
        p.add_surface(scalar=scalar, cmap=cmap, clim=clim,
                      reflect=reflect, **surface_kwargs)
        if add_edges:
            p.add_mesh_edges(level=1, reflect=reflect)
        if add_wake:
            p.add_wake(reflect=reflect, extend_to_infinity=extend_to_infinity)
        if geom.NumberOfPropulsionElements > 0:
            p.add_propulsion_elements(reflect=reflect)
        p.add_colorbar()
        p.set_view(view)
        p.save(os.path.join(output_dir, fmt.format(case)))

    return adb_file.NumberOfCases


# ---------------------------------------------------------------------------
# plot_wake_modes() -- 2x2 comparison of all wake rendering styles
# ---------------------------------------------------------------------------

def plot_wake_modes(geom, soln, header, output_path,
                    scalar='Cp', cmap=None, clim=None,
                    wake_color='steelblue',
                    extend_to_infinity=False, reflect=False,
                    view='iso', window_size=_DEFAULT_WINDOW_SIZE,
                    off_screen=True):
    """Render all four wake styles side-by-side in a 2x2 grid and save to PNG.

    Each subplot shows the surface colored by ``scalar`` (same colormap and
    limits in all four panels) overlaid with one wake rendering mode:

    +----------+-----------+
    |  lines   |  tubes    |
    +----------+-----------+
    |  points  |  surface  |
    +----------+-----------+

    The 'surface' panel uses the wake sheet triangles (TriList_surface_type
    == 0); the other three use the vortex filament polylines.

    Args:
        geom (ADBGeometry): Geometry for the case to visualize.
        soln (ADBSolution): Solution for the case to visualize.
        header (ADBHeader): File header.
        output_path (str): Output PNG file path.
        scalar (str): Surface scalar field (default 'Cp').
        cmap (str or None): Colormap name.  None uses CMAP_VIEWER.
        clim (tuple or None): (min, max) color limits.  None auto-ranges.
        wake_color (str): Color for all wake geometry.
        extend_to_infinity (bool): If True, include the far-field
            wake extension.  Default False.
        reflect (bool): If True, add Y-reflected copies for symmetry cases.
        view (str): Camera view preset (see ADBPlotter.set_view()).
        window_size (tuple): Image resolution (width, height).
        off_screen (bool): True (default) writes a PNG to output_path; False
            opens an interactive pyvista window instead.

    Returns:
        str or None: ``output_path`` when off_screen=True, else None.
    """
    if cmap is None:
        cmap = CMAP_VIEWER

    # Build surface meshes once; reuse across all four subplots.
    surface_meshes = _build_surface_meshes(geom)

    if clim is None:
        all_scalars = np.concatenate([
            getattr(soln, scalar)[t_idx].astype(np.float64)
            for _, t_idx, _ in surface_meshes
        ])
        lo, hi = _scalar_auto_range(all_scalars)
        if header.ModelType == PANEL_MODEL:
            hi = max(hi, 1.0)
        clim = (lo, hi)

    # Guard degenerate range.
    if clim[0] == clim[1]:
        delta = max(abs(clim[0]) * 0.05, 0.1)
        clim = (clim[0] - delta, clim[1] + delta)

    # Pre-build wake meshes.
    wake_filament = _build_wake(soln, extend_to_infinity=extend_to_infinity)
    wake_surface  = _build_wake_surface(geom, extend_to_infinity=extend_to_infinity)

    # Bounding box of the wing surface (for tube radius and camera framing).
    all_bounds = [m.bounds for m, _, _ in surface_meshes]
    xmin = min(b[0] for b in all_bounds)
    xmax = max(b[1] for b in all_bounds)
    ymin = min(b[2] for b in all_bounds)
    ymax = max(b[3] for b in all_bounds)
    zmin = min(b[4] for b in all_bounds)
    zmax = max(b[5] for b in all_bounds)
    span = max(xmax - xmin, ymax - ymin, max(zmax - zmin, 1e-3))
    pad  = span * 0.15
    cam_bounds = [xmin-pad, xmax+pad, ymin-pad, ymax+pad, zmin-pad, zmax+pad]
    tube_r = 0.002 * np.sqrt((xmax-xmin)**2 + (ymax-ymin)**2 + (zmax-zmin)**2)

    modes  = ['lines',   'tubes',   'points',   'surface']
    labels = ['Lines',   'Tubes',   'Points',   'Surface']

    p = pv.Plotter(shape=(2, 2), off_screen=off_screen,
                   window_size=list(window_size))
    p.background_color = 'white'

    add_kwargs = dict(
        scalars         = scalar,
        clim            = list(clim),
        cmap            = cmap,
        smooth_shading  = False,
        show_edges      = False,
        show_scalar_bar = False,
    )

    last_actor = None

    for idx, (mode, label) in enumerate(zip(modes, labels)):
        row, col = divmod(idx, 2)
        p.subplot(row, col)

        # Surface Cp
        for mesh, t_idx, _ in surface_meshes:
            m = mesh.copy()
            m.cell_data[scalar] = getattr(soln, scalar)[t_idx].astype(np.float64)
            last_actor = p.add_mesh(m, **add_kwargs)
            if reflect:
                p.add_mesh(_reflect_polydata(m), **add_kwargs)

        # Wake overlay
        if mode == 'surface':
            if wake_surface is not None:
                p.add_mesh(wake_surface, color=wake_color, opacity=0.4,
                           show_edges=False)
                if reflect:
                    p.add_mesh(_reflect_polydata(wake_surface),
                               color=wake_color, opacity=0.4, show_edges=False)
        elif mode == 'tubes':
            if wake_filament is not None:
                p.add_mesh(wake_filament.tube(radius=tube_r), color=wake_color)
                if reflect:
                    p.add_mesh(
                        _reflect_polydata(wake_filament).tube(radius=tube_r),
                        color=wake_color)
        elif mode == 'points':
            if wake_filament is not None:
                p.add_mesh(wake_filament, color=wake_color, point_size=3,
                           render_points_as_spheres=True, style='points')
                if reflect:
                    p.add_mesh(_reflect_polydata(wake_filament), color=wake_color,
                               point_size=3, render_points_as_spheres=True,
                               style='points')
        else:  # 'lines'
            if wake_filament is not None:
                p.add_mesh(wake_filament, color=wake_color,
                           line_width=1.0, style='wireframe')
                if reflect:
                    p.add_mesh(_reflect_polydata(wake_filament),
                               color=wake_color, line_width=1.0,
                               style='wireframe')

        # Propulsion elements (disk/annulus + outline).
        if geom.NumberOfPropulsionElements > 0:
            _add_propulsion_elements_to_plotter(p, geom, reflect=reflect)

        # Per-subplot label and camera.
        p.add_text(label, position='upper_left', font_size=14, color='black')

        if view == 'iso':
            p.view_isometric()
        else:
            vec, up = _VIEW_VECTORS[view]
            p.view_vector(vec, viewup=up)
        p.reset_camera(bounds=cam_bounds)
        p.camera_set = True
        p.enable_parallel_projection()

    # Shared colorbar -- added to the last active subplot (bottom-right).
    p.subplot(1, 1)
    if last_actor is not None:
        p.add_scalar_bar(
            title      = scalar,
            mapper     = last_actor.mapper,
            vertical   = True,
            position_x = 0.87,
            position_y = 0.1,
            width      = 0.06,
            height     = 0.8,
            n_labels   = 5,
            fmt        = '%.1f',
        )

    # Synchronize cameras so dragging/zooming in one panel moves all four.
    p.link_views()

    if off_screen:
        p.screenshot(output_path)
        return output_path
    else:
        if output_path is not None:
            def _before_close(plotter):
                print(f'Writing {output_path}')
                plotter.screenshot(output_path)
            p.show(before_close_callback=_before_close)
        else:
            p.show()
