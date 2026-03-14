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
animate_sweep.py -- animation frame sequence for a VSPAERO sweep or unsteady run
================================================================================

Iterates over all cases in an *.adb file and writes one numbered PNG frame
per case into a ``{stem}_frames/`` subdirectory alongside the *.adb file.
All frames share the same colormap limits (global mean +/- 1sigma auto range).

The frames can be assembled into a video with ffmpeg:

    ffmpeg -r 24 -i {stem}_frames/frame_%04d.png -c:v libx264 animation.mp4

In export mode (default) all frames are written.  Pass --interactive to open
a live window for the selected case_index instead of rendering all frames.

Usage::

    python animate_sweep.py <file.adb> [case_index] [--interactive]

Requires the optional 'plot' extras:
    pip install 'pyvspaero[plot]'
"""

import argparse
import os
import sys

from pyvspaero import ADBFile
from pyvspaero.adb_plot import ADBPlotter, animate


def main():
    """Parse arguments and write animation frames for all cases."""
    parser = argparse.ArgumentParser(
        description='Write animation frames for a VSPAERO sweep or unsteady run.',
    )
    parser.add_argument('adb_path',
                        help='Path to the VSPAERO *.adb output file.')
    parser.add_argument('case_index', nargs='?', type=int, default=1,
                        help='1-based case index for --interactive mode (default: 1).')
    parser.add_argument('--interactive', action='store_true',
                        help='Open an interactive window for case_index instead of '
                             'rendering all frames.')
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

    reflect = bool(adb.Header.SymmetryFlag)

    print(f'Opened: {adb}')
    print(f'  Cases: {n}')

    if args.interactive:
        global_clim = adb.global_cp_auto_range()
        geom, soln  = adb.LoadCase(idx)
        frames_dir  = os.path.join(out_dir, f'{stem}_frames')
        out_path    = os.path.join(frames_dir, f'frame_{idx:04d}.png')
        print(f'\nOpening interactive view for case {idx}...')
        print(f'Will save to: {out_path}')
        os.makedirs(frames_dir, exist_ok=True)
        (ADBPlotter(geom, soln, adb.Header, off_screen=False)
            .add_surface(scalar='Cp', clim=global_clim, reflect=reflect)
            .add_wake(reflect=reflect)
            .add_colorbar(title='Cp', fmt='%.1f')
            .set_view('iso')
            .show(save_on_close=out_path))
        return

    frames_dir = os.path.join(out_dir, f'{stem}_frames')
    print(f'\nWriting animation frames to {frames_dir}/')

    n_frames = animate(
        adb,
        output_dir = frames_dir,
        scalar     = 'Cp',
        add_wake   = True,
        add_edges  = False,
        reflect    = reflect,
        view       = 'iso',
    )

    print(f'Wrote {n_frames} frames.')
    print(f'Assemble with:')
    print(f"  ffmpeg -r 24 -i '{frames_dir}/frame_%04d.png' -c:v libx264 animation.mp4")


if __name__ == '__main__':
    main()
