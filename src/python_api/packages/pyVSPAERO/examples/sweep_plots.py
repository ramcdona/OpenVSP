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
sweep_plots.py -- Cp plots for every case in a sweep with a shared colormap
===========================================================================

Iterates over all cases in an *.adb file and writes one Cp PNG per case.
All images share the same colormap limits (global mean +/- 1sigma auto range) so
the colors are directly comparable across the sweep.

A summary table is printed to stdout showing Mach, Alpha, Beta, and the Cp
data-range for every case.  Per-case auto ranges (mean +/- 1sigma) are also
reported so you can see how the auto-ranging tracks the changing solution.

In export mode (default) one PNG per case is written alongside the *.adb
file.  Pass --interactive to open a live window for the selected case_index
instead of exporting all cases.

Usage::

    python sweep_plots.py <file.adb> [case_index] [--interactive]

Requires the optional 'plot' extras:
    pip install 'pyvspaero[plot]'
"""

import argparse
import os
import sys

from pyvspaero import ADBFile
from pyvspaero.adb_plot import ADBPlotter


def main():
    """Parse arguments and write one Cp PNG per case with a shared colormap."""
    parser = argparse.ArgumentParser(
        description='Per-case Cp sweep plots with a shared colormap.',
    )
    parser.add_argument('adb_path',
                        help='Path to the VSPAERO *.adb output file.')
    parser.add_argument('case_index', nargs='?', type=int, default=1,
                        help='1-based case index for --interactive mode (default: 1).')
    parser.add_argument('--interactive', action='store_true',
                        help='Open an interactive window for case_index instead of '
                             'exporting all cases.')
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

    global_clim = adb.global_cp_auto_range()
    reflect     = bool(adb.Header.SymmetryFlag)

    print(f'Opened: {adb}')
    print(f'  Cases      : {n}')
    print(f'  Global Cp auto range (mean +/- 1sigma): {global_clim[0]:.4f} .. {global_clim[1]:.4f}')

    # Per-case auto ranges -- useful to understand how the colormap tracks the data.
    print(f'\n  Per-case Cp auto range:')
    for i in range(1, n + 1):
        lo, hi = adb.CaseCpAutoRange[i]
        print(f'    [{i:3d}]  lo={lo:.4f}  hi={hi:.4f}')

    # Sweep summary table.
    print(f'\n  Sweep summary:')
    print(f'  {"Case":>4}  {"Mach":>6}  {"Alpha":>7}  {"Beta":>6}  {"CpMin":>7}  {"CpMax":>7}')
    for case, (g, s) in enumerate(adb, start=1):
        print(f'  {case:4d}  {s.Mach:6.4f}  {s.Alpha:7.3f}  {s.Beta:6.3f}'
              f'  {s.Cp[1:].min():7.4f}  {s.Cp[1:].max():7.4f}')

    if args.interactive:
        geom, soln = adb.LoadCase(idx)
        out_path   = os.path.join(out_dir, f'{stem}_sweep_case{idx}_cp.png')
        print(f'\nOpening interactive view for case {idx}...')
        print(f'Will save to: {out_path}')
        (ADBPlotter(geom, soln, adb.Header, off_screen=False)
            .add_surface(scalar='Cp', clim=global_clim, reflect=reflect)
            .add_propulsion_elements(reflect=reflect)
            .add_colorbar(title='Cp', fmt='%.1f')
            .set_view('iso')
            .show(save_on_close=out_path))
        return

    # Export one PNG per case with the shared global colormap.
    print()
    for i in range(1, n + 1):
        geom, soln = adb.LoadCase(i)
        out_path = os.path.join(out_dir, f'{stem}_sweep_case{i}_cp.png')
        print(f'Writing {out_path}')
        (ADBPlotter(geom, soln, adb.Header)
            .add_surface(scalar='Cp', clim=global_clim, reflect=reflect)
            .add_propulsion_elements(reflect=reflect)
            .add_colorbar(title='Cp', fmt='%.1f')
            .set_view('iso')
            .save(out_path))


if __name__ == '__main__':
    main()
