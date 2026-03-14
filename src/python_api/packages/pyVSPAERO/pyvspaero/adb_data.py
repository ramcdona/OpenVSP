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
Data container dataclasses for VSPAERO *.adb file data.

These classes wrap the raw dicts produced by adb_reader and provide:
  - Named attribute access (soln.Cp instead of soln['Cp'])
  - NumPy arrays for all per-element data
  - Derived quantities computed on construction (CpSteady)

Indexing convention
-------------------
All per-element arrays follow the same 1-based convention as the C++ code
and the raw reader.  Arrays have shape (N+1,) or (N+1, 3) etc.; index [0]
is unused (zero or NaN); valid data runs from [1] through [N].  This mirrors
the C++ pattern ``for ( i = 1 ; i <= N ; i++ )`` and lets Python and C++
code be compared line-for-line.
"""

from dataclasses import dataclass
from typing import List, Optional
import numpy as np


# ---------------------------------------------------------------------------
# Header
# ---------------------------------------------------------------------------

@dataclass
class ADBHeader:
    """Contents of the *.adb file header block.

    Mirrors the variables populated by GL_VIEWER::LoadMeshData() and
    ADBSLICER::LoadMeshData() when reading the header section.
    """
    FILE_VERSION: int               # 2 or 3
    ModelType: int                  # VLM_MODEL=1 or PANEL_MODEL=2
    SymmetryFlag: int               # 0 or 1
    TimeAccurate: int               # 0 (steady) or 1 (time-accurate)

    NumberOfVortexLoops: int        # loops at the MG (coarse) level
    NumberOfNodes: int              # nodes at level 0 (fine mesh)
    NumberOfTris: int               # triangles at level 0
    NumberOfSurfaceVortexEdges: int # edges at the MG level

    Sref: float                     # reference area
    Cref: float                     # reference chord
    Bref: float                     # reference span
    Xcg: float                      # center of gravity X
    Ycg: float                      # center of gravity Y
    Zcg: float                      # center of gravity Z

    NumberOfSurfaces: int
    # 1-based lists (index [0] is None); surface names and component IDs
    SurfaceNameList: List[Optional[str]]    # length NumberOfSurfaces + 1
    Cart3DComponentList: List[Optional[int]] # length NumberOfSurfaces + 1


def make_header(raw_header):
    """Construct ADBHeader from the raw dict returned by _read_header().

    Args:
        raw_header (dict): Dict as returned by adb_reader._read_header().

    Returns:
        ADBHeader
    """
    return ADBHeader(
        FILE_VERSION               = raw_header['FILE_VERSION'],
        ModelType                  = raw_header['ModelType'],
        SymmetryFlag               = raw_header['SymmetryFlag'],
        TimeAccurate               = raw_header['TimeAccurate'],
        NumberOfVortexLoops        = raw_header['NumberOfVortexLoops'],
        NumberOfNodes              = raw_header['NumberOfNodes'],
        NumberOfTris               = raw_header['NumberOfTris'],
        NumberOfSurfaceVortexEdges = raw_header['NumberOfSurfaceVortexEdges'],
        Sref                       = raw_header['Sref'],
        Cref                       = raw_header['Cref'],
        Bref                       = raw_header['Bref'],
        Xcg                        = raw_header['Xcg'],
        Ycg                        = raw_header['Ycg'],
        Zcg                        = raw_header['Zcg'],
        NumberOfSurfaces           = raw_header['NumberOfSurfaces'],
        SurfaceNameList            = raw_header['SurfaceNameList'],
        Cart3DComponentList        = raw_header['Cart3DComponentList'],
    )


# ---------------------------------------------------------------------------
# Case record (from *.adb.cases companion text file)
# ---------------------------------------------------------------------------

@dataclass
class CaseRecord:
    """One entry from the *.adb.cases companion text file."""
    Mach:        float
    Alpha:       float   # degrees
    Beta:        float   # degrees
    CommentLine: str


# ---------------------------------------------------------------------------
# Propulsion elements
# ---------------------------------------------------------------------------

@dataclass
class PropulsionElement:
    """One rotor disk or engine nozzle.

    Mirrors the ROTOR_DISK and ENGINE_FACE binary data structures.
    """
    Type: str               # 'PROP_ROTOR' or 'ENGINE_NOZZLE'
    xyz: np.ndarray         # shape (3,)  center location
    normal: np.ndarray      # shape (3,)  disk / face normal
    radius: float

    # Rotor-only fields (zero for nozzles)
    hub_radius: float = 0.0
    rpm:        float = 0.0
    ct:         float = 0.0
    cp:         float = 0.0


# ---------------------------------------------------------------------------
# Control surfaces
# ---------------------------------------------------------------------------

@dataclass
class ControlSurface:
    """One control surface (hinge geometry + affected loop list).

    Mirrors the CONTROL_SURFACE struct used in glviewer and ADBSlicer.
    """
    HingeNode1: np.ndarray      # shape (3,)  hinge line point 1
    HingeNode2: np.ndarray      # shape (3,)  hinge line point 2
    HingeVec:   np.ndarray      # shape (3,)  hinge axis unit vector

    NumberOfLoops: int
    # 1-based int array of loop indices; index [0] unused
    LoopList: np.ndarray        # shape (NumberOfLoops + 1,) int32

    DeflectionAngle: float = 0.0   # degrees (converted from radians on read)


# ---------------------------------------------------------------------------
# Coarse (multi-grid) mesh level
# ---------------------------------------------------------------------------

@dataclass
class CoarseMeshLevel:
    """Nodes and edges for one coarse multi-grid level.

    All arrays are 1-based (shape N+1; index [0] unused).
    """
    NumberOfNodes: int
    NumberOfEdges: int

    # Node coordinates -- shape (NumberOfNodes + 1,) float32
    NodeList_x: np.ndarray
    NodeList_y: np.ndarray
    NodeList_z: np.ndarray

    # Edge connectivity -- shape (NumberOfEdges + 1,) int32 / bool
    EdgeList_SurfaceID:        np.ndarray   # int32; negative stripped, flag in IsBoundaryEdge
    EdgeList_MinValidTimeStep: np.ndarray   # int32
    EdgeList_IsBoundaryEdge:   np.ndarray   # bool/int
    EdgeList_IsKuttaEdge:      np.ndarray   # bool/int
    EdgeList_node1:            np.ndarray   # int32
    EdgeList_node2:            np.ndarray   # int32


# ---------------------------------------------------------------------------
# Geometry
# ---------------------------------------------------------------------------

@dataclass
class ADBGeometry:
    """Mesh geometry for one case / time step.

    All per-element arrays are 1-based (shape N+1; index [0] unused),
    mirroring the C++ convention.

    Mirrors the data populated by GL_VIEWER::UpdateMeshData() and
    ADBSLICER::UpdateMeshData().
    """

    NumberOfTris:  int
    NumberOfNodes: int

    # --- Triangulated surface mesh (level 0 / fine mesh) ---
    # All shape (NumberOfTris + 1,); index [0] unused.
    TriList_node1:            np.ndarray   # int32
    TriList_node2:            np.ndarray   # int32
    TriList_node3:            np.ndarray   # int32
    TriList_surface_type:     np.ndarray   # int32  (ComponentID in writer)
    TriList_surface_id:       np.ndarray   # int32
    TriList_MinValidTimeStep: np.ndarray   # int32
    TriList_area:             np.ndarray   # float32

    # --- Node coordinates (level 0 / fine mesh) ---
    # All shape (NumberOfNodes + 1,); index [0] unused.
    NodeList_x: np.ndarray   # float32
    NodeList_y: np.ndarray   # float32
    NodeList_z: np.ndarray   # float32

    # --- Propulsion elements ---
    NumberOfRotors:             int
    NumberOfNozzles:            int
    NumberOfPropulsionElements: int
    # 1-based list; index [0] is None
    PropulsionElementList: List[Optional[PropulsionElement]]

    # --- Multi-grid levels ---
    NumberOfMeshLevels: int
    # 1-based list of CoarseMeshLevel; index [0] is None
    CoarseMeshLevels: List[Optional[CoarseMeshLevel]]

    # --- Kutta (trailing-edge) data ---
    NumberOfKuttaEdges: int
    NumberOfKuttaNodes: int
    # 1-based int32 array; index [0] unused
    KuttaNodeList: np.ndarray   # shape (NumberOfKuttaNodes + 1,)

    # --- Control surfaces ---
    NumberOfControlSurfaces: int
    # 1-based list; index [0] is None
    ControlSurface: List[Optional[ControlSurface]]


def make_geometry(raw_geom):
    """Construct ADBGeometry from the raw dict returned by _read_geometry().

    Converts all plain Python lists to NumPy arrays.

    Args:
        raw_geom (dict): Dict as returned by adb_reader._read_geometry().

    Returns:
        ADBGeometry
    """
    NumberOfTris  = raw_geom['NumberOfTris']
    NumberOfNodes = raw_geom['NumberOfNodes']

    # --- Tri arrays ---
    TriList_node1            = np.array(raw_geom['TriList_node1'],            dtype=np.int32)
    TriList_node2            = np.array(raw_geom['TriList_node2'],            dtype=np.int32)
    TriList_node3            = np.array(raw_geom['TriList_node3'],            dtype=np.int32)
    TriList_surface_type     = np.array(raw_geom['TriList_surface_type'],     dtype=np.int32)
    TriList_surface_id       = np.array(raw_geom['TriList_surface_id'],       dtype=np.int32)
    TriList_MinValidTimeStep = np.array(raw_geom['TriList_MinValidTimeStep'], dtype=np.int32)
    TriList_area             = np.array(raw_geom['TriList_area'],             dtype=np.float32)

    # --- Node arrays ---
    NodeList_x = np.array(raw_geom['NodeList_x'], dtype=np.float32)
    NodeList_y = np.array(raw_geom['NodeList_y'], dtype=np.float32)
    NodeList_z = np.array(raw_geom['NodeList_z'], dtype=np.float32)

    # --- Propulsion elements ---
    NumberOfPropulsionElements = raw_geom['NumberOfPropulsionElements']
    raw_prop = raw_geom['PropulsionElementList']
    PropulsionElementList = [None]   # index [0] unused
    for j in range(1, NumberOfPropulsionElements + 1):
        r = raw_prop[j]
        if r['Type'] == 'PROP_ROTOR':
            elem = PropulsionElement(
                Type       = r['Type'],
                xyz        = np.array([r['xyz_x'], r['xyz_y'], r['xyz_z']],       dtype=np.float64),
                normal     = np.array([r['normal_x'], r['normal_y'], r['normal_z']], dtype=np.float64),
                radius     = r['radius'],
                hub_radius = r['hub_radius'],
                rpm        = r['rpm'],
                ct         = r['ct'],
                cp         = r['cp'],
            )
        else:  # ENGINE_NOZZLE
            elem = PropulsionElement(
                Type   = r['Type'],
                xyz    = np.array([r['xyz_x'], r['xyz_y'], r['xyz_z']],       dtype=np.float64),
                normal = np.array([r['normal_x'], r['normal_y'], r['normal_z']], dtype=np.float64),
                radius = r['radius'],
            )
        PropulsionElementList.append(elem)

    # --- Coarse mesh levels ---
    NumberOfMeshLevels = raw_geom['NumberOfMeshLevels']
    raw_cn = raw_geom['CoarseNodeList']
    raw_ce = raw_geom['CoarseEdgeList']
    CoarseMeshLevels = [None]   # index [0] unused
    for Level in range(1, NumberOfMeshLevels + 1):
        n_cn = raw_geom['NumberOfCoarseNodesForLevel'][Level]
        n_ce = raw_geom['NumberOfCoarseEdgesForLevel'][Level]
        cn = raw_cn[Level]
        ce = raw_ce[Level]
        CoarseMeshLevels.append(CoarseMeshLevel(
            NumberOfNodes              = n_cn,
            NumberOfEdges              = n_ce,
            NodeList_x                 = np.array(cn['x'],                 dtype=np.float32),
            NodeList_y                 = np.array(cn['y'],                 dtype=np.float32),
            NodeList_z                 = np.array(cn['z'],                 dtype=np.float32),
            EdgeList_SurfaceID         = np.array(ce['SurfaceID'],         dtype=np.int32),
            EdgeList_MinValidTimeStep  = np.array(ce['MinValidTimeStep'],  dtype=np.int32),
            EdgeList_IsBoundaryEdge    = np.array(ce['IsBoundaryEdge'],    dtype=np.int32),
            EdgeList_IsKuttaEdge       = np.array(ce['IsKuttaEdge'],       dtype=np.int32),
            EdgeList_node1             = np.array(ce['node1'],             dtype=np.int32),
            EdgeList_node2             = np.array(ce['node2'],             dtype=np.int32),
        ))

    # --- Kutta nodes ---
    KuttaNodeList = np.array(raw_geom['KuttaNodeList'], dtype=np.int32)

    # --- Control surfaces ---
    NumberOfControlSurfaces = raw_geom['NumberOfControlSurfaces']
    raw_cs = raw_geom['ControlSurface']
    ControlSurfaceList = [None]   # index [0] unused
    for i in range(1, NumberOfControlSurfaces + 1):
        cs = raw_cs[i]
        ControlSurfaceList.append(ControlSurface(
            HingeNode1     = np.array(cs['HingeNode1'], dtype=np.float32),
            HingeNode2     = np.array(cs['HingeNode2'], dtype=np.float32),
            HingeVec       = np.array(cs['HingeVec'],   dtype=np.float32),
            NumberOfLoops  = cs['NumberOfLoops'],
            LoopList       = np.array(cs['LoopList'],   dtype=np.int32),
            DeflectionAngle = cs['DeflectionAngle'],
        ))

    return ADBGeometry(
        NumberOfTris               = NumberOfTris,
        NumberOfNodes              = NumberOfNodes,
        TriList_node1              = TriList_node1,
        TriList_node2              = TriList_node2,
        TriList_node3              = TriList_node3,
        TriList_surface_type       = TriList_surface_type,
        TriList_surface_id         = TriList_surface_id,
        TriList_MinValidTimeStep   = TriList_MinValidTimeStep,
        TriList_area               = TriList_area,
        NodeList_x                 = NodeList_x,
        NodeList_y                 = NodeList_y,
        NodeList_z                 = NodeList_z,
        NumberOfRotors             = raw_geom['NumberOfRotors'],
        NumberOfNozzles            = raw_geom['NumberOfNozzles'],
        NumberOfPropulsionElements = NumberOfPropulsionElements,
        PropulsionElementList      = PropulsionElementList,
        NumberOfMeshLevels         = NumberOfMeshLevels,
        CoarseMeshLevels           = CoarseMeshLevels,
        NumberOfKuttaEdges         = raw_geom['NumberOfKuttaEdges'],
        NumberOfKuttaNodes         = raw_geom['NumberOfKuttaNodes'],
        KuttaNodeList              = KuttaNodeList,
        NumberOfControlSurfaces    = NumberOfControlSurfaces,
        ControlSurface             = ControlSurfaceList,
    )


# ---------------------------------------------------------------------------
# Wake filament
# ---------------------------------------------------------------------------

@dataclass
class WakeFilament:
    """One trailing vortex filament.

    Mirrors VORTEX_TRAIL data as written by VORTEX_TRAIL::WriteToFile().
    Node arrays are 1-based (index [0] unused).
    """
    te_node:   int          # 1-based Kutta node index
    s_over_b:  float        # non-dimensional span location
    NumberOfNodes: int
    # shape (NumberOfNodes + 1,) float64; index [0] unused
    x: np.ndarray
    y: np.ndarray
    z: np.ndarray


# ---------------------------------------------------------------------------
# Solution
# ---------------------------------------------------------------------------

@dataclass
class ADBSolution:
    """Solution data for one case / time step.

    All per-element arrays are 1-based (shape N+1; index [0] unused),
    mirroring the C++ convention.

    Mirrors the data populated by GL_VIEWER::LoadExistingSolutionData() and
    ADBSLICER::LoadSolutionData().
    """

    Mach:      float    # dimensionless
    Alpha:     float    # degrees (converted from radians on read)
    Beta:      float    # degrees (converted from radians on read)

    CpMinSoln: float    # minimum Cp reported by solver
    CpMaxSoln: float    # maximum Cp reported by solver

    # --- Solution on the computational (MG) mesh ---
    # shape (NumberOfVortexLoops + 1,) float64; index [0] unused
    GammaN:       np.ndarray   # circulation strength per loop
    dCp_Unsteady: np.ndarray   # unsteady delta-Cp per loop

    # shape (NumberOfSurfaceVortexEdges + 1,) float64; index [0] unused
    Fx: np.ndarray
    Fy: np.ndarray
    Fz: np.ndarray

    # shape (NumberOfVortexLoops + 1,) float64; index [0] unused
    U: np.ndarray   # velocity X at loop centroid
    V: np.ndarray   # velocity Y at loop centroid
    W: np.ndarray   # velocity Z at loop centroid

    # --- Solution on the input (fine) mesh ---
    # shape (NumberOfTris + 1,) float32; index [0] unused
    Cp:         np.ndarray   # total delta-Cp (steady + unsteady)
    CpUnsteady: np.ndarray   # unsteady component
    CpSteady:   np.ndarray   # derived: Cp - CpUnsteady
    Gamma:      np.ndarray   # circulation (interpolated to fine mesh)

    # --- Wake geometry ---
    NumberOfTrailingVortexEdges: int
    # 1-based list of WakeFilament; index [0] is None
    WakeFilaments: List[Optional[WakeFilament]]

    # Control surface deflection angles (degrees) are stored on the
    # ControlSurface objects inside ADBGeometry, updated during solution read.


def make_solution(raw_soln):
    """Construct ADBSolution from the raw dict returned by _read_solution().

    Converts all plain Python lists to NumPy arrays and computes CpSteady.

    Args:
        raw_soln (dict): Dict as returned by adb_reader._read_solution().

    Returns:
        ADBSolution
    """
    # MachList / AlphaList / BetaList are each length-2 lists (index [0] unused)
    Mach  = raw_soln['MachList'][1]
    Alpha = raw_soln['AlphaList'][1]   # already converted to degrees
    Beta  = raw_soln['BetaList'][1]    # already converted to degrees

    GammaN       = np.array(raw_soln['GammaN'],       dtype=np.float64)
    dCp_Unsteady = np.array(raw_soln['dCp_Unsteady'], dtype=np.float64)

    Fx = np.array(raw_soln['Fx'], dtype=np.float64)
    Fy = np.array(raw_soln['Fy'], dtype=np.float64)
    Fz = np.array(raw_soln['Fz'], dtype=np.float64)

    U = np.array(raw_soln['U'], dtype=np.float64)
    V = np.array(raw_soln['V'], dtype=np.float64)
    W = np.array(raw_soln['W'], dtype=np.float64)

    Cp         = np.array(raw_soln['Cp'],         dtype=np.float32)
    CpUnsteady = np.array(raw_soln['CpUnsteady'], dtype=np.float32)
    CpSteady   = Cp - CpUnsteady                  # derived quantity
    Gamma      = np.array(raw_soln['Gamma'],      dtype=np.float32)

    # --- Wake filaments ---
    NumberOfTrailingVortexEdges = raw_soln['NumberOfTrailingVortexEdges']
    XWake = raw_soln['XWake']
    YWake = raw_soln['YWake']
    ZWake = raw_soln['ZWake']
    NumberOfSubVortexNodesForEdge = raw_soln['NumberOfSubVortexNodesForEdge']

    WakeFilaments = [None]   # index [0] unused
    for i in range(1, NumberOfTrailingVortexEdges + 1):
        n = NumberOfSubVortexNodesForEdge[i]
        WakeFilaments.append(WakeFilament(
            te_node       = raw_soln['WingWakeNode'][i],
            s_over_b      = raw_soln['SWake'][i],
            NumberOfNodes = n,
            x = np.array(XWake[i], dtype=np.float64),
            y = np.array(YWake[i], dtype=np.float64),
            z = np.array(ZWake[i], dtype=np.float64),
        ))

    return ADBSolution(
        Mach                       = Mach,
        Alpha                      = Alpha,
        Beta                       = Beta,
        CpMinSoln                  = raw_soln['CpMinSoln'],
        CpMaxSoln                  = raw_soln['CpMaxSoln'],
        GammaN                     = GammaN,
        dCp_Unsteady               = dCp_Unsteady,
        Fx                         = Fx,
        Fy                         = Fy,
        Fz                         = Fz,
        U                          = U,
        V                          = V,
        W                          = W,
        Cp                         = Cp,
        CpUnsteady                 = CpUnsteady,
        CpSteady                   = CpSteady,
        Gamma                      = Gamma,
        NumberOfTrailingVortexEdges = NumberOfTrailingVortexEdges,
        WakeFilaments              = WakeFilaments,
    )
