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
VSPAERO quad cutting-plane flow-field file reader.

File format documentation
=========================

The quad output files capture velocity and pressure-coefficient data on
adaptive quadtree-meshed cutting planes through the flow field.  Two file
types are produced alongside the main *.adb binary:

``{stem}.quad.cases``  -- Cutting-plane catalog (text)
-----------------------------------------------------
Lists all cutting planes defined for the run.  The stem matches the *.adb
file (e.g. ``Wing.quad.cases``).

Format::

    <N>
    <index> <direction> <value>
    <index> <direction> <value>
    ...

Fields:

- **N** (int): number of cutting planes.
- **index** (int): 1-based plane number (matches the index in the dat filenames).
- **direction** (int): orientation of the cutting plane.

  - 1 -- perpendicular to X (constant-X plane, a.k.a. YZ-plane)
  - 2 -- perpendicular to Y (constant-Y plane, a.k.a. XZ-plane)
  - 3 -- perpendicular to Z (constant-Z plane, a.k.a. XY-plane)

- **value** (float): coordinate of the cutting plane along the chosen axis.

Example::

    1
    1 2 0.000000


``{stem}.case.{C}.quad.{P}.dat``  -- Per-case, per-plane data (text)
--------------------------------------------------------------------
One file per flow condition case *C* and cutting-plane *P*.  The stem and
indices match the *.quad.cases file.

**Naming convention**

For an N-case sweep, cases 1 ... N-1 use positive case numbers.  The final
case (case N) is written with a **negative** case number (``-N``) to signal
to the solver that additional end-of-sweep output should be written.  There
is therefore never a ``*.case.N.quad.*.dat`` file for the last case; instead
``*.case.-N.quad.*.dat`` exists.

Examples for a 3-case sweep with one cutting plane::

    Wing.case.1.quad.1.dat
    Wing.case.2.quad.1.dat
    Wing.case.-3.quad.1.dat   <- last case uses negative index

**File contents**

Line 1 -- header::

    <direction> <value> <Vref> <Vmax>

- **direction** (int): same as in the cases file.
- **value** (float): cutting-plane coordinate.
- **Vref** (float): freestream (reference) velocity from the solver.
- **Vmax** (float): maximum velocity magnitude on this plane.

Line 2 -- counts::

    <N_nodes> <N_cells>

Node section -- N_nodes lines::

    <id> <x> <y> <z> <u> <v> <w> <Cp>

- **id** (int): 1-based sequential node index (renumbered from internal solver IDs).
- **x, y, z** (float): physical position in the same coordinate frame as the
  aircraft geometry in the *.adb file.  Coordinates are directly usable for
  3-D rendering without any remapping.
- **u, v, w** (float): velocity components in absolute units (not normalised).
- **Cp** (float): pressure coefficient.

Cell section -- N_cells lines::

    <id> <n0> <n1> <n2> <n3>

- **id** (int): 1-based sequential cell index.
- **n0, n1, n2, n3** (int): 1-based indices into the node list; the four
  corners of a quadrilateral leaf cell of the quadtree.

Only leaf cells (cells with no children in the quadtree) that lie entirely
outside the body geometry are written.

Authoritative C++ sources
--------------------------
- Writer: ``src/vsp_aero/Solver/QuadTree.C``     ``QUAD_TREE::WriteQuadTreeToFile()``
- Writer: ``src/vsp_aero/Solver/VSP_Solver.C``   (filename construction, header line)
- Reader: ``src/vsp_aero/Viewer/glviewer.C``      ``GL_VIEWER::LoadExistingSolutionData()``
- Catalog: ``src/vsp_aero/Viewer/glviewer.C``     ``GL_VIEWER::LoadQuadCuttingPlaneCaseList()``
"""

import os

import numpy as np

from .quad_data import QuadPlaneDef, QuadSlicePlane


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _parse_cases_file(path):
    """Parse a *.quad.cases catalog file.

    Args:
        path (str): Path to the *.quad.cases file.

    Returns:
        list[QuadPlaneDef]: 1-based list (index [0] is None).
    """
    with open(path, 'r') as f:
        n = int(f.readline().strip())
        planes = [None]                      # index [0] unused
        for _ in range(n):
            parts = f.readline().split()
            planes.append(QuadPlaneDef(
                index     = int(parts[0]),
                direction = int(parts[1]),
                value     = float(parts[2]),
            ))
    return planes


def _parse_dat_file(path):
    """Parse one *.case.N.quad.M.dat data file.

    Args:
        path (str): Path to the dat file.

    Returns:
        QuadSlicePlane: Parsed data.

    Raises:
        FileNotFoundError: If *path* does not exist.
        ValueError: If the file is malformed.
    """
    with open(path, 'r') as f:
        # Header line: direction, value, Vref, Vmax
        header = f.readline().split()
        direction = int(header[0])
        value     = float(header[1])
        Vref      = float(header[2])
        Vmax      = float(header[3])

        # Count line: N_nodes, N_cells
        counts    = f.readline().split()
        n_nodes   = int(counts[0])
        n_cells   = int(counts[1])

        # Node data -- 1-based; index [0] is unused zero-padding
        xyz      = np.zeros((n_nodes + 1, 3), dtype=np.float64)
        velocity = np.zeros((n_nodes + 1, 3), dtype=np.float64)
        Cp       = np.zeros(n_nodes + 1,      dtype=np.float64)

        for i in range(1, n_nodes + 1):
            parts = f.readline().split()
            # id x y z u v w Cp
            xyz[i, 0]      = float(parts[1])
            xyz[i, 1]      = float(parts[2])
            xyz[i, 2]      = float(parts[3])
            velocity[i, 0] = float(parts[4])
            velocity[i, 1] = float(parts[5])
            velocity[i, 2] = float(parts[6])
            Cp[i]          = float(parts[7])

        # Cell data -- 1-based; index [0] is unused zero-padding
        cells = np.zeros((n_cells + 1, 4), dtype=np.int32)

        for j in range(1, n_cells + 1):
            parts = f.readline().split()
            # id n0 n1 n2 n3
            cells[j, 0] = int(parts[1])
            cells[j, 1] = int(parts[2])
            cells[j, 2] = int(parts[3])
            cells[j, 3] = int(parts[4])

    return QuadSlicePlane(
        direction     = direction,
        value         = value,
        Vref          = Vref,
        Vmax          = Vmax,
        NumberOfNodes = n_nodes,
        xyz           = xyz,
        velocity      = velocity,
        Cp            = Cp,
        NumberOfCells = n_cells,
        cells         = cells,
    )


def _dat_path(stem, case, plane_index):
    """Construct the *.dat filename for a given case and plane index.

    Args:
        stem (str): Base filename without extension (e.g. ``'Wing'``).
        case (int): Signed case number as stored in the filename (may be
            negative for the last case in a sweep).
        plane_index (int): 1-based cutting-plane index.

    Returns:
        str: Full path to the dat file.
    """
    return f'{stem}.case.{case}.quad.{plane_index}.dat'


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

class QuadFile:
    """Reader for VSPAERO quad cutting-plane flow-field data.

    Reads the ``*.quad.cases`` catalog file and provides per-case access to
    the per-plane ``*.case.N.quad.M.dat`` data files.

    The ``*.quad.cases`` file is located alongside the ``*.adb`` file.  Each
    ``*.dat`` file contains the velocity and Cp fields on one adaptive
    quadtree mesh for one flow condition.

    Usage::

        qf = QuadFile('Wing.adb')          # or QuadFile('Wing.quad.cases')
        print(qf.NumberOfPlanes)
        planes = qf.LoadCase(1)            # list of QuadSlicePlane
        for p in planes:
            print(p)

    Attributes:
        stem (str): Base filename stem (e.g. ``'Wing'`` or full path without
            the ``.quad.cases`` extension).
        NumberOfPlanes (int): Number of cutting planes.
        PlaneList (list[QuadPlaneDef]): 1-based list of plane descriptors
            (index [0] is None).
        NumberOfCases (int): Number of cases discovered from the dat files.
        CaseNumbers (list[int]): 1-based list of *signed* case numbers as
            stored in the dat filenames (the last case has a negative number).
    """

    def __init__(self, path):
        """Open and catalogue a quad dataset.

        Args:
            path (str): Path to any of the following (the others are inferred):

                - ``*.quad.cases`` catalog file
                - ``*.adb`` binary (looks for a sibling ``*.quad.cases``)
                - bare stem (e.g. ``'Wing'`` -- looks for ``Wing.quad.cases``
                  in the current directory)
        """
        # Resolve stem and locate the *.quad.cases file
        if path.endswith('.quad.cases'):
            cases_path = path
            self.stem  = path[:-len('.quad.cases')]
        elif path.endswith('.adb'):
            self.stem  = path[:-len('.adb')]
            cases_path = self.stem + '.quad.cases'
        else:
            # Treat as bare stem
            self.stem  = path
            cases_path = path + '.quad.cases'

        if not os.path.isfile(cases_path):
            raise FileNotFoundError(
                f'Quad cases file not found: {cases_path}'
            )

        self.PlaneList     = _parse_cases_file(cases_path)
        self.NumberOfPlanes = len(self.PlaneList) - 1   # excluding [0]

        # Discover available cases by scanning for dat files
        self.CaseNumbers = self._discover_cases()
        self.NumberOfCases = len(self.CaseNumbers)

    # ------------------------------------------------------------------

    def _discover_cases(self):
        """Scan the filesystem to find all dat files and record their case numbers.

        Returns:
            list[int]: Signed case numbers in ascending order of |case|.
                The last case has a negative sign; all others are positive.
        """
        if self.NumberOfPlanes == 0:
            return []

        # Use plane 1 as a probe -- find all case indices that have a dat file
        signed_cases = []
        case = 1
        while True:
            # Try positive case number first, then negative
            for signed in (case, -case):
                p = _dat_path(self.stem, signed, 1)
                if os.path.isfile(p):
                    signed_cases.append(signed)
                    break
            else:
                # Neither positive nor negative found -- end of sweep
                break
            if signed_cases[-1] < 0:
                # Negative sign marks the final case
                break
            case += 1

        return signed_cases

    # ------------------------------------------------------------------

    def _signed_case(self, case_index):
        """Return the signed case number stored in the dat filename for *case_index*.

        Args:
            case_index (int): 1-based case number (1 ... NumberOfCases).

        Returns:
            int: Signed case number (positive for intermediate cases, negative
                for the last case in a sweep).

        Raises:
            IndexError: If case_index is out of range.
        """
        if case_index < 1 or case_index > self.NumberOfCases:
            raise IndexError(
                f'case_index {case_index} out of range '
                f'(1 ... {self.NumberOfCases})'
            )
        return self.CaseNumbers[case_index - 1]

    # ------------------------------------------------------------------

    def LoadCase(self, case_index):
        """Load all cutting-plane data for one flow condition.

        Args:
            case_index (int): 1-based case number (1 ... NumberOfCases).

        Returns:
            list[QuadSlicePlane]: 1-based list of :class:`QuadSlicePlane`
                objects, one per cutting plane.  Index [0] is ``None``.

        Raises:
            IndexError: If *case_index* is out of range.
            FileNotFoundError: If a dat file is missing.
        """
        signed = self._signed_case(case_index)
        planes = [None]   # index [0] unused
        for pi in range(1, self.NumberOfPlanes + 1):
            path = _dat_path(self.stem, signed, pi)
            planes.append(_parse_dat_file(path))
        return planes

    # ------------------------------------------------------------------

    def __repr__(self):
        return (f"QuadFile('{self.stem}', "
                f"planes={self.NumberOfPlanes}, "
                f"cases={self.NumberOfCases})")
