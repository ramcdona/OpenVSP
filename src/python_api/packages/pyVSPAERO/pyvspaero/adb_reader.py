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
VSPAERO *.adb file reader.

Mirrors the structure of the authoritative C++ readers:
    Viewer/glviewer.C        GL_VIEWER::LoadMeshData()
                             GL_VIEWER::UpdateMeshData()
                             GL_VIEWER::LoadExistingSolutionData()
    Adb2Load/ADBSlicer.C    ADBSLICER::LoadMeshData()
                             ADBSLICER::UpdateMeshData()
                             ADBSLICER::LoadSolutionData()

Indexing convention
-------------------
VSPAERO C++ code allocates arrays with N+1 entries and ignores index [0],
using 1-based indices throughout (loops run ``for i = 1 ; i <= N ; i++``).
This reader follows the same convention so that the Python and C++ code can
be compared line-for-line.  All lists returned here have a dummy entry at
[0]; valid data starts at [1].
"""

import math
import struct

import numpy as np

from .adb_format import (
    ADB_MAGIC_V2,
    ADB_MAGIC_V3,
    PANEL_MODEL,
)
from .adb_data import (
    ADBHeader,
    ADBGeometry,
    ADBSolution,
    CaseRecord,
    make_header,
    make_geometry,
    make_solution,
)

# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

# Size constants matching C++ sizeof(int), sizeof(float), sizeof(double)
_I = 4   # int32
_F = 4   # float32
_D = 8   # float64
_C = 1   # char


def _read(f, byte_order, fmt):
    """Read and unpack a single struct from the file.

    Args:
        f (file): Open binary file object positioned at the data to read.
        byte_order (str): '<' (little-endian) or '>' (big-endian).
        fmt (str): struct format string WITHOUT a byte-order prefix.

    Returns:
        tuple: Unpacked values.
    """
    full_fmt = byte_order + fmt
    size = struct.calcsize(full_fmt)
    data = f.read(size)
    return struct.unpack(full_fmt, data)


def _readi(f, byte_order):
    """Read one int32."""
    return _read(f, byte_order, 'i')[0]


def _readf(f, byte_order):
    """Read one float32."""
    return _read(f, byte_order, 'f')[0]


def _readd(f, byte_order):
    """Read one float64."""
    return _read(f, byte_order, 'd')[0]


# ---------------------------------------------------------------------------
# Endianness detection
# Mirrors: GL_VIEWER::LoadMeshData() endian check in Viewer/glviewer.C
#          ADBSLICER::LoadMeshData() endian check in Adb2Load/ADBSlicer.C
# ---------------------------------------------------------------------------

def _detect_endianness(f):
    """Detect byte order and file version from the first int in the file.

    Reads the endianness marker.  If it does not match either known magic
    value, assumes the bytes are swapped, rewinds, and re-reads.  This is
    identical to the logic used in GL_VIEWER::LoadMeshData() and
    ADBSLICER::LoadMeshData().

    Args:
        f (file): Open binary file object at position 0.

    Returns:
        tuple: (byte_order, file_version) where byte_order is '<' or '>'
               and file_version is 2 or 3.
    """
    # First attempt: native / little-endian
    byte_order = '<'
    (DumInt,) = struct.unpack('<i', f.read(_I))

    if DumInt != ADB_MAGIC_V2 and DumInt != ADB_MAGIC_V3:
        # Byte-swap required -- rewind and re-read
        byte_order = '>'
        f.seek(0)
        (DumInt,) = struct.unpack('>i', f.read(_I))

    FILE_VERSION = 2
    if DumInt == ADB_MAGIC_V3:
        FILE_VERSION = 3

    return byte_order, FILE_VERSION


# ---------------------------------------------------------------------------
# Header reader
# Mirrors: GL_VIEWER::LoadMeshData()  header section in Viewer/glviewer.C
#          ADBSLICER::LoadMeshData()  header section in Adb2Load/ADBSlicer.C
# ---------------------------------------------------------------------------

def _read_header(f, byte_order, FILE_VERSION):
    """Read the ADB file header (written once per file).

    Mirrors the header-reading portion of GL_VIEWER::LoadMeshData() and
    ADBSLICER::LoadMeshData().  The endianness marker has already been
    consumed by _detect_endianness(); this function reads everything that
    follows it up to and including the surface name table.

    Args:
        f (file): Open binary file positioned immediately after the
                  endianness marker.
        byte_order (str): '<' or '>'.
        FILE_VERSION (int): 2 or 3, determined by _detect_endianness().

    Returns:
        dict: Header fields.  Keys match the C++ variable names where
              possible.  Surface names are stored in SurfaceNameList[1..N]
              (index [0] is None, preserving 1-based convention).
    """

    # Read in model type... VLM or PANEL
    ModelType = _readi(f, byte_order)

    # Read in symmetry flag
    SymmetryFlag = _readi(f, byte_order)

    # Read in unsteady analysis flag
    TimeAccurate = _readi(f, byte_order)

    # Read in header
    NumberOfVortexLoops        = _readi(f, byte_order)
    NumberOfNodes              = _readi(f, byte_order)
    NumberOfTris               = _readi(f, byte_order)
    NumberOfSurfaceVortexEdges = _readi(f, byte_order)

    Sref = _readf(f, byte_order)
    Cref = _readf(f, byte_order)
    Bref = _readf(f, byte_order)
    Xcg  = _readf(f, byte_order)
    Ycg  = _readf(f, byte_order)
    Zcg  = _readf(f, byte_order)

    NumberOfMachs = NumberOfAlphas = NumberOfBetas = 1

    # Read in surface names (Cart3d ID flags, names... in C++ comments)
    NumberOfSurfaces = _readi(f, byte_order)

    # 1-based lists: index [0] is a dummy None entry
    SurfaceNameList      = [None] * (NumberOfSurfaces + 1)
    Cart3DComponentList  = [None] * (NumberOfSurfaces + 1)

    for i in range(1, NumberOfSurfaces + 1):
        _readi(f, byte_order)                         # surface_id (== i)
        raw = f.read(100 * _C)                        # char[100] name
        SurfaceNameList[i] = raw.split(b'\x00', 1)[0].decode('ascii', errors='replace')
        Cart3DComponentList[i] = _readi(f, byte_order)  # component_id

    header = {
        'FILE_VERSION':              FILE_VERSION,
        'ModelType':                 ModelType,
        'SymmetryFlag':              SymmetryFlag,
        'TimeAccurate':              TimeAccurate,
        'NumberOfVortexLoops':       NumberOfVortexLoops,
        'NumberOfNodes':             NumberOfNodes,
        'NumberOfTris':              NumberOfTris,
        'NumberOfSurfaceVortexEdges': NumberOfSurfaceVortexEdges,
        'NumberOfMachs':             NumberOfMachs,
        'NumberOfAlphas':            NumberOfAlphas,
        'NumberOfBetas':             NumberOfBetas,
        'Sref':                      Sref,
        'Cref':                      Cref,
        'Bref':                      Bref,
        'Xcg':                       Xcg,
        'Ycg':                       Ycg,
        'Zcg':                       Zcg,
        'NumberOfSurfaces':          NumberOfSurfaces,
        'SurfaceNameList':           SurfaceNameList,
        'Cart3DComponentList':       Cart3DComponentList,
    }
    return header


# ---------------------------------------------------------------------------
# Geometry reader  (UpdateMeshData equivalent)
# Mirrors: GL_VIEWER::UpdateMeshData()   in Viewer/glviewer.C
#          ADBSLICER::UpdateMeshData()   in Adb2Load/ADBSlicer.C
# ---------------------------------------------------------------------------

def _read_geometry(f, header, byte_order):
    """Read one geometry sub-block from the current file position.

    In the *.adb format a geometry sub-block is written before every
    solution block (for every steady-state case and every time step).
    This function mirrors GL_VIEWER::UpdateMeshData() and
    ADBSLICER::UpdateMeshData().

    Arrays are 1-based: each list has a dummy None / zero entry at [0].

    Args:
        f (file): Open binary file positioned at the start of a geometry block.
        header (dict): Parsed header from _read_header().
        byte_order (str): '<' or '>'.

    Returns:
        dict: Geometry data.
    """

    FILE_VERSION              = header['FILE_VERSION']
    NumberOfTris              = header['NumberOfTris']
    NumberOfNodes             = header['NumberOfNodes']

    # --- Triangulated surface mesh (input / fine mesh, level 0) ---
    # Mirrors fread loop over NumberOfTris in UpdateMeshData()

    # 1-based: index [0] unused
    TriList_node1            = [0] * (NumberOfTris + 1)
    TriList_node2            = [0] * (NumberOfTris + 1)
    TriList_node3            = [0] * (NumberOfTris + 1)
    TriList_surface_type     = [0] * (NumberOfTris + 1)
    TriList_surface_id       = [0] * (NumberOfTris + 1)
    TriList_MinValidTimeStep = [0] * (NumberOfTris + 1)
    TriList_area             = [0.0] * (NumberOfTris + 1)

    for i in range(1, NumberOfTris + 1):
        TriList_node1[i]            = _readi(f, byte_order)
        TriList_node2[i]            = _readi(f, byte_order)
        TriList_node3[i]            = _readi(f, byte_order)
        TriList_surface_type[i]     = _readi(f, byte_order)  # ComponentID in writer
        TriList_surface_id[i]       = _readi(f, byte_order)
        TriList_MinValidTimeStep[i] = _readi(f, byte_order)
        TriList_area[i]             = _readf(f, byte_order)

    # --- Node coordinates (input / fine mesh, level 0) ---

    NodeList_x = [0.0] * (NumberOfNodes + 1)
    NodeList_y = [0.0] * (NumberOfNodes + 1)
    NodeList_z = [0.0] * (NumberOfNodes + 1)

    for i in range(1, NumberOfNodes + 1):
        NodeList_x[i] = _readf(f, byte_order)
        NodeList_y[i] = _readf(f, byte_order)
        NodeList_z[i] = _readf(f, byte_order)

    # --- Propulsion element counts ---

    NumberOfRotors  = _readi(f, byte_order)
    NumberOfNozzles = 0
    if FILE_VERSION == 3:
        NumberOfNozzles = _readi(f, byte_order)

    NumberOfPropulsionElements = NumberOfRotors + NumberOfNozzles

    # --- Rotor disk data ---
    # Mirrors ROTOR_DISK::Read_Binary_STP_Data() in Solver/RotorDisk.C

    # 1-based: index [0] unused
    RotorList = [None] * (NumberOfPropulsionElements + 1)

    j = 0
    for i in range(1, NumberOfRotors + 1):
        j += 1
        RotorList[j] = {
            'Type':       'PROP_ROTOR',
            'xyz_x':      _readd(f, byte_order),
            'xyz_y':      _readd(f, byte_order),
            'xyz_z':      _readd(f, byte_order),
            'normal_x':   _readd(f, byte_order),
            'normal_y':   _readd(f, byte_order),
            'normal_z':   _readd(f, byte_order),
            'radius':     _readd(f, byte_order),
            'hub_radius': _readd(f, byte_order),
            'rpm':        _readd(f, byte_order),
            'ct':         _readd(f, byte_order),
            'cp':         _readd(f, byte_order),
        }

    # --- Engine nozzle data (version 3 only) ---
    # Mirrors ENGINE_FACE::Read_Binary_STP_Data() in Solver/EngineFace.C

    if FILE_VERSION == 3:
        for i in range(1, NumberOfNozzles + 1):
            j += 1
            RotorList[j] = {
                'Type':     'ENGINE_NOZZLE',
                'xyz_x':    _readd(f, byte_order),
                'xyz_y':    _readd(f, byte_order),
                'xyz_z':    _readd(f, byte_order),
                'normal_x': _readd(f, byte_order),
                'normal_y': _readd(f, byte_order),
                'normal_z': _readd(f, byte_order),
                'radius':   _readd(f, byte_order),
            }

    # --- Multi-grid (coarse) mesh levels ---

    NumberOfMeshLevels = _readi(f, byte_order)

    # 1-based outer list; inner lists also 1-based
    NumberOfCoarseNodesForLevel = [0] * (NumberOfMeshLevels + 1)
    NumberOfCoarseEdgesForLevel = [0] * (NumberOfMeshLevels + 1)
    CoarseNodeList = [None] * (NumberOfMeshLevels + 1)
    CoarseEdgeList = [None] * (NumberOfMeshLevels + 1)

    for Level in range(1, NumberOfMeshLevels + 1):

        NumberOfCoarseNodesForLevel[Level] = _readi(f, byte_order)
        NumberOfCoarseEdgesForLevel[Level] = _readi(f, byte_order)

        n_cn = NumberOfCoarseNodesForLevel[Level]
        n_ce = NumberOfCoarseEdgesForLevel[Level]

        # 1-based node coordinate arrays
        cn_x = [0.0] * (n_cn + 1)
        cn_y = [0.0] * (n_cn + 1)
        cn_z = [0.0] * (n_cn + 1)

        for i in range(1, n_cn + 1):
            cn_x[i] = _readf(f, byte_order)
            cn_y[i] = _readf(f, byte_order)
            cn_z[i] = _readf(f, byte_order)

        CoarseNodeList[Level] = {'x': cn_x, 'y': cn_y, 'z': cn_z}

        # 1-based edge arrays
        ce_SurfaceID        = [0] * (n_ce + 1)
        ce_MinValidTimeStep = [0] * (n_ce + 1)
        ce_IsBoundaryEdge   = [0] * (n_ce + 1)
        ce_IsKuttaEdge      = [0] * (n_ce + 1)
        ce_node1            = [0] * (n_ce + 1)
        ce_node2            = [0] * (n_ce + 1)

        for i in range(1, n_ce + 1):
            ce_SurfaceID[i]        = _readi(f, byte_order)
            ce_MinValidTimeStep[i] = _readi(f, byte_order)
            ce_node1[i]            = _readi(f, byte_order)
            ce_node2[i]            = _readi(f, byte_order)

            # Negative SurfaceID flags a boundary / Kutta edge
            if ce_SurfaceID[i] < 0:
                ce_SurfaceID[i]      = -ce_SurfaceID[i]
                ce_IsBoundaryEdge[i] = 1

        CoarseEdgeList[Level] = {
            'SurfaceID':        ce_SurfaceID,
            'MinValidTimeStep': ce_MinValidTimeStep,
            'IsBoundaryEdge':   ce_IsBoundaryEdge,
            'IsKuttaEdge':      ce_IsKuttaEdge,
            'node1':            ce_node1,
            'node2':            ce_node2,
        }

    # --- Kutta (trailing edge) data for level 1 ---

    Level = 1

    NumberOfKuttaEdges = _readi(f, byte_order)

    for i in range(1, NumberOfKuttaEdges + 1):
        Edge = _readi(f, byte_order)
        CoarseEdgeList[Level]['IsKuttaEdge'][Edge] = 1

    NumberOfKuttaNodes = _readi(f, byte_order)

    # 1-based; index [0] unused
    KuttaNodeList = [0] * (NumberOfKuttaNodes + 1)
    for k in range(1, NumberOfKuttaNodes + 1):
        KuttaNodeList[k] = _readi(f, byte_order)

    # --- Control surfaces ---

    NumberOfControlSurfaces = _readi(f, byte_order)

    # 1-based: index [0] unused
    ControlSurface = [None] * (NumberOfControlSurfaces + 1)

    for i in range(1, NumberOfControlSurfaces + 1):

        # dummy / reserved int (always 0 in current writer; C++ readers
        # treat it as NumberOfControlSurfaceNodes which is always 0)
        NumberOfControlSurfaceNodes = _readi(f, byte_order)

        # If future format versions write node data here, this loop reads it
        cs_nodes = []
        for j in range(1, NumberOfControlSurfaceNodes + 1):
            x = _readf(f, byte_order)
            y = _readf(f, byte_order)
            z = _readf(f, byte_order)
            cs_nodes.append((x, y, z))

        # Hinge nodes and vector
        HingeNode1 = (_readf(f, byte_order), _readf(f, byte_order), _readf(f, byte_order))
        HingeNode2 = (_readf(f, byte_order), _readf(f, byte_order), _readf(f, byte_order))
        HingeVec   = (_readf(f, byte_order), _readf(f, byte_order), _readf(f, byte_order))

        # Affected loops -- 1-based
        NumberOfLoops = _readi(f, byte_order)
        LoopList = [0] * (NumberOfLoops + 1)
        for p in range(1, NumberOfLoops + 1):
            LoopList[p] = _readi(f, byte_order)

        ControlSurface[i] = {
            'NumberOfNodes':  NumberOfControlSurfaceNodes,
            'NodeList':       cs_nodes,
            'HingeNode1':     HingeNode1,
            'HingeNode2':     HingeNode2,
            'HingeVec':       HingeVec,
            'NumberOfLoops':  NumberOfLoops,
            'LoopList':       LoopList,
            'DeflectionAngle': 0.0,
        }

    geom = {
        'NumberOfTris':              NumberOfTris,
        'NumberOfNodes':             NumberOfNodes,
        'TriList_node1':             TriList_node1,
        'TriList_node2':             TriList_node2,
        'TriList_node3':             TriList_node3,
        'TriList_surface_type':      TriList_surface_type,
        'TriList_surface_id':        TriList_surface_id,
        'TriList_MinValidTimeStep':  TriList_MinValidTimeStep,
        'TriList_area':              TriList_area,
        'NodeList_x':                NodeList_x,
        'NodeList_y':                NodeList_y,
        'NodeList_z':                NodeList_z,
        'NumberOfRotors':            NumberOfRotors,
        'NumberOfNozzles':           NumberOfNozzles,
        'NumberOfPropulsionElements': NumberOfPropulsionElements,
        'PropulsionElementList':     RotorList,
        'NumberOfMeshLevels':        NumberOfMeshLevels,
        'NumberOfCoarseNodesForLevel': NumberOfCoarseNodesForLevel,
        'NumberOfCoarseEdgesForLevel': NumberOfCoarseEdgesForLevel,
        'CoarseNodeList':            CoarseNodeList,
        'CoarseEdgeList':            CoarseEdgeList,
        'NumberOfKuttaEdges':        NumberOfKuttaEdges,
        'NumberOfKuttaNodes':        NumberOfKuttaNodes,
        'KuttaNodeList':             KuttaNodeList,
        'NumberOfControlSurfaces':   NumberOfControlSurfaces,
        'ControlSurface':            ControlSurface,
    }
    return geom


# ---------------------------------------------------------------------------
# Solution reader
# Mirrors: GL_VIEWER::LoadExistingSolutionData()  in Viewer/glviewer.C
#          ADBSLICER::LoadSolutionData()          in Adb2Load/ADBSlicer.C
# ---------------------------------------------------------------------------

def _read_solution(f, header, geom, byte_order):
    """Read one solution sub-block from the current file position.

    Must be called immediately after _read_geometry() for the same case,
    since the two blocks are written back-to-back in the file.

    Mirrors the solution-reading portion of GL_VIEWER::LoadExistingSolutionData()
    and ADBSLICER::LoadSolutionData().

    Args:
        f (file): Open binary file positioned at the start of a solution block.
        header (dict): Parsed header from _read_header().
        geom (dict): Geometry from _read_geometry() for this case.
        byte_order (str): '<' or '>'.

    Returns:
        dict: Solution data.
    """

    NumberOfMachs              = header['NumberOfMachs']
    NumberOfAlphas             = header['NumberOfAlphas']
    NumberOfBetas              = header['NumberOfBetas']
    NumberOfVortexLoops        = header['NumberOfVortexLoops']
    NumberOfSurfaceVortexEdges = header['NumberOfSurfaceVortexEdges']
    NumberOfTris               = header['NumberOfTris']
    NumberOfControlSurfaces    = geom['NumberOfControlSurfaces']
    ControlSurface             = geom['ControlSurface']

    TORAD = math.pi / 180.0

    # Read in the EdgeMach, Q, and Alpha lists
    # (NumberOfMachs == NumberOfAlphas == NumberOfBetas == 1 always)
    MachList  = [0.0] * (NumberOfMachs  + 1)
    AlphaList = [0.0] * (NumberOfAlphas + 1)
    BetaList  = [0.0] * (NumberOfBetas  + 1)

    for k in range(1, NumberOfMachs  + 1):
        MachList[k]  = _readf(f, byte_order)
    for k in range(1, NumberOfAlphas + 1):
        AlphaList[k] = _readf(f, byte_order) / TORAD   # stored radians -> degrees
    for k in range(1, NumberOfBetas  + 1):
        BetaList[k]  = _readf(f, byte_order) / TORAD   # stored radians -> degrees

    # Read in data set
    CpMinSoln = _readf(f, byte_order)
    CpMaxSoln = _readf(f, byte_order)

    # Solution on computational mesh -- vortex loop strengths and unsteady Cp
    # 1-based: index [0] unused
    GammaN       = [0.0] * (NumberOfVortexLoops + 1)
    dCp_Unsteady = [0.0] * (NumberOfVortexLoops + 1)

    for m in range(1, NumberOfVortexLoops + 1):
        GammaN[m]       = _readd(f, byte_order)
        dCp_Unsteady[m] = _readd(f, byte_order)

    # Vortex edge forces on computational mesh
    # 1-based: index [0] unused
    Fx = [0.0] * (NumberOfSurfaceVortexEdges + 1)
    Fy = [0.0] * (NumberOfSurfaceVortexEdges + 1)
    Fz = [0.0] * (NumberOfSurfaceVortexEdges + 1)

    for m in range(1, NumberOfSurfaceVortexEdges + 1):
        Fx[m] = _readd(f, byte_order)
        Fy[m] = _readd(f, byte_order)
        Fz[m] = _readd(f, byte_order)

    # Solution on computational mesh -- loop velocities
    # 1-based: index [0] unused
    U = [0.0] * (NumberOfVortexLoops + 1)
    V = [0.0] * (NumberOfVortexLoops + 1)
    W = [0.0] * (NumberOfVortexLoops + 1)

    for m in range(1, NumberOfVortexLoops + 1):
        U[m] = _readd(f, byte_order)
        V[m] = _readd(f, byte_order)
        W[m] = _readd(f, byte_order)

    # Solution on input mesh -- Cp and Gamma per triangle
    # 1-based: index [0] unused
    Cp         = [0.0] * (NumberOfTris + 1)
    CpUnsteady = [0.0] * (NumberOfTris + 1)
    Gamma      = [0.0] * (NumberOfTris + 1)

    for m in range(1, NumberOfTris + 1):
        Cp[m]         = _readf(f, byte_order)
        CpUnsteady[m] = _readf(f, byte_order)
        Gamma[m]      = _readf(f, byte_order)

    # Read in the wake location data
    NumberOfTrailingVortexEdges = _readi(f, byte_order)

    # 1-based: index [0] unused
    WingWakeNode                = [0]   * (NumberOfTrailingVortexEdges + 1)
    SWake                       = [0.0] * (NumberOfTrailingVortexEdges + 1)
    NumberOfSubVortexNodesForEdge = [0] * (NumberOfTrailingVortexEdges + 1)
    XWake = [None] * (NumberOfTrailingVortexEdges + 1)
    YWake = [None] * (NumberOfTrailingVortexEdges + 1)
    ZWake = [None] * (NumberOfTrailingVortexEdges + 1)

    for i in range(1, NumberOfTrailingVortexEdges + 1):
        WingWakeNode[i] = _readi(f, byte_order)   # Kutta node index
        SWake[i]        = _readd(f, byte_order)   # span location (s/b)
        NumberOfSubVortexNodesForEdge[i] = _readi(f, byte_order)

        n = NumberOfSubVortexNodesForEdge[i]
        # 1-based node coordinate arrays for this filament
        xw = [0.0] * (n + 1)
        yw = [0.0] * (n + 1)
        zw = [0.0] * (n + 1)

        for j in range(1, n + 1):
            xw[j] = _readd(f, byte_order)
            yw[j] = _readd(f, byte_order)
            zw[j] = _readd(f, byte_order)

        XWake[i] = xw
        YWake[i] = yw
        ZWake[i] = zw

    # Read in any control surface deflection data
    for i in range(1, NumberOfControlSurfaces + 1):
        ControlSurface[i]['DeflectionAngle'] = _readf(f, byte_order) / TORAD  # radians -> degrees

    soln = {
        'MachList':                     MachList,
        'AlphaList':                    AlphaList,   # degrees
        'BetaList':                     BetaList,    # degrees
        'CpMinSoln':                    CpMinSoln,
        'CpMaxSoln':                    CpMaxSoln,
        'GammaN':                       GammaN,
        'dCp_Unsteady':                 dCp_Unsteady,
        'Fx':                           Fx,
        'Fy':                           Fy,
        'Fz':                           Fz,
        'U':                            U,
        'V':                            V,
        'W':                            W,
        'Cp':                           Cp,
        'CpUnsteady':                   CpUnsteady,
        'Gamma':                        Gamma,
        'NumberOfTrailingVortexEdges':  NumberOfTrailingVortexEdges,
        'WingWakeNode':                 WingWakeNode,
        'SWake':                        SWake,
        'NumberOfSubVortexNodesForEdge': NumberOfSubVortexNodesForEdge,
        'XWake':                        XWake,
        'YWake':                        YWake,
        'ZWake':                        ZWake,
    }
    return soln


# ---------------------------------------------------------------------------
# Companion *.adb.cases text file reader
# Mirrors: ADBSLICER::LoadSolutionCaseList() in Adb2Load/ADBSlicer.C
#          GL_VIEWER::LoadSolutionCaseList()  in Viewer/glviewer.C
# ---------------------------------------------------------------------------

def read_cases_file(filename):
    """Read the companion *.adb.cases text file.

    Mirrors ADBSLICER::LoadSolutionCaseList() and
    GL_VIEWER::LoadSolutionCaseList().

    The file has one line per solved case in the order they appear in the
    *.adb file:
        "%10.7f %10.7f %10.7f    %-200s"
        Field 1: Mach
        Field 2: Alpha in DEGREES
        Field 3: Beta  in DEGREES
        Field 4: Case comment string (up to 200 chars)

    Returns a 1-based list: index [0] is None; real entries start at [1].

    Args:
        filename (str): Path to the *.adb.cases file.

    Returns:
        list: 1-based list of dicts, each with keys 'Mach', 'Alpha', 'Beta',
              'CommentLine'.  Index [0] is None.
    """
    with open(filename, 'r') as f:
        lines = [line for line in f if line.strip()]

    NumberOfADBCases = len(lines)

    # 1-based: index [0] is None
    ADBCaseList = [None] * (NumberOfADBCases + 1)

    for i, line in enumerate(lines, start=1):
        parts = line.split(None, 3)
        Mach  = float(parts[0])
        Alpha = float(parts[1])
        Beta  = float(parts[2])
        CommentLine = parts[3].rstrip('\n') if len(parts) > 3 else ''
        ADBCaseList[i] = CaseRecord(
            Mach        = Mach,
            Alpha       = Alpha,       # degrees
            Beta        = Beta,        # degrees
            CommentLine = CommentLine,
        )

    return ADBCaseList


# ---------------------------------------------------------------------------
# ADBFile -- top-level reader class
# ---------------------------------------------------------------------------

class ADBFile:
    """Reader for a VSPAERO *.adb binary file.

    Mirrors the two-pass approach used by both C++ readers:
      1. ``LoadMeshData()``   -- reads the header once and records the file
                               position at the start of the case data.
      2. ``LoadSolutionData(Case)`` -- seeks back to the recorded position
                               and reads geometry + solution for each case.

    Usage::

        adb = ADBFile('mycase.adb')
        print(adb.NumberOfCases)
        geom, soln = adb.LoadCase(1)

    Attributes:
        Header (ADBHeader):  Parsed file header.
        NumberOfCases (int): Number of solved cases in the file.
        CaseList (list):     1-based list of CaseRecord from the companion
                             *.adb.cases file, or None if that file is not found.
        CaseCpBounds (list): 1-based list of (CpMinSoln, CpMaxSoln) tuples as
                             reported by the solver for each case.
        CaseCpDataBounds (list): 1-based list of (cp_min, cp_max) tuples computed
                             from the actual surface Cp data for each case.  Unlike
                             CaseCpBounds these reflect the true data range rather
                             than any conservative solver defaults.
        CaseCpAutoRange (list): 1-based list of (lo, hi) display range tuples
                             computed as mean +/- 1 standard deviation of the surface
                             Cp values, clamped to the actual data range.  Mirrors
                             GL_VIEWER::FindSolutionMinMax() in the C++ Viewer.
                             Use ``clim = adb.global_cp_auto_range()`` for a
                             consistent outlier-robust colormap range across a sweep.
        ByteSwapForADB (bool): True if byte-swapping was needed.
        FILE_VERSION (int):  2 or 3.
        QuadData (QuadFile or None): Quad cutting-plane reader loaded from
                             the companion ``*.quad.cases`` file, or None if
                             that file is not present.
    """

    def __init__(self, filename):
        """Open and partially parse an *.adb file.

        Reads the header, records the start-of-cases file position, then
        scans all cases to determine NumberOfCases.  Does not load solution
        data -- call LoadCase() for that.

        Args:
            filename (str): Path to the *.adb file.
        """
        self._filename = filename
        self.CaseList  = None

        with open(filename, 'rb') as f:

            # --- Detect endianness (mirrors LoadMeshData endian check) ---
            byte_order, FILE_VERSION = _detect_endianness(f)
            self._byte_order = byte_order
            self.FILE_VERSION = FILE_VERSION
            self.ByteSwapForADB = (byte_order == '>')

            # --- Read header ---
            # Keep the raw dict for internal reader functions; expose the
            # dataclass as the public interface.
            self._raw_header = _read_header(f, byte_order, FILE_VERSION)
            self.Header = make_header(self._raw_header)

            # --- Record start of case data (mirrors fgetpos after header) ---
            self._StartOfCaseData = f.tell()

        # --- Scan to count cases and collect per-case Cp bounds ---
        self.NumberOfCases     = self._scan_case_count()
        self.CaseCpBounds      = self._CaseCpBounds      # public alias (1-based)
        self.CaseCpDataBounds  = self._CaseCpDataBounds  # public alias (1-based)
        self.CaseCpAutoRange   = self._CaseCpAutoRange   # public alias (1-based)

        # --- Load companion *.adb.cases file if present ---
        cases_filename = filename + '.cases'
        try:
            self.CaseList = read_cases_file(cases_filename)
        except FileNotFoundError:
            pass

        # --- Load quad cutting-plane catalog if present ---
        from .quad_reader import QuadFile
        self.QuadData = None
        try:
            self.QuadData = QuadFile(filename)
        except FileNotFoundError:
            pass

    # ------------------------------------------------------------------

    def _scan_case_count(self):
        """Count the number of cases by reading through the entire file.

        Each case consists of a geometry block followed by a solution block.
        Both are variable-length, so we must actually parse them to find case
        boundaries.  Byte offsets for each case are recorded in
        ``self._CaseOffsets`` (1-based; index [0] unused) for use by
        LoadCase().

        Returns:
            int: Number of cases found.
        """
        self._CaseOffsets       = [None]   # index [0] unused; offsets are 1-based
        self._CaseCpBounds      = [None]   # index [0] unused; (CpMinSoln, CpMaxSoln) per case
        self._CaseCpDataBounds  = [None]   # index [0] unused; actual data range per case
        self._CaseCpAutoRange   = [None]   # index [0] unused; mean +/- 1 sigma display range per case

        n_tris = self._raw_header['NumberOfTris']

        with open(self._filename, 'rb') as f:
            f.seek(self._StartOfCaseData)

            while True:
                offset = f.tell()

                # Try to read a geometry block; stop if we hit EOF
                try:
                    geom = _read_geometry(f, self._raw_header, self._byte_order)
                except struct.error:
                    break

                # Read the corresponding solution block
                try:
                    soln = _read_solution(f, self._raw_header, geom, self._byte_order)
                except struct.error:
                    break

                self._CaseOffsets.append(offset)
                self._CaseCpBounds.append(
                    (soln['CpMinSoln'], soln['CpMaxSoln'])
                )

                # Compute per-case statistics from surface-only triangles.
                # TriList_surface_type[i] > 0 for wing/body tris; == 0 for wake sheet.
                st  = np.array(geom['TriList_surface_type'][1:n_tris + 1], dtype=np.int32)
                cp  = np.array(soln['Cp'][1:n_tris + 1],                   dtype=np.float32)
                mask = st > 0
                if mask.any():
                    cp_surf = cp[mask]
                    actual_min = float(cp_surf.min())
                    actual_max = float(cp_surf.max())
                    self._CaseCpDataBounds.append((actual_min, actual_max))

                    # Mean +/- 1 sigma display range, clamped to actual data bounds.
                    # Mirrors GL_VIEWER::FindSolutionMinMax() in the C++ Viewer:
                    # this trims statistical outliers so the majority of the
                    # colormap range captures the physically meaningful variation.
                    avg = float(cp_surf.mean())
                    std = float(cp_surf.std())
                    auto_lo = max(avg - std, actual_min)
                    auto_hi = min(avg + std, actual_max)
                    # Thick (PANEL) bodies always have a stagnation point where
                    # Cp = 1.0.  Ensure the display range always includes +1 so
                    # the stagnation region is never clipped below the colormap.
                    if self._raw_header['ModelType'] == PANEL_MODEL:
                        auto_hi = max(auto_hi, 1.0)
                    self._CaseCpAutoRange.append((auto_lo, auto_hi))
                else:
                    self._CaseCpDataBounds.append(
                        (soln['CpMinSoln'], soln['CpMaxSoln'])
                    )
                    self._CaseCpAutoRange.append(
                        (soln['CpMinSoln'], soln['CpMaxSoln'])
                    )

        return len(self._CaseOffsets) - 1   # subtract the dummy [0] entry

    # ------------------------------------------------------------------

    def global_cp_bounds(self):
        """Return the Cp range that spans all cases in the file.

        Takes the minimum of all per-case CpMinSoln values and the maximum
        of all per-case CpMaxSoln values, giving a single (lo, hi) pair
        suitable for use as a consistent colormap range across a sweep or
        animation.

        Returns:
            tuple: (cp_min, cp_max) covering the full range of all cases,
                   or None if no cases are present.
        """
        bounds = self._CaseCpBounds[1:]   # strip dummy [0]
        if not bounds:
            return None
        cp_min = min(b[0] for b in bounds)
        cp_max = max(b[1] for b in bounds)
        return (cp_min, cp_max)

    # ------------------------------------------------------------------

    def global_cp_data_bounds(self):
        """Return the data-driven Cp range that spans all cases in the file.

        Unlike global_cp_bounds(), which uses the solver-reported CpMinSoln /
        CpMaxSoln values (which may be conservative defaults), this method
        computes the envelope of the actual surface Cp data values recorded
        during the scan pass.  This gives a tight, physically meaningful range
        that is suitable as the default colormap limits for sweep plots and
        animations.

        Returns:
            tuple: (cp_min, cp_max) covering the actual Cp data across all
                   cases, or None if no cases are present.
        """
        bounds = self._CaseCpDataBounds[1:]   # strip dummy [0]
        if not bounds:
            return None
        cp_min = min(b[0] for b in bounds)
        cp_max = max(b[1] for b in bounds)
        return (cp_min, cp_max)

    # ------------------------------------------------------------------

    def global_cp_auto_range(self):
        """Return an outlier-robust Cp display range that spans all cases.

        Takes the envelope of the per-case mean +/- 1 sigma ranges stored in
        CaseCpAutoRange.  Each per-case range trims statistical outliers
        (spurious high/low Cp values on isolated triangles) so the majority
        of the colormap captures the physically meaningful variation.  The
        envelope then gives a single (lo, hi) pair that is consistent across
        a sweep or animation.

        This mirrors the GL_VIEWER::FindSolutionMinMax() logic in the C++
        Viewer, which uses mean +/- 1 sigma of surface Cp values to set the default
        colormap range.

        Returns:
            tuple: (lo, hi) outlier-robust range covering all cases,
                   or None if no cases are present.
        """
        bounds = self._CaseCpAutoRange[1:]   # strip dummy [0]
        if not bounds:
            return None
        lo = min(b[0] for b in bounds)
        hi = max(b[1] for b in bounds)
        return (lo, hi)

    # ------------------------------------------------------------------

    def LoadCase(self, Case):
        """Load geometry and solution data for a specific case.

        Mirrors GL_VIEWER::LoadExistingSolutionData(Case) and
        ADBSLICER::LoadSolutionData(Case).

        Args:
            Case (int): 1-based case number (1 .. NumberOfCases).

        Returns:
            tuple: (ADBGeometry, ADBSolution) for the requested case.

        Raises:
            IndexError: If Case is out of range.
        """
        if Case < 1 or Case > self.NumberOfCases:
            raise IndexError(
                f'Case {Case} out of range; file contains '
                f'{self.NumberOfCases} case(s).'
            )

        with open(self._filename, 'rb') as f:
            f.seek(self._CaseOffsets[Case])
            raw_geom = _read_geometry(f, self._raw_header, self._byte_order)
            raw_soln = _read_solution(f, self._raw_header, raw_geom, self._byte_order)

        return make_geometry(raw_geom), make_solution(raw_soln)

    # ------------------------------------------------------------------

    def __iter__(self):
        """Iterate over all cases, yielding (geom, soln) for each.

        Walks cases 1 through NumberOfCases in order without holding all
        data in memory simultaneously.

        Yields:
            tuple: (ADBGeometry, ADBSolution) for each case.
        """
        for Case in range(1, self.NumberOfCases + 1):
            yield self.LoadCase(Case)

    # ------------------------------------------------------------------

    def __len__(self):
        return self.NumberOfCases

    # ------------------------------------------------------------------

    def __repr__(self):
        return (
            f'ADBFile({self._filename!r}, '
            f'version={self.FILE_VERSION}, '
            f'cases={self.NumberOfCases})'
        )
