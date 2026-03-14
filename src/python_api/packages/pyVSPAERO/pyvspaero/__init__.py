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
pyvspaero -- Python library for reading and post-processing VSPAERO output files.

The primary entry point is :class:`ADBFile`, which opens a VSPAERO ``*.adb``
binary output file and provides case-by-case access to geometry and solution data.

Quick start::

    from pyvspaero import ADBFile

    adb = ADBFile('Wing.adb')
    print(adb.NumberOfCases)

    # Load one case.
    geom, soln = adb.LoadCase(1)
    print(f'M={soln.Mach:.4f}  Alpha={soln.Alpha:.2f} deg')
    print(f'Cp range: {soln.Cp[1:].min():.3f} to {soln.Cp[1:].max():.3f}')

    # Iterate over all cases.
    for geom, soln in adb:
        print(soln.Alpha)

    # Outlier-robust colormap range spanning all cases (useful for sweep plots).
    lo, hi = adb.global_cp_auto_range()

All per-element arrays use 1-based indexing (index [0] is unused zero-padding).

Modules
-------
adb_format
    Binary file format specification and struct format strings.  This is the
    canonical reference for the ``*.adb`` format; update it when VSPAERO
    changes the format.
adb_reader
    Low-level parser for ``*.adb`` and ``*.adb.cases`` files.  Implements
    :class:`ADBFile` and :func:`read_cases_file`.
adb_data
    Data container dataclasses (:class:`ADBHeader`, :class:`ADBGeometry`,
    :class:`ADBSolution`, etc.) returned by :class:`ADBFile`.
adb_plot
    Visualization tools (requires ``pyvista``).  Import separately::

        from pyvspaero.adb_plot import ADBPlotter

quad_reader
    Reader for the adaptive quadtree cutting-plane flow-field output
    (``*.quad.cases`` and ``*.case.N.quad.M.dat`` files).
quad_data
    Data container dataclasses for cutting-plane data (:class:`QuadSlicePlane`).
"""

from .adb_reader import ADBFile, read_cases_file
from .adb_data import (
    ADBHeader,
    ADBGeometry,
    ADBSolution,
    CaseRecord,
    PropulsionElement,
    ControlSurface,
    CoarseMeshLevel,
    WakeFilament,
)
from .quad_reader import QuadFile
from .quad_data import QuadPlaneDef, QuadSlicePlane
