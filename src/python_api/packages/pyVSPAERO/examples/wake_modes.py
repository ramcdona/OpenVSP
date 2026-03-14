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
wake_modes.py -- side-by-side comparison of all four wake rendering styles
=========================================================================

Renders a 2x2 grid showing the same case with lines, tubes, points, and
surface wake styles so all four representations can be compared directly.
In export mode (default) a single PNG is written.  Pass --interactive to
open a live pyvista window with the 2x2 grid.

    +----------+-----------+
    |  lines   |  tubes    |
    +----------+-----------+
    |  points  |  surface  |
    +----------+-----------+

Usage::

    python wake_modes.py <file.adb> [case_index] [--interactive]

Requires the optional 'plot' extras:
    pip install 'pyvspaero[plot]'
"""

import argparse
import os
import sys

from pyvspaero import ADBFile
from pyvspaero.adb_plot import plot_wake_modes


def main():
    """Parse arguments and render the 2x2 wake style comparison."""
    parser = argparse.ArgumentParser(
        description='2x2 wake-style comparison for a VSPAERO *.adb file.',
    )
    parser.add_argument('adb_path',
                        help='Path to the VSPAERO *.adb output file.')
    parser.add_argument('case_index', nargs='?', type=int, default=1,
                        help='1-based case index to plot (default: 1).')
    parser.add_argument('--interactive', action='store_true',
                        help='Open an interactive window instead of writing a PNG.')
    args = parser.parse_args()

    adb = ADBFile(args.adb_path)
    n   = adb.NumberOfCases
    idx = args.case_index

    if idx < 1 or idx > n:
        print(f'Error: case_index {idx} out of range -- file has {n} case(s).',
              file=sys.stderr)
        sys.exit(1)

    out_dir = os.path.dirname(os.path.abspath(args.adb_path))
    stem    = os.path.splitext(os.path.basename(args.adb_path))[0]

    geom, soln = adb.LoadCase(idx)
    global_clim = adb.global_cp_auto_range()
    reflect     = bool(adb.Header.SymmetryFlag)

    print(f'Opened: {adb}')
    print(f'Case {idx}:  M={soln.Mach:.4f}  Alpha={soln.Alpha:.2f} deg  Beta={soln.Beta:.2f} deg')

    out_path = os.path.join(out_dir, f'{stem}_case{idx}_wake_modes.png')
    if args.interactive:
        print(f'Will save to: {out_path}')
        plot_wake_modes(geom, soln, adb.Header, out_path,
                        clim=global_clim, reflect=reflect,
                        off_screen=False)
    else:
        print(f'Writing {out_path}')
        plot_wake_modes(geom, soln, adb.Header, out_path,
                        clim=global_clim, reflect=reflect)


if __name__ == '__main__':
    main()
