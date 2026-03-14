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

"""Unit tests for pyvspaero.quad_reader (QuadFile) and quad_data (QuadSlicePlane)."""

import numpy as np
import pytest

from pyvspaero import QuadFile
from pyvspaero.quad_data import QuadPlaneDef, QuadSlicePlane


# ---------------------------------------------------------------------------
# Fixtures: QuadFile objects opened from the *.adb path
# ---------------------------------------------------------------------------

@pytest.fixture(scope='module')
def wing_quad(wing_adb_path):
    """QuadFile loaded from Wing.adb (1 cutting plane)."""
    return QuadFile(wing_adb_path)


@pytest.fixture(scope='module')
def wbd_quad(adb_path):
    """QuadFile loaded from WingBodyDisk.adb (5 cutting planes)."""
    return QuadFile(adb_path)


@pytest.fixture(scope='module')
def wing_case1_planes(wing_quad):
    """Wing QuadFile LoadCase(1) -- 1-based list of QuadSlicePlane."""
    return wing_quad.LoadCase(1)


# ---------------------------------------------------------------------------
# QuadFile -- Wing (1 plane, 3 cases)
# ---------------------------------------------------------------------------

def test_quad_file_opens_from_adb_path(wing_quad):
    """QuadFile opens without error when given an *.adb path."""
    assert wing_quad is not None


def test_quad_number_of_planes_wing(wing_quad):
    """Wing.adb has 1 quad cutting plane."""
    assert wing_quad.NumberOfPlanes == 1


def test_quad_number_of_cases_wing(wing_quad):
    """Wing QuadFile discovers 3 cases from the dat files."""
    assert wing_quad.NumberOfCases == 3


def test_quad_repr_contains_plane_and_case_counts(wing_quad):
    """repr() contains both NumberOfPlanes and NumberOfCases."""
    r = repr(wing_quad)
    assert 'planes=1' in r
    assert 'cases=3' in r


def test_quad_plane_list_is_one_based(wing_quad):
    """PlaneList has NumberOfPlanes+1 entries; PlaneList[0] is None."""
    assert len(wing_quad.PlaneList) == wing_quad.NumberOfPlanes + 1
    assert wing_quad.PlaneList[0] is None


def test_quad_plane_def_type(wing_quad):
    """PlaneList[1] is a QuadPlaneDef instance."""
    assert isinstance(wing_quad.PlaneList[1], QuadPlaneDef)


def test_quad_plane_direction_is_y(wing_quad):
    """Wing cutting plane is perpendicular to Y (direction == 2)."""
    assert wing_quad.PlaneList[1].direction == 2


def test_quad_plane_value_near_zero(wing_quad):
    """Wing cutting plane lies at Y=0."""
    assert wing_quad.PlaneList[1].value == pytest.approx(0.0, abs=1e-4)


def test_quad_plane_axis_name_y(wing_quad):
    """PlaneList[1].axis_name == 'Y'."""
    assert wing_quad.PlaneList[1].axis_name == 'Y'


# ---------------------------------------------------------------------------
# QuadFile.LoadCase -- Wing case 1
# ---------------------------------------------------------------------------

def test_load_case_returns_one_based_list(wing_case1_planes):
    """LoadCase returns a list with NumberOfPlanes+1 entries; index [0] is None."""
    assert len(wing_case1_planes) == 2   # [None, plane1]
    assert wing_case1_planes[0] is None


def test_load_case_plane_is_quad_slice_plane(wing_case1_planes):
    """LoadCase(1)[1] is a QuadSlicePlane instance."""
    assert isinstance(wing_case1_planes[1], QuadSlicePlane)


def test_quad_slice_direction(wing_case1_planes):
    """QuadSlicePlane direction matches the catalog entry (2 = Y)."""
    assert wing_case1_planes[1].direction == 2


def test_quad_slice_value_near_zero(wing_case1_planes):
    """QuadSlicePlane value is Y=0."""
    assert wing_case1_planes[1].value == pytest.approx(0.0, abs=1e-4)


def test_quad_slice_axis_name_y(wing_case1_planes):
    """QuadSlicePlane.axis_name == 'Y'."""
    assert wing_case1_planes[1].axis_name == 'Y'


def test_quad_slice_number_of_nodes(wing_case1_planes):
    """Wing case 1 plane has 25 nodes."""
    assert wing_case1_planes[1].NumberOfNodes == 25


def test_quad_slice_number_of_cells(wing_case1_planes):
    """Wing case 1 plane has 16 quad cells."""
    assert wing_case1_planes[1].NumberOfCells == 16


def test_quad_slice_xyz_shape(wing_case1_planes):
    """xyz array has shape (NumberOfNodes+1, 3) -- 1-based."""
    p = wing_case1_planes[1]
    assert p.xyz.shape == (p.NumberOfNodes + 1, 3)


def test_quad_slice_velocity_shape(wing_case1_planes):
    """velocity array has shape (NumberOfNodes+1, 3) -- 1-based."""
    p = wing_case1_planes[1]
    assert p.velocity.shape == (p.NumberOfNodes + 1, 3)


def test_quad_slice_cp_shape(wing_case1_planes):
    """Cp array has shape (NumberOfNodes+1,) -- 1-based."""
    p = wing_case1_planes[1]
    assert p.Cp.shape == (p.NumberOfNodes + 1,)


def test_quad_slice_cells_shape(wing_case1_planes):
    """cells array has shape (NumberOfCells+1, 4) -- 1-based."""
    p = wing_case1_planes[1]
    assert p.cells.shape == (p.NumberOfCells + 1, 4)


def test_quad_slice_cell_indices_in_range(wing_case1_planes):
    """All cell node indices are in [1, NumberOfNodes]."""
    p = wing_case1_planes[1]
    assert np.all(p.cells[1:] >= 1)
    assert np.all(p.cells[1:] <= p.NumberOfNodes)


def test_quad_slice_velocity_magnitude_shape(wing_case1_planes):
    """velocity_magnitude has shape (NumberOfNodes+1,)."""
    p = wing_case1_planes[1]
    assert p.velocity_magnitude.shape == (p.NumberOfNodes + 1,)


def test_quad_slice_velocity_magnitude_index_zero(wing_case1_planes):
    """velocity_magnitude[0] == 0.0 (unused padding index)."""
    assert wing_case1_planes[1].velocity_magnitude[0] == pytest.approx(0.0)


def test_quad_slice_velocity_magnitude_nonnegative(wing_case1_planes):
    """All velocity magnitudes at valid nodes are non-negative."""
    vmag = wing_case1_planes[1].velocity_magnitude
    assert np.all(vmag[1:] >= 0.0)


def test_quad_slice_xyz_finite(wing_case1_planes):
    """All node coordinates are finite (no NaN or Inf)."""
    p = wing_case1_planes[1]
    assert np.all(np.isfinite(p.xyz[1:]))


def test_quad_vref_positive(wing_case1_planes):
    """Vref (freestream velocity) is positive."""
    assert wing_case1_planes[1].Vref > 0.0


# ---------------------------------------------------------------------------
# QuadFile.LoadCase -- invalid indices raise IndexError
# ---------------------------------------------------------------------------

def test_load_case_zero_raises(wing_quad):
    """LoadCase(0) raises IndexError."""
    with pytest.raises(IndexError):
        wing_quad.LoadCase(0)


def test_load_case_too_high_raises(wing_quad):
    """LoadCase(N+1) raises IndexError when only N cases exist."""
    with pytest.raises(IndexError):
        wing_quad.LoadCase(wing_quad.NumberOfCases + 1)


# ---------------------------------------------------------------------------
# QuadFile -- WingBodyDisk (5 planes, 3 cases)
# ---------------------------------------------------------------------------

def test_wbd_quad_number_of_planes(wbd_quad):
    """WingBodyDisk.adb has 5 quad cutting planes."""
    assert wbd_quad.NumberOfPlanes == 5


def test_wbd_quad_number_of_cases(wbd_quad):
    """WingBodyDisk QuadFile discovers 3 cases."""
    assert wbd_quad.NumberOfCases == 3


def test_wbd_load_case_returns_six_entry_list(wbd_quad):
    """WingBodyDisk LoadCase(1) returns a list of length 6 (5 planes + None at [0])."""
    planes = wbd_quad.LoadCase(1)
    assert len(planes) == 6
    assert planes[0] is None


def test_wbd_all_planes_are_quad_slice_planes(wbd_quad):
    """All entries in WingBodyDisk LoadCase(1) are QuadSlicePlane instances."""
    planes = wbd_quad.LoadCase(1)
    for i in range(1, 6):
        assert isinstance(planes[i], QuadSlicePlane)


# ---------------------------------------------------------------------------
# QuadFile constructor -- missing file
# ---------------------------------------------------------------------------

def test_quad_file_missing_raises(tmp_path):
    """QuadFile raises FileNotFoundError when the *.quad.cases file is absent."""
    with pytest.raises(FileNotFoundError):
        QuadFile(str(tmp_path / 'nonexistent.adb'))
