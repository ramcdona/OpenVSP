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
Entry point for ``python -m pyvspaero_viewer``.

Usage::

    python -m pyvspaero_viewer <file.adb> [case_index] [--output FILE]
"""

import argparse
import os
import sys


def _build_parser():
    parser = argparse.ArgumentParser(
        prog='vspaero-viewer',
        description='Desktop viewer for VSPAERO *.adb files.',
    )
    parser.add_argument(
        'adb_path',
        help='Path to the VSPAERO *.adb output file.',
    )
    parser.add_argument(
        'case_index',
        nargs='?',
        type=int,
        default=1,
        help='1-based case index to display (default: 1).',
    )
    parser.add_argument(
        '--output',
        metavar='FILE',
        help='Write a PNG to FILE without opening a window (off-screen mode).',
    )
    return parser


def main():
    """Parse command-line arguments and launch the viewer."""
    parser = _build_parser()
    args   = parser.parse_args()

    from pyvspaero import ADBFile

    if not os.path.isfile(args.adb_path):
        print(f'Error: file not found: {args.adb_path}', file=sys.stderr)
        sys.exit(1)

    adb = ADBFile(args.adb_path)
    n   = adb.NumberOfCases
    idx = args.case_index

    # Print case list.
    print(f'Opened: {adb}')
    print(f'  Cases: {n}')
    if adb.CaseList is not None:
        for i in range(1, n + 1):
            cr     = adb.CaseList[i]
            marker = ' <--' if i == idx else ''
            print(f'  [{i:3d}]  M={cr.Mach:.4f}  Alpha={cr.Alpha:.2f} deg  '
                  f'Beta={cr.Beta:.2f} deg{marker}')

    if idx < 1 or idx > n:
        print(f'Error: case_index {idx} is out of range -- '
              f'file has {n} case(s).', file=sys.stderr)
        sys.exit(1)

    # --output mode: off-screen PNG, no window.
    if args.output is not None:
        from pyvspaero_viewer.scene import _SceneManager
        scene = _SceneManager(adb, idx)
        scene.rebuild()
        scene.set_view('iso')
        print(f'Writing {args.output}')
        scene.plotter.screenshot(args.output)
        return

    # Desktop mode.
    from PySide6.QtWidgets import QApplication
    qt_app = QApplication.instance() or QApplication(sys.argv)
    from pyvspaero_viewer.app import VSPAERODesktopApp
    viewer = VSPAERODesktopApp(args.adb_path, case_index=idx)
    viewer.run()
    sys.exit(qt_app.exec())


if __name__ == '__main__':
    main()
