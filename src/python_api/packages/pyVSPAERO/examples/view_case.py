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
view_case.py -- interactive VSPAERO case viewer / quick-plot utility
====================================================================

Quick-look utility for VSPAERO *.adb files.  By default opens an interactive
pyvista window; pass ``--output`` to write a PNG instead.

Shows the surface colored by Cp with tube wake and computational mesh
overlay.  Colormap limits are the global mean +/- 1sigma auto range across all
cases so the display is representative even when viewing a single case from
a multi-case sweep.

Usage::

    python view_case.py <path/to/file.adb> [case_index] [--output FILE]

Arguments:
    adb_path      Path to the VSPAERO *.adb output file.
    case_index    1-based case number to display (default: 1).
    --output FILE Write a PNG to FILE instead of opening an interactive window.

Requires the optional 'plot' extras:
    pip install 'pyvspaero[plot]'
"""

import argparse
import os
import sys

from pyvspaero import ADBFile
from pyvspaero.adb_plot import ADBPlotter


def main():
    """Parse arguments and open the interactive quick-look viewer."""
    parser = argparse.ArgumentParser(
        description='Cp viewer for VSPAERO *.adb files.',
    )
    parser.add_argument('adb_path',
                        help='Path to the VSPAERO *.adb output file.')
    parser.add_argument('case_index', nargs='?', type=int, default=1,
                        help='1-based case index to display (default: 1).')
    parser.add_argument('--output', metavar='FILE',
                        help='Write a PNG to FILE instead of opening an '
                             'interactive window.')
    args = parser.parse_args()

    adb = ADBFile(args.adb_path)

    n = adb.NumberOfCases
    idx = args.case_index
    if idx < 1 or idx > n:
        print(f'Error: case_index {idx} out of range -- file has {n} case(s).',
              file=sys.stderr)
        sys.exit(1)

    print(f'Opened: {adb}')
    print(f'  Cases: {n}')
    if adb.CaseList is not None:
        for i in range(1, n + 1):
            cr = adb.CaseList[i]
            marker = ' <--' if i == idx else ''
            print(f'  [{i:3d}]  M={cr.Mach:.4f}  Alpha={cr.Alpha:.2f} deg  '
                  f'Beta={cr.Beta:.2f} deg{marker}')

    geom, soln = adb.LoadCase(idx)
    print(f'\nLoaded case {idx}:  M={soln.Mach:.4f}  Alpha={soln.Alpha:.2f} deg  '
          f'Beta={soln.Beta:.2f} deg')

    global_clim = adb.global_cp_auto_range()
    reflect = bool(adb.Header.SymmetryFlag)
    off_screen = args.output is not None

    plotter = (ADBPlotter(geom, soln, adb.Header, off_screen=off_screen)
        .add_surface(scalar='Cp', style='smooth', clim=global_clim,
                     reflect=reflect)
        .add_mesh_edges(level=1, color='black', line_width=0.4,
                        reflect=reflect)
        .add_wake(style='tubes', reflect=reflect)
        .add_propulsion_elements(reflect=reflect)
        .add_colorbar(title='Cp', fmt='%.1f')
        .set_view('iso'))

    if off_screen:
        print(f'Writing {args.output}')
        plotter.save(args.output)
    else:
        out_dir  = os.path.dirname(os.path.abspath(args.adb_path))
        stem     = os.path.splitext(os.path.basename(args.adb_path))[0]
        out_path = os.path.join(out_dir, f'{stem}_case{idx}_view.png')
        print(f'Will save to: {out_path}')
        plotter.show(save_on_close=out_path)


if __name__ == '__main__':
    main()
