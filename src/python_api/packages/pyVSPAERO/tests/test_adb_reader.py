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

"""Unit tests for pyvspaero.adb_reader (ADBFile public interface)."""

import pytest

from pyvspaero import ADBFile
from pyvspaero.adb_data import ADBGeometry, ADBHeader, ADBSolution


# ---------------------------------------------------------------------------
# Basic open / metadata
# ---------------------------------------------------------------------------

def test_adb_opens(adb_file):
    """ADBFile opens WingBodyDisk.adb without raising."""
    assert adb_file is not None


def test_number_of_cases(adb_file):
    """WingBodyDisk.adb contains 3 solved cases."""
    assert adb_file.NumberOfCases == 3


def test_len_equals_number_of_cases(adb_file):
    """len(adb) == NumberOfCases."""
    assert len(adb_file) == adb_file.NumberOfCases


def test_repr_contains_filename_and_case_count(adb_file):
    """repr() includes the filename stem and the case count."""
    r = repr(adb_file)
    assert 'WingBodyDisk' in r
    assert '3' in r


def test_file_version_is_valid(adb_file):
    """FILE_VERSION is 2 or 3."""
    assert adb_file.FILE_VERSION in (2, 3)


def test_no_byte_swap_on_little_endian_host(adb_file):
    """ByteSwapForADB is False on a native little-endian host."""
    assert adb_file.ByteSwapForADB is False


# ---------------------------------------------------------------------------
# Header
# ---------------------------------------------------------------------------

def test_header_is_adb_header_instance(adb_file):
    """adb.Header is an ADBHeader dataclass."""
    assert isinstance(adb_file.Header, ADBHeader)


def test_header_has_positive_reference_quantities(adb_file):
    """Sref, Cref, and Bref are all positive."""
    h = adb_file.Header
    assert h.Sref > 0.0
    assert h.Cref > 0.0
    assert h.Bref > 0.0


def test_header_symmetry_flag_valid(adb_file):
    """SymmetryFlag is 0 (no symmetry) or 1 (symmetry)."""
    assert adb_file.Header.SymmetryFlag in (0, 1)


# ---------------------------------------------------------------------------
# CaseList (companion *.adb.cases file)
# ---------------------------------------------------------------------------

def test_case_list_loaded(adb_file):
    """CaseList is populated from the companion *.adb.cases text file."""
    assert adb_file.CaseList is not None


def test_case_list_is_one_based(adb_file):
    """CaseList has NumberOfCases+1 entries; CaseList[0] is None."""
    assert len(adb_file.CaseList) == adb_file.NumberOfCases + 1
    assert adb_file.CaseList[0] is None


def test_case_list_mach_positive(adb_file):
    """Mach in every CaseRecord is positive."""
    for i in range(1, adb_file.NumberOfCases + 1):
        assert adb_file.CaseList[i].Mach > 0.0


# ---------------------------------------------------------------------------
# Per-case Cp bounds arrays
# ---------------------------------------------------------------------------

def test_case_cp_bounds_is_one_based(adb_file):
    """CaseCpBounds has NumberOfCases+1 entries; index [0] is None."""
    assert len(adb_file.CaseCpBounds) == adb_file.NumberOfCases + 1
    assert adb_file.CaseCpBounds[0] is None


def test_case_cp_data_bounds_is_one_based(adb_file):
    """CaseCpDataBounds has NumberOfCases+1 entries; index [0] is None."""
    assert len(adb_file.CaseCpDataBounds) == adb_file.NumberOfCases + 1
    assert adb_file.CaseCpDataBounds[0] is None


# ---------------------------------------------------------------------------
# LoadCase
# ---------------------------------------------------------------------------

def test_load_case_returns_geometry_and_solution(adb_file):
    """LoadCase(1) returns an (ADBGeometry, ADBSolution) pair."""
    geom, soln = adb_file.LoadCase(1)
    assert isinstance(geom, ADBGeometry)
    assert isinstance(soln, ADBSolution)


def test_load_case_zero_raises_index_error(adb_file):
    """LoadCase(0) raises IndexError -- below the 1-based valid range."""
    with pytest.raises(IndexError):
        adb_file.LoadCase(0)


def test_load_case_too_high_raises_index_error(adb_file):
    """LoadCase(N+1) raises IndexError when only N cases exist."""
    with pytest.raises(IndexError):
        adb_file.LoadCase(adb_file.NumberOfCases + 1)


def test_load_case_all_three_differ_in_alpha(adb_file):
    """The three cases have distinct Alpha values."""
    alphas = {adb_file.LoadCase(i)[1].Alpha for i in range(1, 4)}
    assert len(alphas) == 3


# ---------------------------------------------------------------------------
# Iteration
# ---------------------------------------------------------------------------

def test_iteration_yields_all_cases(adb_file):
    """Iterating over ADBFile yields exactly NumberOfCases pairs."""
    pairs = list(adb_file)
    assert len(pairs) == adb_file.NumberOfCases


def test_iteration_yields_correct_types(adb_file):
    """Every element from iteration is an (ADBGeometry, ADBSolution) pair."""
    for geom, soln in adb_file:
        assert isinstance(geom, ADBGeometry)
        assert isinstance(soln, ADBSolution)


# ---------------------------------------------------------------------------
# Global Cp range helpers
# ---------------------------------------------------------------------------

def test_global_cp_bounds_lo_lt_hi(adb_file):
    """global_cp_bounds() returns (lo, hi) with lo < hi."""
    lo, hi = adb_file.global_cp_bounds()
    assert lo < hi


def test_global_cp_data_bounds_lo_lt_hi(adb_file):
    """global_cp_data_bounds() returns (lo, hi) with lo < hi."""
    lo, hi = adb_file.global_cp_data_bounds()
    assert lo < hi


def test_global_cp_auto_range_lo_lt_hi(adb_file):
    """global_cp_auto_range() returns (lo, hi) with lo < hi."""
    lo, hi = adb_file.global_cp_auto_range()
    assert lo < hi


def test_global_cp_auto_range_panel_hi_clamped_to_one(adb_file):
    """global_cp_auto_range() clamps hi to 1.0 for a Panel (thick-body) model."""
    lo, hi = adb_file.global_cp_auto_range()
    assert hi == pytest.approx(1.0, abs=1e-5)


# ---------------------------------------------------------------------------
# QuadData auto-load
# ---------------------------------------------------------------------------

def test_quad_data_loaded_when_cases_file_present(adb_file):
    """QuadData is populated when a *.quad.cases file exists next to the adb."""
    assert adb_file.QuadData is not None


# ---------------------------------------------------------------------------
# Wing (VLM) and WingBody (Panel) cross-checks
# ---------------------------------------------------------------------------

def test_wing_model_type_vlm(wing_adb_file):
    """Wing.adb has ModelType == 1 (VLM)."""
    assert wing_adb_file.Header.ModelType == 1


def test_wing_number_of_cases(wing_adb_file):
    """Wing.adb contains 3 solved cases."""
    assert wing_adb_file.NumberOfCases == 3


def test_wing_two_surfaces(wing_adb_file):
    """Wing.adb has 2 aerodynamic surfaces."""
    assert wing_adb_file.Header.NumberOfSurfaces == 2


def test_wingbody_model_type_panel(wingbody_adb_file):
    """WingBody.adb has ModelType == 2 (Panel)."""
    assert wingbody_adb_file.Header.ModelType == 2


def test_wingbody_three_surfaces(wingbody_adb_file):
    """WingBody.adb has 3 aerodynamic surfaces."""
    assert wingbody_adb_file.Header.NumberOfSurfaces == 3


def test_wingbodydisk_three_surfaces(adb_file):
    """WingBodyDisk.adb has 3 aerodynamic surfaces."""
    assert adb_file.Header.NumberOfSurfaces == 3
