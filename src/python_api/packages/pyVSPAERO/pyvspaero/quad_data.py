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
Data container dataclasses for VSPAERO quad cutting-plane flow field data.

These classes wrap the data produced by quad_reader and provide named
attribute access and NumPy arrays for all per-element data.

Indexing convention
-------------------
All per-node and per-cell arrays use 1-based indexing (index [0] is unused
padding), matching the C++ convention used throughout VSPAERO and the rest of
pyvspaero.  Arrays have shape (N+1,) or (N+1, 3); valid data runs from [1]
through [N].
"""

from dataclasses import dataclass
import numpy as np


@dataclass
class QuadPlaneDef:
    """Descriptor for one cutting plane from a *.quad.cases catalog file.

    Each entry specifies the orientation and position of one adaptive
    quadtree-meshed flow-field slice that VSPAERO evaluated.

    Attributes:
        index (int): 1-based plane index within the catalog.
        direction (int): Cutting-plane orientation.

            - 1 -- plane perpendicular to the X axis (constant X)
            - 2 -- plane perpendicular to the Y axis (constant Y)
            - 3 -- plane perpendicular to the Z axis (constant Z)

        value (float): Coordinate of the cutting plane along the chosen axis
            (e.g. for direction=2 the plane lies at Y=value).
    """
    index:     int
    direction: int
    value:     float

    @property
    def axis_name(self):
        """Human-readable axis label ('X', 'Y', or 'Z')."""
        return {1: 'X', 2: 'Y', 3: 'Z'}.get(self.direction, '?')

    def __repr__(self):
        return (f'QuadPlaneDef(index={self.index}, '
                f'direction={self.direction} ({self.axis_name}), '
                f'value={self.value})')


@dataclass
class QuadSlicePlane:
    """Flow-field data for one cutting plane at one flow condition.

    Contains the adaptive quadtree mesh geometry and the VSPAERO-computed
    velocity and pressure-coefficient fields at each mesh node.

    The mesh is a 2-D quadrilateral (quad) mesh embedded in 3-D space.  All
    cells are leaf cells of the quadtree that lie entirely outside the body
    geometry.

    Indexing convention
    -------------------
    All per-node and per-cell arrays use 1-based indexing (index [0] is unused
    padding), matching the rest of pyvspaero.  Valid data runs from index 1
    through NumberOfNodes / NumberOfCells.

    Coordinate system
    -----------------
    Node coordinates are stored exactly as written by the VSPAERO solver and
    read by the VSPAERO Viewer -- they are directly usable as (x, y, z) in the
    same world frame as the aircraft geometry in the *.adb file.

    Velocity normalisation
    ----------------------
    Velocity components are stored as **absolute** values (same units as
    Vref, typically physical velocity).  The header values Vref and Vmax are
    provided for optional normalisation by the caller.

    Attributes:
        direction (int): Cutting-plane orientation (1=X, 2=Y, 3=Z).
        value (float): Cutting-plane coordinate along the chosen axis.
        Vref (float): Reference (freestream) velocity from the solver.
        Vmax (float): Maximum velocity magnitude recorded on this plane.
        NumberOfNodes (int): Number of mesh nodes (N); valid indices 1..N.
        xyz (np.ndarray): Node positions, shape (N+1, 3).  xyz[i] = [x, y, z].
        velocity (np.ndarray): Node velocity vectors, shape (N+1, 3).
            velocity[i] = [u, v, w] in absolute velocity units.
        Cp (np.ndarray): Node pressure coefficients, shape (N+1,).
        NumberOfCells (int): Number of quad cells (M); valid indices 1..M.
        cells (np.ndarray): Quad connectivity, shape (M+1, 4).  cells[j]
            gives four 1-based node indices for the j-th quad cell.
    """
    direction:     int
    value:         float
    Vref:          float
    Vmax:          float
    NumberOfNodes: int
    xyz:           np.ndarray   # (N+1, 3)
    velocity:      np.ndarray   # (N+1, 3)
    Cp:            np.ndarray   # (N+1,)
    NumberOfCells: int
    cells:         np.ndarray   # (M+1, 4) -- 1-based node indices

    @property
    def axis_name(self):
        """Human-readable axis label ('X', 'Y', or 'Z')."""
        return {1: 'X', 2: 'Y', 3: 'Z'}.get(self.direction, '?')

    @property
    def velocity_magnitude(self):
        """Per-node velocity magnitude, shape (N+1,).  Index [0] is 0."""
        mag = np.zeros(self.NumberOfNodes + 1, dtype=np.float64)
        v = self.velocity[1:]
        mag[1:] = np.sqrt((v ** 2).sum(axis=1))
        return mag

    def __repr__(self):
        return (f'QuadSlicePlane({self.axis_name}={self.value}, '
                f'nodes={self.NumberOfNodes}, cells={self.NumberOfCells})')
