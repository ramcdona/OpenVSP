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
quad_plot.py -- Cp surface with quad cutting-plane flow-field overlay
=====================================================================

Renders the aircraft surface colored by Cp overlaid with the VSPAERO
adaptive quad cutting-plane slices.  Each slice shows the flow-field scalar
distribution on the quadtree mesh computed by VSPAERO for that plane.

By default all available cutting planes are shown.  Use ``--planes`` to
select a subset.

In export mode (default) a single isometric PNG is written alongside the
*.adb file.  Pass --interactive to open a live pyvista window instead.

If no *.quad.cases file is found next to the *.adb file the script exits
with an informative message.

Usage::

    python quad_plot.py <file.adb> [case_index] [--interactive]
    python quad_plot.py <file.adb> [case_index] [--scalar Vmag]
    python quad_plot.py <file.adb> [case_index] [--planes 1,3,5]
    python quad_plot.py <file.adb> [case_index] [--planes 1-4,6]
    python quad_plot.py <file.adb> [case_index] [--transparent]

Requires the optional 'plot' extras:
    pip install 'pyvspaero[plot]'
"""

import argparse
import os
import sys

from pyvspaero import ADBFile
from pyvspaero.adb_plot import ADBPlotter

_TRANSPARENT_OPACITY = 0.4


def _parse_plane_selection(spec, n_planes):
    """Parse a plane selection string into a sorted list of 1-based indices.

    Accepts comma-separated integers and inclusive ranges, e.g.::

        '1,3,5'   -> [1, 3, 5]
        '1-4,6'   -> [1, 2, 3, 4, 6]
        '2-2'     -> [2]

    Args:
        spec (str): Selection string.
        n_planes (int): Total number of available planes (upper bound).

    Returns:
        list[int]: Sorted unique plane indices.

    Raises:
        SystemExit: If any token is invalid or out of range.
    """
    indices = set()
    for token in spec.split(','):
        token = token.strip()
        if '-' in token:
            parts = token.split('-', 1)
            try:
                lo, hi = int(parts[0]), int(parts[1])
            except ValueError:
                print(f'Error: invalid plane range "{token}".', file=sys.stderr)
                sys.exit(1)
            indices.update(range(lo, hi + 1))
        else:
            try:
                indices.add(int(token))
            except ValueError:
                print(f'Error: invalid plane index "{token}".', file=sys.stderr)
                sys.exit(1)

    bad = [i for i in indices if i < 1 or i > n_planes]
    if bad:
        print(f'Error: plane index/indices {bad} out of range '
              f'(1 ... {n_planes}).', file=sys.stderr)
        sys.exit(1)

    return sorted(indices)


def main():
    """Parse arguments and render the Cp surface with quad cutting-plane overlay."""
    parser = argparse.ArgumentParser(
        description='Cp surface + quad flow-field slice plot for a VSPAERO *.adb file.',
    )
    parser.add_argument('adb_path',
                        help='Path to the VSPAERO *.adb output file.')
    parser.add_argument('case_index', nargs='?', type=int, default=1,
                        help='1-based case index to plot (default: 1).')
    parser.add_argument('--interactive', action='store_true',
                        help='Open an interactive window instead of writing a PNG.')
    parser.add_argument('--scalar', default='Cp', metavar='FIELD',
                        help="Scalar field for the quad slices: 'Cp' (default) or 'Vmag'.")
    parser.add_argument('--planes', default=None, metavar='SPEC',
                        help="Comma-separated plane indices or ranges to plot, e.g. "
                             "'1,3,5' or '1-4,6'.  Default: all planes.")
    parser.add_argument('--transparent', action='store_true',
                        help=f'Render quad slices semi-transparently '
                             f'(opacity={_TRANSPARENT_OPACITY}).  '
                             f'Default is fully opaque.')
    args = parser.parse_args()

    adb = ADBFile(args.adb_path)
    n   = adb.NumberOfCases
    idx = args.case_index

    if idx < 1 or idx > n:
        print(f'Error: case_index {idx} out of range -- file has {n} case(s).',
              file=sys.stderr)
        sys.exit(1)

    if adb.QuadData is None:
        print(f'No quad cutting-plane data found alongside {args.adb_path}.',
              file=sys.stderr)
        print('Run VSPAERO with cutting planes enabled to generate *.quad.cases '
              'and *.case.N.quad.M.dat files.', file=sys.stderr)
        sys.exit(1)

    n_planes = adb.QuadData.NumberOfPlanes

    # Resolve which planes to plot
    if args.planes is not None:
        plane_indices = _parse_plane_selection(args.planes, n_planes)
    else:
        plane_indices = list(range(1, n_planes + 1))

    out_dir = os.path.dirname(os.path.abspath(args.adb_path))
    stem    = os.path.splitext(os.path.basename(args.adb_path))[0]

    geom, soln      = adb.LoadCase(idx)
    all_quad_planes = adb.QuadData.LoadCase(idx)
    global_clim     = adb.global_cp_auto_range()
    reflect         = bool(adb.Header.SymmetryFlag)

    # Build a 1-based list containing only the selected planes (others -> None)
    selected_planes = [None] + [
        all_quad_planes[i] if i in plane_indices else None
        for i in range(1, n_planes + 1)
    ]

    print(f'Opened: {adb}')
    print(f'Quad data: {adb.QuadData}')
    print(f'Case {idx}:  M={soln.Mach:.4f}  Alpha={soln.Alpha:.2f} deg  Beta={soln.Beta:.2f} deg')
    print(f'  Cutting planes available: {n_planes}')
    print(f'  Cutting planes selected : {plane_indices}')
    for i in plane_indices:
        p = all_quad_planes[i]
        if p is not None:
            print(f'    [{i}] {p}')

    opacity    = _TRANSPARENT_OPACITY if args.transparent else 1.0
    off_screen = not args.interactive

    # For Vmag slices use a different colormap and auto range
    if args.scalar == 'Vmag':
        quad_cmap = 'plasma'
        quad_clim = None        # auto-range from the plane data
    else:
        quad_cmap = None        # default viewer colormap
        quad_clim = global_clim

    plotter = (ADBPlotter(geom, soln, adb.Header, off_screen=off_screen)
        .add_surface(scalar='Cp', style='smooth', clim=global_clim,
                     reflect=reflect)
        .add_quad_slices(selected_planes, scalar=args.scalar,
                         cmap=quad_cmap, clim=quad_clim,
                         opacity=opacity, show_edges=True, reflect=reflect)
        .add_propulsion_elements(reflect=reflect)
        .add_colorbar(title='Cp', fmt='%.1f')
        .set_view('iso'))

    out_path = os.path.join(out_dir, f'{stem}_case{idx}_quad_{args.scalar}.png')
    if args.interactive:
        print(f'Will save to: {out_path}')
        plotter.show(save_on_close=out_path)
    else:
        print(f'Writing {out_path}')
        plotter.save(out_path)


if __name__ == '__main__':
    main()
