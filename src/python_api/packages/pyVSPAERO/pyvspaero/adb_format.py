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
VSPAERO *.adb (Aerothermal Database) File Format Specification
==============================================================

This module documents the binary format of VSPAERO *.adb files.
It is intended as a living specification to be kept in sync with the
C++ source code that reads and writes these files.

When the VSPAERO solver changes the *.adb format, the corresponding
changes must be reflected here so the Python parser can be updated to match.

C++ Source References
---------------------
WRITER (authoritative):
    src/vsp_aero/Solver/VSP_Solver.C
        WriteOutAerothermalDatabaseHeader()
        WriteOutAerothermalDatabaseGeometry()
        WriteOutAerothermalDatabaseSolution()
    src/vsp_aero/Solver/Vortex_Trail.C
        VORTEX_TRAIL::WriteToFile()
    src/vsp_aero/Solver/RotorDisk.C
        ROTOR_DISK::Write_Binary_STP_Data()
    src/vsp_aero/Solver/EngineFace.C
        ENGINE_FACE::Write_Binary_STP_Data()

READERS (authoritative, use as cross-checks):
    src/vsp_aero/Viewer/glviewer.C
        GL_VIEWER::LoadMeshData()
        GL_VIEWER::UpdateMeshData()
        GL_VIEWER::LoadExistingSolutionData()
    src/vsp_aero/Adb2Load/ADBSlicer.C
        ADBSLICER::LoadMeshData()
        ADBSLICER::UpdateMeshData()
        ADBSLICER::LoadSolutionData()

LESS TRUSTED READER (may be out of date):
    src/vsp_aero/Solver/VSP_Solver.C
        ReadInAerothermalDatabaseHeader()
        ReadInAerothermalDatabaseGeometry()
        ReadInAerothermalDatabaseSolution()

File Overview
-------------
The *.adb file is a raw binary file written with no Fortran record markers or
other framing.  All data is written using C fwrite() in the native byte order
of the machine that ran the solver.  An endianness marker at the start of the
file lets readers detect whether byte-swapping is needed.

C++ primitive sizes (portable assumptions, same as the C++ code):
    int    -> 4 bytes  (int32)
    float  -> 4 bytes  (float32)
    double -> 8 bytes  (float64)
    char   -> 1 byte

Python struct format prefix:
    '<'  little-endian (x86 / typical)
    '>'  big-endian

Model type constants (stored as int in the header):
    VLM_MODEL   = 1  (Vortex Lattice Method)
    PANEL_MODEL = 2  (Panel method)

File Version Constants
----------------------
The first int in the file is an endianness / version marker.

    ADB_MAGIC_V2 = -123789456       (file version 2)
    ADB_MAGIC_V3 = -123789453       (file version 3, = -123789456 + 3)

Version 2 vs Version 3 differences:
    Version 3 adds nozzle (engine face) support in the geometry block.
    In version 2, NumberOfNozzles is NOT present; readers must assume 0.
    In version 3, NumberOfNozzles is written immediately after NumberOfRotors.

File Structure
--------------
The file is organized as follows:

    [HEADER BLOCK]              -- written once by WriteOutAerothermalDatabaseHeader()
    [CASE BLOCK 1]              -- written once per solved case / time step
        [GEOMETRY SUB-BLOCK]   -- written by WriteOutAerothermalDatabaseGeometry()
        [SOLUTION SUB-BLOCK]   -- written by WriteOutAerothermalDatabaseSolution()
    [CASE BLOCK 2]
        [GEOMETRY SUB-BLOCK]
        [SOLUTION SUB-BLOCK]
    ...

Key insight: For steady-state parametric sweeps (different Mach/Alpha/Beta),
the geometry is identical across cases, but it is re-written in full for each
case.  For time-accurate analyses the geometry changes each time step because
node positions and wake geometry evolve.

The readers (Viewer, Adb2Load) save the file position immediately after the
header block, then use that position to seek back and re-read the full
geometry block when loading each successive case.

Companion *.adb.cases Text File
---------------------------------
VSPAERO also writes a plain text file alongside the *.adb file:
    <casename>.adb.cases

Each line corresponds to one solved case in the order they appear in the
*.adb file:

    Format (fprintf):
        "%10.7f %10.7f %10.7f    %-200s \n"
        Field 1: Mach           (float, 10.7f)
        Field 2: Alpha (degrees) (float, 10.7f)   <- NOTE: degrees in text file
        Field 3: Beta  (degrees) (float, 10.7f)   <- NOTE: degrees in text file
        Field 4: CaseString     (up to 200 chars)

    NOTE: Alpha and Beta are stored in RADIANS in the binary *.adb file,
    but in DEGREES in the *.adb.cases text file.

"""

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

ADB_MAGIC_V2 = -123789456        # Endianness / version marker for file version 2
ADB_MAGIC_V3 = -123789456 + 3    # Endianness / version marker for file version 3

VLM_MODEL   = 1
PANEL_MODEL = 2

# ---------------------------------------------------------------------------
# Struct format strings
#
# Each string uses Python's struct module syntax.
# The byte-order prefix ('<' or '>') must be prepended at parse time based
# on the detected endianness.
#
# Format letters:
#   'i'  -> int32   (4 bytes)
#   'f'  -> float32 (4 bytes)
#   'd'  -> float64 (8 bytes)
#   's'  -> char    (1 byte); prefix with count, e.g. '100s' for 100 chars
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# SECTION 1: HEADER
# Mirrors: WriteOutAerothermalDatabaseHeader() in Solver/VSP_Solver.C
# Read by: GL_VIEWER::LoadMeshData()       in Viewer/glviewer.C
#          ADBSLICER::LoadMeshData()       in Adb2Load/ADBSlicer.C
# ---------------------------------------------------------------------------

# First int -- endianness/version marker.  Read without byte-swap first;
# if it does not match ADB_MAGIC_V2 or ADB_MAGIC_V3, rewind and apply swap.
FMT_ENDIAN_MARKER = 'i'   # int32: ADB_MAGIC_V2 or ADB_MAGIC_V3

# Immediately following the marker:
FMT_HEADER_SCALARS = (
    'i'   # model_type          int32  VLM_MODEL=1 or PANEL_MODEL=2
    'i'   # do_symmetry_plane   int32  0 or 1
    'i'   # time_accurate       int32  0 (steady) or 1 (time-accurate)
    'i'   # n_vortex_loops      int32  number of loops at the MG (coarse) level
    'i'   # n_nodes             int32  number of nodes at level 0 (fine mesh)
    'i'   # n_tris              int32  number of triangles at level 0
    'i'   # n_edges             int32  number of surface vortex edges at MG level
    'f'   # sref                float  reference area
    'f'   # cref                float  reference chord
    'f'   # bref                float  reference span
    'f'   # xcg                 float  center of gravity X
    'f'   # ycg                 float  center of gravity Y
    'f'   # zcg                 float  center of gravity Z
)
# Followed by the surface list:
FMT_N_SURFACES = 'i'          # int32: number of named surfaces
# For each surface i (1 .. n_surfaces):
FMT_SURFACE_ENTRY = (
    'i'       # surface_id   int32  == i (1-based, matches loop index)
    '100s'    # name         100 bytes (null-padded ASCII string)
    'i'       # component_id int32
)

# ---------------------------------------------------------------------------
# SECTION 2 (per-case): GEOMETRY SUB-BLOCK
# Mirrors: WriteOutAerothermalDatabaseGeometry() in Solver/VSP_Solver.C
# Read by: GL_VIEWER::UpdateMeshData()     in Viewer/glviewer.C
#          ADBSLICER::UpdateMeshData()     in Adb2Load/ADBSlicer.C
# ---------------------------------------------------------------------------

# --- 2a. Triangulated surface mesh (input / fine mesh, level 0) ---
# For each tri j (1 .. n_tris):
FMT_TRI = (
    'i'   # node1              int32  1-based node index
    'i'   # node2              int32  1-based node index
    'i'   # node3              int32  1-based node index
    'i'   # component_id       int32
    'i'   # surface_id         int32
    'i'   # min_valid_time_step int32
    'f'   # area               float
)
# Total bytes per tri: 7 fields = 6*4 + 1*4 = 28 bytes

# --- 2b. Node coordinates (input / fine mesh, level 0) ---
# For each node j (1 .. n_nodes):
FMT_NODE = (
    'f'   # x  float
    'f'   # y  float
    'f'   # z  float
)
# Total bytes per node: 12 bytes

# --- 2c. Propulsion element counts ---
FMT_N_ROTORS  = 'i'   # int32: number of rotor disks
# Version 3 ONLY -- not present in version 2 files:
FMT_N_NOZZLES = 'i'   # int32: number of engine nozzle faces

# --- 2d. Rotor disk data ---
# For each rotor (1 .. n_rotors):
# Mirrors: ROTOR_DISK::Write_Binary_STP_Data() in Solver/RotorDisk.C
FMT_ROTOR = (
    'd'   # xyz_x        float64  rotor center X
    'd'   # xyz_y        float64  rotor center Y
    'd'   # xyz_z        float64  rotor center Z
    'd'   # normal_x     float64  rotor disk normal X
    'd'   # normal_y     float64  rotor disk normal Y
    'd'   # normal_z     float64  rotor disk normal Z
    'd'   # radius       float64  outer radius
    'd'   # hub_radius   float64  hub (inner) radius
    'd'   # rpm          float64  rotational speed (RPM)
    'd'   # ct           float64  thrust coefficient
    'd'   # cp           float64  power coefficient
)
# Total bytes per rotor: 11 doubles = 88 bytes

# --- 2e. Engine nozzle data (version 3 only) ---
# For each nozzle (1 .. n_nozzles):
# Mirrors: ENGINE_FACE::Write_Binary_STP_Data() in Solver/EngineFace.C
FMT_NOZZLE = (
    'd'   # xyz_x     float64  nozzle center X
    'd'   # xyz_y     float64  nozzle center Y
    'd'   # xyz_z     float64  nozzle center Z
    'd'   # normal_x  float64  nozzle face normal X
    'd'   # normal_y  float64  nozzle face normal Y
    'd'   # normal_z  float64  nozzle face normal Z
    'd'   # radius    float64  nozzle radius
)
# Total bytes per nozzle: 7 doubles = 56 bytes

# --- 2f. Multi-grid (coarse) mesh levels ---
FMT_N_LEVELS = 'i'   # int32: total number of mesh levels (MaxLevels)
# For each level (1 .. n_levels):
FMT_LEVEL_HEADER = (
    'i'   # n_coarse_nodes  int32  number of nodes at this level
    'i'   # n_coarse_edges  int32  number of edges at this level
)
# For each coarse node (1 .. n_coarse_nodes):
FMT_COARSE_NODE = (
    'f'   # x  float
    'f'   # y  float
    'f'   # z  float
)
# For each coarse edge (1 .. n_coarse_edges):
FMT_COARSE_EDGE = (
    'i'   # surface_id          int32  negative means this is a boundary/Kutta edge;
          #                            absolute value is the actual surface ID.
    'i'   # min_valid_time_step int32
    'i'   # node1               int32  1-based node index
    'i'   # node2               int32  1-based node index
)
# Total bytes per coarse edge: 16 bytes

# --- 2g. Kutta (trailing edge) data for level 1 ---
FMT_N_KUTTA_EDGES = 'i'   # int32: number of Kutta trailing edges
# For each Kutta TE edge (1 .. n_kutta_edges):
FMT_KUTTA_EDGE = 'i'      # int32: 1-based index into the level-1 edge list

FMT_N_KUTTA_NODES = 'i'   # int32: number of Kutta nodes
# For each Kutta node (1 .. n_kutta_nodes):
FMT_KUTTA_NODE = 'i'      # int32: 1-based trailing-edge node index

# --- 2h. Control surfaces ---
FMT_N_CONTROL_SURFACES = 'i'   # int32: number of control surfaces
# For each control surface k (1 .. n_control_surfaces):
FMT_CONTROL_SURFACE_HEADER = 'i'   # int32: reserved / dummy (always 0).
                                    # The Viewer and Adb2Load readers interpret
                                    # this as "NumberOfControlSurfaceNodes" and
                                    # since it is always 0, no extra node data
                                    # follows.  It is written as 'p = 0' in
                                    # WriteOutAerothermalDatabaseGeometry().
FMT_CONTROL_SURFACE_GEOMETRY = (
    'f'   # hinge_node1_x  float  hinge line point 1, X
    'f'   # hinge_node1_y  float  hinge line point 1, Y
    'f'   # hinge_node1_z  float  hinge line point 1, Z
    'f'   # hinge_node2_x  float  hinge line point 2, X
    'f'   # hinge_node2_y  float  hinge line point 2, Y
    'f'   # hinge_node2_z  float  hinge line point 2, Z
    'f'   # hinge_vec_x    float  hinge axis unit vector X
    'f'   # hinge_vec_y    float  hinge axis unit vector Y
    'f'   # hinge_vec_z    float  hinge axis unit vector Z
)
FMT_N_CONTROL_LOOPS = 'i'   # int32: number of computational loops on this surface
# For each control loop (1 .. n_control_loops):
FMT_CONTROL_LOOP = 'i'      # int32: 1-based loop index

# ---------------------------------------------------------------------------
# SECTION 3 (per-case): SOLUTION SUB-BLOCK
# Mirrors: WriteOutAerothermalDatabaseSolution() in Solver/VSP_Solver.C
# Read by: GL_VIEWER::LoadExistingSolutionData() in Viewer/glviewer.C
#          ADBSLICER::LoadSolutionData()         in Adb2Load/ADBSlicer.C
# ---------------------------------------------------------------------------

# --- 3a. Flow conditions ---
FMT_FLOW_CONDITIONS = (
    'f'   # mach              float  Mach number (dimensionless)
    'f'   # alpha_radians     float  angle of attack  (RADIANS)
    'f'   # beta_radians      float  sideslip angle   (RADIANS)
)
# NOTE: the *.adb.cases companion text file stores Alpha and Beta in DEGREES.
# NOTE: the Viewer converts by dividing by TORAD (= pi/180) on read,
#       i.e.  alpha_degrees = alpha_radians / TORAD  ==  alpha_radians * 180/pi

FMT_CP_RANGE = (
    'f'   # cp_min  float  minimum Cp in this solution
    'f'   # cp_max  float  maximum Cp in this solution
)

# --- 3b. Solution on the computational (MG) mesh ---
# For each vortex loop (1 .. n_vortex_loops):
FMT_VORTEX_LOOP = (
    'd'   # gamma        float64  circulation strength
    'd'   # dcp_unsteady float64  unsteady component of delta-Cp
)

# For each surface vortex edge (1 .. n_edges):
FMT_EDGE_FORCES = (
    'd'   # fx  float64  edge force component X
    'd'   # fy  float64  edge force component Y
    'd'   # fz  float64  edge force component Z
)

# For each vortex loop (1 .. n_vortex_loops):
FMT_LOOP_VELOCITY = (
    'd'   # u  float64  velocity X at loop centroid
    'd'   # v  float64  velocity Y at loop centroid
    'd'   # w  float64  velocity Z at loop centroid
)

# --- 3c. Solution interpolated to the input (fine) mesh ---
# For each tri j (1 .. n_tris):
FMT_TRI_SOLUTION = (
    'f'   # cp            float  total delta-Cp (steady + unsteady)
    'f'   # cp_unsteady   float  unsteady component of Cp
    'f'   # gamma         float  circulation strength (interpolated from MG mesh)
)
# Steady Cp is not stored explicitly; compute as:  cp_steady = cp - cp_unsteady

# --- 3d. Wake geometry ---
FMT_N_TRAILING_VORTICES = 'i'   # int32: total number of trailing vortex filaments
# For each trailing vortex filament (1 .. n_trailing_vortices):
# Mirrors: VORTEX_TRAIL::WriteToFile() in Solver/Vortex_Trail.C
FMT_TRAILING_VORTEX_HEADER = (
    'i'   # te_node   int32   1-based index of the Kutta (trailing edge) node
    'd'   # s_over_b  float64 non-dimensional span location (s/b)
    'i'   # n_nodes   int32   number of nodes along this filament
)
# For each node along the filament (1 .. n_nodes):
FMT_WAKE_NODE = (
    'd'   # x  float64
    'd'   # y  float64
    'd'   # z  float64
)

# --- 3e. Control surface deflections ---
# For each control surface k (1 .. n_control_surfaces):
FMT_CONTROL_DEFLECTION = 'f'   # float  deflection angle (RADIANS)

# ---------------------------------------------------------------------------
# Helper: build a complete struct format string with byte-order prefix
# ---------------------------------------------------------------------------

def _join_fmt(byte_order, *fmt_parts):
    """Join format string parts with a byte-order prefix.

    Args:
        byte_order (str): '<' for little-endian, '>' for big-endian.
        *fmt_parts: Individual format strings or tuples of format chars.

    Returns:
        str: Complete struct format string.

    Example:
        import struct
        fmt = _join_fmt('<', FMT_TRI)
        values = struct.unpack(fmt, data)
    """
    combined = ''.join(
        ''.join(p) if isinstance(p, (list, tuple)) else p
        for p in fmt_parts
    )
    return byte_order + combined


# ---------------------------------------------------------------------------
# Named field lists for documentation and parser convenience
#
# These parallel the FMT_* strings above and provide human-readable
# field names that can be used to build dataclasses or numpy structured
# arrays in the parser.
# ---------------------------------------------------------------------------

HEADER_SCALAR_FIELDS = [
    'model_type',
    'do_symmetry_plane',
    'time_accurate',
    'n_vortex_loops',
    'n_nodes',
    'n_tris',
    'n_edges',
    'sref',
    'cref',
    'bref',
    'xcg',
    'ycg',
    'zcg',
]

SURFACE_ENTRY_FIELDS = [
    'surface_id',
    'name',        # bytes; decode to str and strip null bytes
    'component_id',
]

TRI_FIELDS = [
    'node1',
    'node2',
    'node3',
    'component_id',
    'surface_id',
    'min_valid_time_step',
    'area',
]

NODE_FIELDS = ['x', 'y', 'z']

ROTOR_FIELDS = [
    'xyz_x', 'xyz_y', 'xyz_z',
    'normal_x', 'normal_y', 'normal_z',
    'radius',
    'hub_radius',
    'rpm',
    'ct',
    'cp',
]

NOZZLE_FIELDS = [
    'xyz_x', 'xyz_y', 'xyz_z',
    'normal_x', 'normal_y', 'normal_z',
    'radius',
]

COARSE_EDGE_FIELDS = [
    'surface_id',           # negative value flags boundary/Kutta edge
    'min_valid_time_step',
    'node1',
    'node2',
]

CONTROL_SURFACE_GEOMETRY_FIELDS = [
    'hinge_node1_x', 'hinge_node1_y', 'hinge_node1_z',
    'hinge_node2_x', 'hinge_node2_y', 'hinge_node2_z',
    'hinge_vec_x',   'hinge_vec_y',   'hinge_vec_z',
]

FLOW_CONDITION_FIELDS = [
    'mach',
    'alpha_radians',
    'beta_radians',
]

VORTEX_LOOP_FIELDS = [
    'gamma',
    'dcp_unsteady',
]

EDGE_FORCE_FIELDS = ['fx', 'fy', 'fz']

LOOP_VELOCITY_FIELDS = ['u', 'v', 'w']

TRI_SOLUTION_FIELDS = [
    'cp',
    'cp_unsteady',
    'gamma',
]

TRAILING_VORTEX_HEADER_FIELDS = [
    'te_node',
    's_over_b',
    'n_nodes',
]

WAKE_NODE_FIELDS = ['x', 'y', 'z']

# ---------------------------------------------------------------------------
# Summary of per-field byte sizes for quick offset calculations
# ---------------------------------------------------------------------------
#
# HEADER:
#   endianness_marker:      4 bytes  (1 int32)
#   model_type:             4 bytes
#   do_symmetry_plane:      4 bytes
#   time_accurate:          4 bytes
#   n_vortex_loops:         4 bytes
#   n_nodes:                4 bytes
#   n_tris:                 4 bytes
#   n_edges:                4 bytes
#   sref, cref, bref:      12 bytes
#   xcg, ycg, zcg:         12 bytes
#   n_surfaces:             4 bytes
#   per surface:     4 + 100 + 4 = 108 bytes
#
# GEOMETRY SUB-BLOCK (per case):
#   per tri:        6*4 + 4 = 28 bytes
#   per node:       3*4      = 12 bytes
#   n_rotors:                  4 bytes
#   n_nozzles (v3 only):       4 bytes
#   per rotor:      11*8     = 88 bytes
#   per nozzle:     7*8      = 56 bytes
#   n_levels:                  4 bytes
#   per level header: 2*4    =  8 bytes
#   per coarse node:  3*4    = 12 bytes
#   per coarse edge:  4*4    = 16 bytes
#   n_kutta_edges:             4 bytes
#   per kutta edge:            4 bytes
#   n_kutta_nodes:             4 bytes
#   per kutta node:            4 bytes
#   n_control_surfaces:        4 bytes
#   per control surface:
#       dummy:                 4 bytes
#       hinge data: 9*4      = 36 bytes
#       n_control_loops:       4 bytes
#       per loop:              4 bytes
#
# SOLUTION SUB-BLOCK (per case):
#   mach, alpha, beta:  3*4  = 12 bytes
#   cp_min, cp_max:     2*4  =  8 bytes
#   per vortex loop:    2*8  = 16 bytes
#   per edge:           3*8  = 24 bytes
#   per vortex loop:    3*8  = 24 bytes  (velocities)
#   per tri:            3*4  = 12 bytes
#   n_trailing_vortices:       4 bytes
#   per trailing vortex:
#       te_node:               4 bytes
#       s_over_b:              8 bytes
#       n_nodes:               4 bytes
#       per wake node:  3*8  = 24 bytes
#   per control surface (deflection): 4 bytes
