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
mesh_view.py -- compare all mesh levels in a VSPAERO *.adb file
===============================================================

Renders a multi-panel figure showing every mesh representation available in
the file:

  Panel 1  -- full surface triangle mesh
  Panel 2  -- finest coarse polygon mesh (level 1)
  Panel 3  -- next coarser polygon mesh  (level 2)
  ...

The subplot grid is sized automatically to accommodate however many levels
are present.  All 3-D views are linked so rotating or zooming in any panel
moves all others in sync.

No Cp solution or colorbar is shown; this is purely a geometric comparison.
The script loads case 1 (mesh topology is the same for all cases in a steady
sweep).

Usage::

    python mesh_view.py <file.adb> [--interactive]

Requires the optional 'plot' extras:
    pip install 'pyvspaero[plot]'
"""

import argparse
import math
import os
import sys

import pyvista as pv

from pyvspaero import ADBFile
from pyvspaero.adb_plot import (
    _add_propulsion_elements_to_plotter,
    _build_surface_mesh,
    _build_mesh_edges,
    _reflect_polydata,
)

_WINDOW_SIZE  = (1920, 1080)
_SURF_COLOR   = 'lightgray'
_EDGE_COLOR   = 'black'


def _count_surface_polygons(level):
    """Count polygon loops (panels) in a coarse mesh level.

    Uses Euler's formula for planar graphs:

        N_polygons = E - V + C

    where E = number of surface edge segments, V = number of unique nodes
    in those edges, and C = number of connected components (found via
    Union-Find).  This formula holds for any planar graph regardless of
    whether the surface is open, non-watertight, or has T-junction
    non-manifold edges.  It would only be incorrect for surfaces with
    topological handles (genus > 0), which do not arise in aerodynamic
    panel meshes.
    """
    import numpy as np

    n_ce = level.NumberOfEdges
    surf_mask = np.array(level.EdgeList_SurfaceID[1:n_ce + 1], dtype=np.int32) > 0
    e_idx = np.where(surf_mask)[0] + 1

    if len(e_idx) == 0:
        return 0

    n1 = np.array(level.EdgeList_node1, dtype=np.int64)[e_idx]
    n2 = np.array(level.EdgeList_node2, dtype=np.int64)[e_idx]

    nodes = np.unique(np.concatenate([n1, n2]))
    V = len(nodes)
    E = len(e_idx)

    # Union-Find to count connected components.
    parent = dict(zip(nodes.tolist(), nodes.tolist()))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]   # path compression
            x = parent[x]
        return x

    for a, b in zip(n1.tolist(), n2.tolist()):
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[ra] = rb

    C = len({find(n) for n in nodes.tolist()})

    return E - V + C


def _grid_shape(n):
    """Return (rows, cols) for n subplots, approximately square."""
    cols = math.ceil(math.sqrt(n))
    rows = math.ceil(n / cols)
    return rows, cols


def main():
    """Parse arguments and render the multi-panel mesh level figure."""
    parser = argparse.ArgumentParser(
        description='Multi-panel mesh level viewer for VSPAERO *.adb files.',
    )
    parser.add_argument('adb_path',
                        help='Path to the VSPAERO *.adb output file.')
    parser.add_argument('--interactive', action='store_true',
                        help='Open an interactive window instead of writing a PNG.')
    args = parser.parse_args()

    adb = ADBFile(args.adb_path)

    out_dir = os.path.dirname(os.path.abspath(args.adb_path))
    stem    = os.path.splitext(os.path.basename(args.adb_path))[0]

    # Mesh topology is the same for every case in a steady sweep; load case 1.
    geom, _ = adb.LoadCase(1)

    n_levels   = geom.NumberOfMeshLevels
    n_subplots = 1 + n_levels          # triangles + one panel per coarse level
    rows, cols = _grid_shape(n_subplots)
    reflect    = bool(adb.Header.SymmetryFlag)

    print(f'Opened: {adb}')
    print(f'  Surfaces     : {adb.Header.NumberOfSurfaces}')
    print(f'  Mesh levels  : {n_levels}')
    print(f'  Subplot grid : {rows} x {cols}  ({n_subplots} panels)')

    # -----------------------------------------------------------------------
    # Build geometry once; reuse across all subplots.
    # -----------------------------------------------------------------------

    surface_mesh, _ = _build_surface_mesh(geom)
    tri_edges = surface_mesh.extract_all_edges()

    edge_meshes  = []
    poly_counts  = []
    for lvl in range(1, n_levels + 1):
        ml = geom.CoarseMeshLevels[lvl]
        em = _build_mesh_edges(ml)
        np_ = _count_surface_polygons(ml)
        edge_meshes.append(em)
        poly_counts.append(np_)
        print(f'  Level {lvl}: {ml.NumberOfNodes} nodes, {np_} polygons '
              f'({em.n_cells} surface edge segments)')

    print(f'  Triangles    : {surface_mesh.n_cells} surface tris')

    # Labels for each subplot.
    labels = [f'Triangle Mesh  ({surface_mesh.n_cells} tris)']
    for lvl, np_ in enumerate(poly_counts, start=1):
        labels.append(f'Level {lvl}  ({np_} polygons)')

    # -----------------------------------------------------------------------
    # Camera framing -- based on the surface bounding box.
    # -----------------------------------------------------------------------

    b    = list(surface_mesh.bounds)
    span = max(b[1]-b[0], b[3]-b[2], max(b[5]-b[4], 1e-3))
    pad  = span * 0.15
    cam_bounds = [b[0]-pad, b[1]+pad, b[2]-pad, b[3]+pad, b[4]-pad, b[5]+pad]

    # -----------------------------------------------------------------------
    # Build the multi-panel figure.
    # -----------------------------------------------------------------------

    off_screen = not args.interactive
    p = pv.Plotter(shape=(rows, cols), off_screen=off_screen,
                   window_size=list(_WINDOW_SIZE))
    p.background_color = 'white'

    for i in range(rows * cols):
        row, col = divmod(i, cols)
        p.subplot(row, col)

        if i >= n_subplots:
            # Empty cell -- leave blank.
            continue

        # All panels: gray filled surface + explicit edge line mesh on top.
        # Using the same rendering path for both triangle and polygon edges
        # ensures consistent line appearance across subplots.
        p.add_mesh(surface_mesh, color=_SURF_COLOR, show_edges=False,
                   show_scalar_bar=False)
        if reflect:
            p.add_mesh(_reflect_polydata(surface_mesh),
                       color=_SURF_COLOR, show_edges=False,
                       show_scalar_bar=False)

        if i == 0:
            # Panel 1: triangle mesh edges.
            p.add_mesh(tri_edges, color=_EDGE_COLOR, line_width=1.5)
            if reflect:
                p.add_mesh(_reflect_polydata(tri_edges),
                           color=_EDGE_COLOR, line_width=1.5)
        else:
            # Panels 2+: coarse polygon mesh edges.
            p.add_mesh(edge_meshes[i - 1], color=_EDGE_COLOR, line_width=1.5)
            if reflect:
                p.add_mesh(_reflect_polydata(edge_meshes[i - 1]),
                           color=_EDGE_COLOR, line_width=1.5)

        _add_propulsion_elements_to_plotter(p, geom, reflect=reflect)

        p.add_text(labels[i], position='upper_left', font_size=12, color='black')
        p.view_isometric()
        p.reset_camera(bounds=cam_bounds)
        p.camera_set = True
        p.enable_parallel_projection()

    p.link_views()

    out_path = os.path.join(out_dir, f'{stem}_mesh_levels.png')
    if off_screen:
        print(f'\nWriting {out_path}')
        p.screenshot(out_path)
    else:
        print(f'Will save to: {out_path}')
        def _before_close(plotter):
            print(f'Writing {out_path}')
            plotter.screenshot(out_path)
        p.show(before_close_callback=_before_close)


if __name__ == '__main__':
    main()
