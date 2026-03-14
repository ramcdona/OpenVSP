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
cp_plot.py -- Cp surface plot with mesh overlay
===============================================

Renders the surface colored by Cp with the computational mesh drawn on top.
In export mode (default) two PNG files are written -- an isometric view and a
top (plan) view.  Pass --interactive to open a live pyvista window instead.

Colormap limits are the global mean +/- 1sigma auto range across all cases in the
file, so the image is visually consistent with other plots from the same run.
Symmetric (half-model) geometries are reflected automatically.

If the solution contains propulsion elements (actuator disks / nozzles) they
are overlaid as an additional transparent-surface figure.

Usage::

    python cp_plot.py <file.adb> [case_index] [--interactive]

Requires the optional 'plot' extras:
    pip install 'pyvspaero[plot]'
"""

import argparse
import os
import sys

from pyvspaero import ADBFile
from pyvspaero.adb_plot import ADBPlotter


def main():
    """Parse arguments and render Cp surface plots for the chosen case."""
    parser = argparse.ArgumentParser(
        description='Cp surface plot with mesh overlay for a VSPAERO *.adb file.',
    )
    parser.add_argument('adb_path',
                        help='Path to the VSPAERO *.adb output file.')
    parser.add_argument('case_index', nargs='?', type=int, default=1,
                        help='1-based case index to plot (default: 1).')
    parser.add_argument('--interactive', action='store_true',
                        help='Open an interactive window instead of writing PNGs.')
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
    print(f'  Nodes / Tris : {geom.NumberOfNodes} / {geom.NumberOfTris}')
    print(f'  Cp auto range: {global_clim[0]:.4f} .. {global_clim[1]:.4f}  (global mean +/- 1sigma)')

    if args.interactive:
        # Interactive mode -- isometric view with mesh overlay.
        out_iso = os.path.join(out_dir, f'{stem}_case{idx}_cp_iso.png')
        print(f'Will save to: {out_iso}')
        (ADBPlotter(geom, soln, adb.Header, off_screen=False)
            .add_surface(scalar='Cp', style='smooth', clim=global_clim,
                         reflect=reflect)
            .add_mesh_edges(level=1, color='black', line_width=0.4,
                            reflect=reflect)
            .add_propulsion_elements(reflect=reflect)
            .add_colorbar(title='Cp', fmt='%.1f')
            .set_view('iso')
            .show(save_on_close=out_iso))
        return

    # Export mode -- write isometric and top-view PNGs.
    out_iso = os.path.join(out_dir, f'{stem}_case{idx}_cp_iso.png')
    out_top = os.path.join(out_dir, f'{stem}_case{idx}_cp_top.png')

    print(f'Writing {out_iso}')
    (ADBPlotter(geom, soln, adb.Header)
        .add_surface(scalar='Cp', style='smooth', clim=global_clim,
                     reflect=reflect)
        .add_mesh_edges(level=1, color='black', line_width=0.4,
                        reflect=reflect)
        .add_propulsion_elements(reflect=reflect)
        .add_colorbar(title='Cp', fmt='%.1f')
        .set_view('iso')
        .save(out_iso))

    print(f'Writing {out_top}')
    (ADBPlotter(geom, soln, adb.Header)
        .add_surface(scalar='Cp', clim=global_clim, reflect=reflect)
        .add_propulsion_elements(reflect=reflect)
        .add_colorbar(title='Cp', fmt='%.1f')
        .set_view('top')
        .save(out_top))

    # Propulsion elements (actuator disks / nozzles) -- only if present.
    if geom.NumberOfPropulsionElements > 0:
        out_prop = os.path.join(out_dir, f'{stem}_case{idx}_propulsion.png')
        print(f'Writing {out_prop}')
        (ADBPlotter(geom, soln, adb.Header)
            .add_surface(scalar='Cp', style='smooth', clim=global_clim,
                         reflect=reflect)
            .add_propulsion_elements(rotor_color='blue', nozzle_color='red',
                                     reflect=reflect)
            .add_colorbar(title='Cp', fmt='%.1f')
            .set_view('iso')
            .save(out_prop))


if __name__ == '__main__':
    main()
