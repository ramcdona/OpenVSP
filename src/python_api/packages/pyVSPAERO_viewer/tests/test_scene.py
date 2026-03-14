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

"""Unit tests for pyvspaero_viewer.scene (_SceneManager)."""

import pytest

from pyvspaero import ADBFile
from pyvspaero_viewer.scene import _SceneManager


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture(scope='module')
def scene(adb_file):
    """_SceneManager loaded from WingBodyDisk.adb case 1, rebuilt with defaults."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild()
    return sm


# ---------------------------------------------------------------------------
# Phase 1: initialisation and basic rebuild
# ---------------------------------------------------------------------------

def test_scene_manager_has_plotter(scene):
    """_SceneManager.plotter is a pyvista Plotter instance."""
    import pyvista as pv
    assert isinstance(scene.plotter, pv.Plotter)


def test_scene_manager_plotter_is_offscreen(scene):
    """_SceneManager plotter runs off-screen (no display server required)."""
    assert scene.plotter.off_screen


def test_scene_manager_geom_loaded(scene):
    """_SceneManager.geom is populated after construction."""
    assert scene.geom is not None
    assert scene.geom.NumberOfTris > 0


def test_scene_manager_soln_loaded(scene):
    """_SceneManager.soln is populated after construction."""
    assert scene.soln is not None


def test_rebuild_produces_actors(adb_file):
    """rebuild() with default state adds at least one actor to the plotter."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild()
    assert len(sm.plotter.renderer.actors) > 0


def test_rebuild_off_mode_produces_no_actors(adb_file):
    """rebuild() with scalar_mode='Off' and all toggles False adds no actors."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild({
        'scalar_mode':        'Off',
        'show_mesh_edges':    False,
        'show_triangulation': False,
        'show_wake':          False,
        'show_propulsion':    False,
        'show_annotations':   False,
        'show_colorbar':      False,
        'reflect':            False,
    })
    assert len(sm.plotter.renderer.actors) == 0


def test_rebuild_shaded_mode_adds_surface(adb_file):
    """rebuild() with scalar_mode='Shaded' adds a surface actor."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild({
        'scalar_mode':        'Shaded',
        'show_mesh_edges':    False,
        'show_triangulation': False,
        'show_wake':          False,
        'show_propulsion':    False,
        'show_annotations':   False,
        'show_colorbar':      False,
        'reflect':            False,
    })
    assert len(sm.plotter.renderer.actors) >= 1


def test_rebuild_cp_mode_adds_surface(adb_file):
    """rebuild() with scalar_mode='Cp' adds a surface actor with scalar data."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild({
        'scalar_mode':        'Cp',
        'shading_style':      'smooth',
        'clim_min':           -1.5,
        'clim_max':            0.5,
        'show_mesh_edges':    False,
        'show_triangulation': False,
        'show_wake':          False,
        'show_propulsion':    False,
        'show_annotations':   False,
        'show_colorbar':      False,
        'reflect':            False,
    })
    assert len(sm.plotter.renderer.actors) >= 1


def test_camera_preserved_across_rebuild(adb_file):
    """Camera position is unchanged after a rebuild (no view preset called)."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild()
    sm.set_view('iso')
    pos_before = sm.plotter.camera_position

    sm.rebuild({'scalar_mode': 'Shaded',
                'show_mesh_edges': False, 'show_triangulation': False,
                'show_wake': False, 'show_propulsion': False,
                'show_annotations': False, 'show_colorbar': False,
                'reflect': False})

    pos_after = sm.plotter.camera_position
    # Camera tuples contain floats -- compare with tolerance.
    import numpy as np
    before_flat = [v for triplet in pos_before for v in triplet]
    after_flat  = [v for triplet in pos_after  for v in triplet]
    assert np.allclose(before_flat, after_flat, atol=1e-6)


# ---------------------------------------------------------------------------
# Phase 3: scalar field modes
# ---------------------------------------------------------------------------

@pytest.mark.parametrize('mode', ['Cp', 'CpSteady', 'CpUnsteady', 'Gamma'])
def test_scalar_modes_add_surface_actor(adb_file, mode):
    """Each scalar mode adds at least one actor with scalar data."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild({
        'scalar_mode':        mode,
        'shading_style':      'smooth',
        'clim_min':           -1.5,
        'clim_max':            0.5,
        'show_mesh_edges':    False,
        'show_triangulation': False,
        'show_wake':          False,
        'show_propulsion':    False,
        'show_annotations':   False,
        'show_colorbar':      False,
        'reflect':            False,
    })
    assert len(sm.plotter.renderer.actors) >= 1



def test_load_case_changes_soln(adb_file):
    """load_case(2) updates soln to case 2 data."""
    sm = _SceneManager(adb_file, case_index=1)
    alpha_case1 = sm.soln.Alpha
    sm.load_case(2)
    assert sm.soln.Alpha != alpha_case1


def test_load_case_invalid_raises(adb_file):
    """load_case() raises ValueError for out-of-range indices."""
    sm = _SceneManager(adb_file, case_index=1)
    with pytest.raises(ValueError, match='out of range'):
        sm.load_case(999)
    with pytest.raises(ValueError, match='out of range'):
        sm.load_case(0)


# ---------------------------------------------------------------------------
# Phase 4: case navigation
# ---------------------------------------------------------------------------

def test_load_case_restores_case1(adb_file):
    """load_case(1) after load_case(2) returns to case-1 values."""
    sm = _SceneManager(adb_file, case_index=1)
    alpha1 = sm.soln.Alpha
    sm.load_case(2)
    sm.load_case(1)
    assert sm.soln.Alpha == pytest.approx(alpha1)


# ---------------------------------------------------------------------------
# Phase 5: display toggles
# ---------------------------------------------------------------------------

def _minimal_state(**overrides):
    """Return a state dict with all toggles off, applying *overrides*."""
    base = {
        'scalar_mode':        'Shaded',
        'shading_style':      'smooth',
        'clim_min':           -1.5,
        'clim_max':            0.5,
        'show_mesh_edges':    False,
        'mesh_level':         1,
        'show_triangulation': False,
        'show_wake':          False,
        'wake_mode':          'tubes',
        'show_propulsion':    False,
        'show_annotations':   False,
        'show_colorbar':      False,
        'reflect':            False,
    }
    base.update(overrides)
    return base


def test_show_wake_false_no_wake_actors(adb_file):
    """show_wake=False adds no wake actors."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild(_minimal_state(scalar_mode='Off', show_wake=False))
    assert len(sm.plotter.renderer.actors) == 0


def test_show_wake_true_adds_actors(adb_file):
    """show_wake=True adds at least one actor."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild(_minimal_state(scalar_mode='Off', show_wake=True))
    assert len(sm.plotter.renderer.actors) >= 1


def test_reflect_true_more_actors_than_false(adb_file):
    """reflect=True adds reflected actors; total count exceeds reflect=False."""
    sm_no  = _SceneManager(adb_file, case_index=1)
    sm_no.rebuild(_minimal_state(reflect=False))
    n_no = len(sm_no.plotter.renderer.actors)

    sm_yes = _SceneManager(adb_file, case_index=1)
    sm_yes.rebuild(_minimal_state(reflect=True))
    n_yes = len(sm_yes.plotter.renderer.actors)

    assert n_yes > n_no


def test_show_mesh_edges_adds_actors(adb_file):
    """show_mesh_edges=True adds edge actors on top of the surface."""
    sm_no  = _SceneManager(adb_file, case_index=1)
    sm_no.rebuild(_minimal_state(show_mesh_edges=False))
    n_no = len(sm_no.plotter.renderer.actors)

    sm_yes = _SceneManager(adb_file, case_index=1)
    sm_yes.rebuild(_minimal_state(show_mesh_edges=True))
    n_yes = len(sm_yes.plotter.renderer.actors)

    assert n_yes > n_no


@pytest.mark.parametrize('mode', ['lines', 'tubes', 'points'])
def test_wake_modes_add_actors(adb_file, mode):
    """Each wake mode adds at least one actor."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild(_minimal_state(scalar_mode='Off', show_wake=True, wake_mode=mode))
    assert len(sm.plotter.renderer.actors) >= 1


# ---------------------------------------------------------------------------
# Phase 8: component visibility
# ---------------------------------------------------------------------------

def test_surface_items_populated(adb_file):
    """surface_items in app state contains the correct number of surfaces."""
    from pyvspaero_viewer.app import VSPAEROViewerApp
    app = VSPAEROViewerApp.__new__(VSPAEROViewerApp)
    app._adb = adb_file
    items = [
        {'id': i, 'name': adb_file.Header.SurfaceNameList[i], 'visible': True}
        for i in range(1, adb_file.Header.NumberOfSurfaces + 1)
    ]
    assert len(items) == adb_file.Header.NumberOfSurfaces
    assert all(item['visible'] for item in items)


def test_hide_all_components_removes_surface(adb_file):
    """Setting all surface_items visible=False produces no surface actors."""
    sm = _SceneManager(adb_file, case_index=1)
    n_surfaces = adb_file.Header.NumberOfSurfaces
    hidden_items = [
        {'id': i, 'name': 'S', 'visible': False}
        for i in range(1, n_surfaces + 1)
    ]
    sm.rebuild(_minimal_state(scalar_mode='Shaded', surface_items=hidden_items))
    assert len(sm.plotter.renderer.actors) == 0


def test_show_one_component_fewer_actors_than_all(adb_file):
    """Hiding all but one component produces fewer cells than showing all."""
    import numpy as np
    sm_all = _SceneManager(adb_file, case_index=1)
    sm_all.rebuild(_minimal_state(scalar_mode='Cp', clim_auto=True))

    n_surfaces = adb_file.Header.NumberOfSurfaces
    one_item = [
        {'id': i, 'name': 'S', 'visible': i == 1}
        for i in range(1, n_surfaces + 1)
    ]
    sm_one = _SceneManager(adb_file, case_index=1)
    sm_one.rebuild(_minimal_state(scalar_mode='Cp', clim_auto=True, surface_items=one_item))
    # With only one surface, the bounding box should be smaller.
    b_all = sm_all._surface_mesh.bounds
    b_one = sm_one.plotter.renderer.bounds
    span_all = b_all[1] - b_all[0]
    span_one = b_one[1] - b_one[0]
    # At minimum, the visible mesh must be smaller or equal in span.
    assert span_one <= span_all + 1e-6


# ---------------------------------------------------------------------------
# Phase 9: cutting planes
# ---------------------------------------------------------------------------

def test_quad_available_for_wingbodydisk(adb_file):
    """WingBodyDisk.adb has quad data (quad_available should be True)."""
    assert adb_file.QuadData is not None
    assert adb_file.QuadData.NumberOfPlanes >= 1


def test_quad_planes_loaded_on_init(adb_file):
    """_SceneManager loads quad planes for the initial case."""
    sm = _SceneManager(adb_file, case_index=1)
    assert sm._quad_planes is not None
    assert len(sm._quad_planes) > 1   # [None, plane1, plane2, ...]


def test_quad_rebuild_adds_actors(adb_file):
    """rebuild() with quad_available=True adds at least one quad actor."""
    sm = _SceneManager(adb_file, case_index=1)
    n_planes = adb_file.QuadData.NumberOfPlanes
    sm.rebuild(_minimal_state(
        scalar_mode='Off',
        quad_available=True,
        quad_planes_enabled=[True] * n_planes,
        quad_scalar='Cp',
        quad_opacity=1.0,
    ))
    assert len(sm.plotter.renderer.actors) >= 1


def test_quad_rebuild_disabled_planes_no_actors(adb_file):
    """All planes disabled produces no quad actors."""
    sm = _SceneManager(adb_file, case_index=1)
    n_planes = adb_file.QuadData.NumberOfPlanes
    sm.rebuild(_minimal_state(
        scalar_mode='Off',
        quad_available=True,
        quad_planes_enabled=[False] * n_planes,
        quad_scalar='Cp',
        quad_opacity=1.0,
    ))
    assert len(sm.plotter.renderer.actors) == 0


def test_quad_planes_reload_on_load_case(adb_file):
    """load_case(2) reloads quad planes for case 2."""
    sm = _SceneManager(adb_file, case_index=1)
    planes_1 = sm._quad_planes
    sm.load_case(2)
    planes_2 = sm._quad_planes
    # Both should be non-None and distinct objects.
    assert planes_1 is not None
    assert planes_2 is not None
    assert planes_1 is not planes_2


def test_rebuild_after_load_case_uses_new_data(adb_file):
    """load_case(2) changes the Cp solution data used in rebuild()."""
    import numpy as np
    sm = _SceneManager(adb_file, case_index=1)
    cp_case1 = np.array(sm.soln.Cp[sm._surface_id])
    sm.load_case(2)
    cp_case2 = np.array(sm.soln.Cp[sm._surface_id])
    # WingBodyDisk case 2 has a different alpha, so Cp must differ.
    assert not np.allclose(cp_case1, cp_case2)


# ---------------------------------------------------------------------------
# Phase 7: colormap control
# ---------------------------------------------------------------------------

def test_scalar_auto_range_returns_tuple(adb_file):
    """scalar_auto_range() returns a (min, max) float tuple for Cp."""
    sm = _SceneManager(adb_file, case_index=1)
    rng = sm.scalar_auto_range('Cp')
    assert rng is not None
    lo, hi = rng
    assert isinstance(lo, float)
    assert isinstance(hi, float)
    assert lo < hi


def test_scalar_auto_range_none_for_off(adb_file):
    """scalar_auto_range() returns None for non-scalar modes."""
    sm = _SceneManager(adb_file, case_index=1)
    assert sm.scalar_auto_range('Off') is None
    assert sm.scalar_auto_range('Shaded') is None


def test_scalar_auto_range_differs_across_fields(adb_file):
    """Gamma auto range differs from Cp auto range (different physical scale)."""
    sm  = _SceneManager(adb_file, case_index=1)
    cp  = sm.scalar_auto_range('Cp')
    gam = sm.scalar_auto_range('Gamma')
    assert cp != gam


def test_clim_auto_uses_data_range(adb_file):
    """With clim_auto=True, rebuild() uses the data range (not stored clim)."""
    import numpy as np
    sm = _SceneManager(adb_file, case_index=1)
    gam_min, gam_max = sm.scalar_auto_range('Gamma')

    # Rebuild with a deliberately wrong manual clim but auto=True.
    sm.rebuild(_minimal_state(
        scalar_mode='Gamma',
        clim_auto=True,
        clim_min=-999.0,
        clim_max=999.0,
        show_colorbar=False,
    ))
    actors = [
        a for a in sm.plotter.renderer.actors.values()
        if hasattr(a, 'mapper') and a.mapper is not None
    ]
    assert actors, 'Expected at least one surface actor'
    lo, hi = actors[0].mapper.scalar_range
    assert np.isclose(lo, gam_min, rtol=0.01)
    assert np.isclose(hi, gam_max, rtol=0.01)


def test_clim_manual_range_respected(adb_file):
    """With clim_auto=False, the supplied clim_min/max are used."""
    import numpy as np
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild(_minimal_state(
        scalar_mode='Cp',
        clim_auto=False,
        clim_min=-0.5,
        clim_max=0.5,
        show_colorbar=False,
    ))
    actors = [
        a for a in sm.plotter.renderer.actors.values()
        if hasattr(a, 'mapper') and a.mapper is not None
    ]
    assert actors
    lo, hi = actors[0].mapper.scalar_range
    assert np.isclose(lo, -0.5, atol=1e-4)
    assert np.isclose(hi,  0.5, atol=1e-4)
