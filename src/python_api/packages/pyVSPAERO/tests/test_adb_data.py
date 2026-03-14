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

"""Unit tests for pyvspaero data containers (ADBHeader, ADBGeometry, ADBSolution)."""

import numpy as np
import pytest

from pyvspaero.adb_data import (
    ADBGeometry,
    ADBHeader,
    ADBSolution,
    CoarseMeshLevel,
    ControlSurface,
    PropulsionElement,
    WakeFilament,
)


# ---------------------------------------------------------------------------
# Fixtures: load one case from each test file
# ---------------------------------------------------------------------------

@pytest.fixture(scope='module')
def wing_geom_soln(wing_adb_file):
    """Wing.adb case 1 geometry and solution (VLM, no propulsion)."""
    return wing_adb_file.LoadCase(1)


@pytest.fixture(scope='module')
def wingbody_geom_soln(wingbody_adb_file):
    """WingBody.adb case 1 geometry and solution (Panel, no propulsion)."""
    return wingbody_adb_file.LoadCase(1)


@pytest.fixture(scope='module')
def wbd_geom_soln(adb_file):
    """WingBodyDisk.adb case 1 geometry and solution (Panel, propulsion, control surfaces)."""
    return adb_file.LoadCase(1)


# ---------------------------------------------------------------------------
# ADBHeader -- Wing (VLM)
# ---------------------------------------------------------------------------

def test_header_model_type_vlm(wing_adb_file):
    """Wing header reports ModelType == 1 (VLM_MODEL)."""
    assert wing_adb_file.Header.ModelType == 1


def test_header_number_of_surfaces_wing(wing_adb_file):
    """Wing header reports 2 surfaces."""
    assert wing_adb_file.Header.NumberOfSurfaces == 2


def test_header_surface_name_list_length(wing_adb_file):
    """SurfaceNameList has NumberOfSurfaces+1 entries (1-based)."""
    h = wing_adb_file.Header
    assert len(h.SurfaceNameList) == h.NumberOfSurfaces + 1


def test_header_surface_name_list_index_zero_is_none(wing_adb_file):
    """SurfaceNameList[0] is None (unused 1-based padding)."""
    assert wing_adb_file.Header.SurfaceNameList[0] is None


def test_header_surface_names_are_nonempty(wing_adb_file):
    """All surface names are non-empty strings."""
    h = wing_adb_file.Header
    for i in range(1, h.NumberOfSurfaces + 1):
        assert isinstance(h.SurfaceNameList[i], str)
        assert len(h.SurfaceNameList[i]) > 0


def test_header_symmetry_flag_wing(wing_adb_file):
    """Wing header SymmetryFlag is 0 (no symmetry)."""
    assert wing_adb_file.Header.SymmetryFlag == 0


# ---------------------------------------------------------------------------
# ADBHeader -- WingBodyDisk (Panel)
# ---------------------------------------------------------------------------

def test_header_model_type_panel(adb_file):
    """WingBodyDisk header reports ModelType == 2 (PANEL_MODEL)."""
    assert adb_file.Header.ModelType == 2


def test_header_number_of_surfaces_wbd(adb_file):
    """WingBodyDisk header reports 3 surfaces."""
    assert adb_file.Header.NumberOfSurfaces == 3


# ---------------------------------------------------------------------------
# ADBGeometry -- Wing (VLM, 1380 tris, 775 nodes, 2 mesh levels)
# ---------------------------------------------------------------------------

def test_geom_number_of_tris_wing(wing_geom_soln):
    """Wing case 1 geometry has 1380 triangles."""
    geom, _ = wing_geom_soln
    assert geom.NumberOfTris == 1380


def test_geom_number_of_nodes_wing(wing_geom_soln):
    """Wing case 1 geometry has 775 nodes."""
    geom, _ = wing_geom_soln
    assert geom.NumberOfNodes == 775


def test_geom_tri_arrays_shape(wing_geom_soln):
    """Triangle arrays have shape (NumberOfTris+1,) -- 1-based."""
    geom, _ = wing_geom_soln
    n = geom.NumberOfTris
    assert geom.TriList_node1.shape == (n + 1,)
    assert geom.TriList_node2.shape == (n + 1,)
    assert geom.TriList_node3.shape == (n + 1,)
    assert geom.TriList_surface_id.shape == (n + 1,)
    assert geom.TriList_area.shape == (n + 1,)


def test_geom_node_arrays_shape(wing_geom_soln):
    """Node coordinate arrays have shape (NumberOfNodes+1,) -- 1-based."""
    geom, _ = wing_geom_soln
    n = geom.NumberOfNodes
    assert geom.NodeList_x.shape == (n + 1,)
    assert geom.NodeList_y.shape == (n + 1,)
    assert geom.NodeList_z.shape == (n + 1,)


def test_geom_node_coordinates_finite(wing_geom_soln):
    """All node coordinates are finite (no NaN or Inf)."""
    geom, _ = wing_geom_soln
    assert np.all(np.isfinite(geom.NodeList_x[1:]))
    assert np.all(np.isfinite(geom.NodeList_y[1:]))
    assert np.all(np.isfinite(geom.NodeList_z[1:]))


def test_geom_tri_node_indices_in_range(wing_geom_soln):
    """Triangle node indices are in [1, NumberOfNodes]."""
    geom, _ = wing_geom_soln
    n = geom.NumberOfNodes
    for arr in (geom.TriList_node1, geom.TriList_node2, geom.TriList_node3):
        assert np.all(arr[1:] >= 1)
        assert np.all(arr[1:] <= n)


def test_geom_tri_areas_positive(wing_geom_soln):
    """All triangle areas are positive."""
    geom, _ = wing_geom_soln
    assert np.all(geom.TriList_area[1:] > 0.0)


def test_geom_mesh_levels_wing(wing_geom_soln):
    """Wing geometry has 2 coarse mesh levels."""
    geom, _ = wing_geom_soln
    assert geom.NumberOfMeshLevels == 2


def test_geom_coarse_mesh_level_type(wing_geom_soln):
    """CoarseMeshLevels[1] is a CoarseMeshLevel instance."""
    geom, _ = wing_geom_soln
    assert isinstance(geom.CoarseMeshLevels[1], CoarseMeshLevel)


def test_geom_coarse_mesh_level_has_nodes(wing_geom_soln):
    """CoarseMeshLevels[1] has at least one node."""
    geom, _ = wing_geom_soln
    assert geom.CoarseMeshLevels[1].NumberOfNodes > 0


def test_geom_coarse_mesh_index_zero_is_none(wing_geom_soln):
    """CoarseMeshLevels[0] is None (1-based list)."""
    geom, _ = wing_geom_soln
    assert geom.CoarseMeshLevels[0] is None


def test_geom_no_propulsion_wing(wing_geom_soln):
    """Wing geometry has 0 propulsion elements."""
    geom, _ = wing_geom_soln
    assert geom.NumberOfPropulsionElements == 0


def test_geom_no_control_surfaces_wing(wing_geom_soln):
    """Wing geometry has 0 control surfaces."""
    geom, _ = wing_geom_soln
    assert geom.NumberOfControlSurfaces == 0


# ---------------------------------------------------------------------------
# ADBGeometry -- WingBodyDisk (propulsion + control surfaces)
# ---------------------------------------------------------------------------

def test_geom_number_of_tris_wbd(wbd_geom_soln):
    """WingBodyDisk case 1 geometry has 2228 triangles."""
    geom, _ = wbd_geom_soln
    assert geom.NumberOfTris == 2228


def test_geom_number_of_nodes_wbd(wbd_geom_soln):
    """WingBodyDisk case 1 geometry has 1188 nodes."""
    geom, _ = wbd_geom_soln
    assert geom.NumberOfNodes == 1188


def test_geom_one_propulsion_element(wbd_geom_soln):
    """WingBodyDisk geometry has exactly 1 propulsion element."""
    geom, _ = wbd_geom_soln
    assert geom.NumberOfPropulsionElements == 1


def test_geom_propulsion_element_type(wbd_geom_soln):
    """PropulsionElementList[1] is a PropulsionElement with type PROP_ROTOR."""
    geom, _ = wbd_geom_soln
    elem = geom.PropulsionElementList[1]
    assert isinstance(elem, PropulsionElement)
    assert elem.Type == 'PROP_ROTOR'


def test_geom_propulsion_element_xyz_shape(wbd_geom_soln):
    """PropulsionElement xyz has shape (3,)."""
    geom, _ = wbd_geom_soln
    assert geom.PropulsionElementList[1].xyz.shape == (3,)


def test_geom_propulsion_element_radius_positive(wbd_geom_soln):
    """PropulsionElement radius is positive."""
    geom, _ = wbd_geom_soln
    assert geom.PropulsionElementList[1].radius > 0.0


def test_geom_two_control_surfaces(wbd_geom_soln):
    """WingBodyDisk geometry has 2 control surfaces."""
    geom, _ = wbd_geom_soln
    assert geom.NumberOfControlSurfaces == 2


def test_geom_control_surface_type(wbd_geom_soln):
    """ControlSurface[1] is a ControlSurface instance."""
    geom, _ = wbd_geom_soln
    assert isinstance(geom.ControlSurface[1], ControlSurface)


def test_geom_control_surface_hinge_shape(wbd_geom_soln):
    """ControlSurface HingeNode1 has shape (3,)."""
    geom, _ = wbd_geom_soln
    assert geom.ControlSurface[1].HingeNode1.shape == (3,)


def test_geom_control_surface_index_zero_is_none(wbd_geom_soln):
    """ControlSurface[0] is None (1-based list)."""
    geom, _ = wbd_geom_soln
    assert geom.ControlSurface[0] is None


# ---------------------------------------------------------------------------
# ADBSolution -- Wing (VLM, Alpha=0 case)
# ---------------------------------------------------------------------------

def test_soln_alpha_case1_near_zero(wing_geom_soln):
    """Wing case 1 Alpha is approximately 0 degrees."""
    _, soln = wing_geom_soln
    assert soln.Alpha == pytest.approx(0.0, abs=0.01)


def test_soln_mach_near_0001(wing_geom_soln):
    """Wing case 1 Mach is approximately 0.001."""
    _, soln = wing_geom_soln
    assert soln.Mach == pytest.approx(0.001, rel=0.01)


def test_soln_beta_near_zero(wing_geom_soln):
    """Wing case 1 Beta is approximately 0 degrees."""
    _, soln = wing_geom_soln
    assert soln.Beta == pytest.approx(0.0, abs=0.01)


def test_soln_cp_array_shape(wing_geom_soln):
    """Cp array has shape (NumberOfTris+1,) -- 1-based."""
    geom, soln = wing_geom_soln
    assert soln.Cp.shape == (geom.NumberOfTris + 1,)


def test_soln_cp_index_zero_unused(wing_geom_soln):
    """Cp[0] is 0.0 (unused 1-based padding)."""
    _, soln = wing_geom_soln
    assert soln.Cp[0] == pytest.approx(0.0)


def test_soln_cp_steady_equals_cp_minus_cp_unsteady(wing_geom_soln):
    """CpSteady == Cp - CpUnsteady exactly (derived quantity, no numerical error)."""
    _, soln = wing_geom_soln
    residual = np.max(np.abs(soln.CpSteady - (soln.Cp - soln.CpUnsteady)))
    assert residual == pytest.approx(0.0, abs=1e-6)


def test_soln_cp_steady_invariant_on_wingbody(wingbody_geom_soln):
    """CpSteady == Cp - CpUnsteady holds for a Panel model case as well."""
    _, soln = wingbody_geom_soln
    residual = np.max(np.abs(soln.CpSteady - (soln.Cp - soln.CpUnsteady)))
    assert residual == pytest.approx(0.0, abs=1e-6)


def test_soln_gamma_array_shape(wing_geom_soln):
    """Gamma array has shape (NumberOfTris+1,) -- 1-based."""
    geom, soln = wing_geom_soln
    assert soln.Gamma.shape == (geom.NumberOfTris + 1,)


def test_soln_wake_filaments_count(wing_geom_soln):
    """WakeFilaments list has NumberOfTrailingVortexEdges+1 entries (1-based)."""
    _, soln = wing_geom_soln
    assert len(soln.WakeFilaments) == soln.NumberOfTrailingVortexEdges + 1


def test_soln_wake_filaments_index_zero_is_none(wing_geom_soln):
    """WakeFilaments[0] is None (1-based list)."""
    _, soln = wing_geom_soln
    assert soln.WakeFilaments[0] is None


def test_soln_wake_filament_type(wing_geom_soln):
    """WakeFilaments[1] is a WakeFilament instance."""
    _, soln = wing_geom_soln
    assert isinstance(soln.WakeFilaments[1], WakeFilament)


def test_soln_wake_filament_x_shape(wing_geom_soln):
    """WakeFilament.x has shape (NumberOfNodes+1,) -- 1-based."""
    _, soln = wing_geom_soln
    wf = soln.WakeFilaments[1]
    assert wf.x.shape == (wf.NumberOfNodes + 1,)


def test_soln_wing_31_wakes(wing_geom_soln):
    """Wing case 1 has 31 trailing vortex edges."""
    _, soln = wing_geom_soln
    assert soln.NumberOfTrailingVortexEdges == 31


# ---------------------------------------------------------------------------
# ADBSolution -- WingBodyDisk (Panel, non-trivial Cp range)
# ---------------------------------------------------------------------------

def test_soln_wbd_cp_range_plausible(wbd_geom_soln):
    """WingBodyDisk case 1 surface Cp range is physically plausible."""
    geom, soln = wbd_geom_soln
    cp_surf = soln.Cp[1:]
    assert cp_surf.min() > -5.0
    assert cp_surf.max() < 5.0


def test_soln_wbd_three_cases_have_increasing_alpha(adb_file):
    """WingBodyDisk cases 1, 2, 3 have strictly increasing Alpha."""
    alphas = [adb_file.LoadCase(i)[1].Alpha for i in range(1, 4)]
    assert alphas[0] < alphas[1] < alphas[2]
