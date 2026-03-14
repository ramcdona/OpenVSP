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

"""Unit tests for pyvspaero_viewer UI widget modules (SolutionTab, CutPlanesTab).

All tests require the session-scoped ``qt_app`` fixture from conftest.py so
that a QApplication exists before any QWidget is created.
"""

import pytest

from PySide6.QtCore import Qt


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _make_surface_items(names, visible=True):
    """Build a list of surface-item dicts for SolutionTab.set_state."""
    return [
        {'id': i + 1, 'name': name, 'visible': visible}
        for i, name in enumerate(names)
    ]


# ---------------------------------------------------------------------------
# SolutionTab
# ---------------------------------------------------------------------------

class TestSolutionTab:

    @pytest.fixture(autouse=True)
    def widget(self, qt_app):
        """Fresh SolutionTab instance for each test."""
        from pyvspaero_viewer.ui.solution_tab import SolutionTab
        w = SolutionTab()
        w.show()
        yield w
        w.close()

    # ------------------------------------------------------------------
    # Instantiation and signals
    # ------------------------------------------------------------------

    def test_instantiates(self, widget):
        """SolutionTab constructs without raising."""
        assert widget is not None

    def test_has_state_changed_signal(self, widget):
        """SolutionTab exposes a state_changed Signal."""
        assert hasattr(widget, 'state_changed')

    def test_has_case_step_requested_signal(self, widget):
        """SolutionTab exposes a case_step_requested Signal."""
        assert hasattr(widget, 'case_step_requested')

    # ------------------------------------------------------------------
    # get_state
    # ------------------------------------------------------------------

    def test_get_state_returns_dict(self, widget):
        """get_state() returns a dict."""
        state = widget.get_state()
        assert isinstance(state, dict)

    def test_get_state_has_surface_items_key(self, widget):
        """get_state() contains a 'surface_items' key."""
        assert 'surface_items' in widget.get_state()

    def test_get_state_empty_initially(self, widget):
        """get_state()['surface_items'] is empty before any set_state call."""
        assert widget.get_state()['surface_items'] == []

    # ------------------------------------------------------------------
    # populate_components
    # ------------------------------------------------------------------

    def test_populate_adds_items(self, widget):
        """populate_components() adds the expected number of list items."""
        items = _make_surface_items(['Wing', 'Body', 'Tail'])
        widget.populate_components(items)
        assert widget._component_list.count() == 3

    def test_populate_sets_names(self, widget):
        """populate_components() preserves surface names."""
        items = _make_surface_items(['Wing', 'Fuselage'])
        widget.populate_components(items)
        names = [widget._component_list.item(i).text() for i in range(2)]
        assert names == ['Wing', 'Fuselage']

    def test_populate_sets_visibility(self, widget):
        """populate_components() sets check state from 'visible' field."""
        items = [
            {'id': 1, 'name': 'Wing',  'visible': True},
            {'id': 2, 'name': 'Body',  'visible': False},
        ]
        widget.populate_components(items)
        assert widget._component_list.item(0).checkState() == Qt.CheckState.Checked
        assert widget._component_list.item(1).checkState() == Qt.CheckState.Unchecked

    # ------------------------------------------------------------------
    # select / unselect all
    # ------------------------------------------------------------------

    def test_select_all_checks_all(self, widget):
        """Clicking 'Select All' checks every item in the list."""
        widget.populate_components(_make_surface_items(['A', 'B', 'C'], visible=False))
        widget._select_all_btn.click()
        for i in range(3):
            assert widget._component_list.item(i).checkState() == Qt.CheckState.Checked

    def test_unselect_all_unchecks_all(self, widget):
        """Clicking 'Unselect All' unchecks every item in the list."""
        widget.populate_components(_make_surface_items(['A', 'B', 'C'], visible=True))
        widget._unselect_all_btn.click()
        for i in range(3):
            assert widget._component_list.item(i).checkState() == Qt.CheckState.Unchecked

    # ------------------------------------------------------------------
    # update_case_display
    # ------------------------------------------------------------------

    def test_update_case_display_label(self, widget):
        """update_case_display() updates the case label text."""
        widget.update_case_display(2, 3, 0.5, 5.0, 0.0)
        assert '2' in widget._case_label.text()
        assert '3' in widget._case_label.text()

    def test_update_case_display_mach_label(self, widget):
        """update_case_display() updates the Mach label."""
        widget.update_case_display(1, 3, 0.3, 0.0, 0.0)
        assert '0.3' in widget._mach_label.text()

    def test_update_case_display_prev_disabled_at_first(self, widget):
        """Prev button is disabled when case_index == 1."""
        widget.update_case_display(1, 3, 0.0, 0.0, 0.0)
        assert not widget._prev_btn.isEnabled()

    def test_update_case_display_next_disabled_at_last(self, widget):
        """Next button is disabled when case_index == n_cases."""
        widget.update_case_display(3, 3, 0.0, 0.0, 0.0)
        assert not widget._next_btn.isEnabled()

    def test_update_case_display_both_enabled_in_middle(self, widget):
        """Both nav buttons are enabled for a middle case."""
        widget.update_case_display(2, 3, 0.0, 0.0, 0.0)
        assert widget._prev_btn.isEnabled()
        assert widget._next_btn.isEnabled()

    # ------------------------------------------------------------------
    # set_state / get_state round-trip
    # ------------------------------------------------------------------

    def test_set_state_get_state_roundtrip_surface_items(self, widget):
        """set_state followed by get_state preserves component visibility."""
        items = [
            {'id': 1, 'name': 'Wing',  'visible': True},
            {'id': 2, 'name': 'Body',  'visible': False},
            {'id': 3, 'name': 'Tail',  'visible': True},
        ]
        widget.set_state({'surface_items': items})
        result = widget.get_state()['surface_items']
        assert len(result) == 3
        assert result[0]['name'] == 'Wing'
        assert result[0]['visible'] is True
        assert result[1]['name'] == 'Body'
        assert result[1]['visible'] is False
        assert result[2]['visible'] is True

    def test_set_state_does_not_emit_state_changed(self, widget, qt_app):
        """set_state() does not trigger state_changed (suppresses feedback loop)."""
        fired = []
        widget.state_changed.connect(lambda: fired.append(1))
        widget.set_state({'surface_items': _make_surface_items(['Wing'])})
        assert fired == []

    # ------------------------------------------------------------------
    # case_step_requested signal
    # ------------------------------------------------------------------

    def test_next_button_emits_plus_one(self, widget):
        """Clicking the Next button emits case_step_requested(+1)."""
        received = []
        widget.case_step_requested.connect(received.append)
        widget.update_case_display(1, 3, 0.0, 0.0, 0.0)
        widget._next_btn.click()
        assert received == [1]

    def test_prev_button_emits_minus_one(self, widget):
        """Clicking the Prev button emits case_step_requested(-1)."""
        received = []
        widget.case_step_requested.connect(received.append)
        widget.update_case_display(2, 3, 0.0, 0.0, 0.0)
        widget._prev_btn.click()
        assert received == [-1]


# ---------------------------------------------------------------------------
# CutPlanesTab
# ---------------------------------------------------------------------------

class TestCutPlanesTab:

    @pytest.fixture(autouse=True)
    def widget(self, qt_app):
        """Fresh CutPlanesTab instance for each test."""
        from pyvspaero_viewer.ui.cutplanes_tab import CutPlanesTab
        w = CutPlanesTab()
        w.show()
        yield w
        w.close()

    # ------------------------------------------------------------------
    # Instantiation and signals
    # ------------------------------------------------------------------

    def test_instantiates(self, widget):
        """CutPlanesTab constructs without raising."""
        assert widget is not None

    def test_has_state_changed_signal(self, widget):
        """CutPlanesTab exposes a state_changed Signal."""
        assert hasattr(widget, 'state_changed')

    # ------------------------------------------------------------------
    # get_state default values
    # ------------------------------------------------------------------

    def test_get_state_returns_dict(self, widget):
        """get_state() returns a dict."""
        assert isinstance(widget.get_state(), dict)

    def test_get_state_has_required_keys(self, widget):
        """get_state() contains all expected state keys."""
        state = widget.get_state()
        required = {
            'quad_scalar', 'quad_shading', 'quad_opacity',
            'show_quad_mesh', 'show_quad_contours', 'n_quad_contour_levels',
            'show_quad_vectors', 'quad_vector_scale', 'quad_planes_enabled',
        }
        assert required.issubset(state.keys())

    def test_default_scalar_is_cp(self, widget):
        """Default scalar selection is 'Cp'."""
        assert widget.get_state()['quad_scalar'] == 'Cp'

    def test_default_opacity_is_one(self, widget):
        """Default opacity is 1.0 (slider at 100)."""
        assert widget.get_state()['quad_opacity'] == 1.0

    def test_default_mesh_hidden(self, widget):
        """Mesh overlay is hidden by default."""
        assert widget.get_state()['show_quad_mesh'] is False

    def test_default_contours_hidden(self, widget):
        """Contour lines are hidden by default."""
        assert widget.get_state()['show_quad_contours'] is False

    def test_default_vectors_hidden(self, widget):
        """Velocity vectors are hidden by default."""
        assert widget.get_state()['show_quad_vectors'] is False

    # ------------------------------------------------------------------
    # populate_planes
    # ------------------------------------------------------------------

    def test_populate_planes_adds_items(self, widget):
        """populate_planes() adds the expected number of list items."""
        widget.populate_planes(['Y=0.0', 'Y=1.0', 'Y=2.0'])
        assert widget._plane_list.count() == 3

    def test_populate_planes_sets_labels(self, widget):
        """populate_planes() preserves plane label text."""
        widget.populate_planes(['Y=0.0', 'X=1.5'])
        assert widget._plane_list.item(0).text() == 'Y=0.0'
        assert widget._plane_list.item(1).text() == 'X=1.5'

    def test_populate_planes_enabled_flag(self, widget):
        """populate_planes() applies the enabled list to check states."""
        widget.populate_planes(['P1', 'P2', 'P3'], enabled=[True, False, True])
        assert widget._plane_list.item(0).checkState() == Qt.CheckState.Checked
        assert widget._plane_list.item(1).checkState() == Qt.CheckState.Unchecked
        assert widget._plane_list.item(2).checkState() == Qt.CheckState.Checked

    # ------------------------------------------------------------------
    # select / unselect all planes
    # ------------------------------------------------------------------

    def test_select_all_planes(self, widget):
        """'Select All' checks every plane in the list."""
        widget.populate_planes(['P1', 'P2'], enabled=[False, False])
        widget._planes_select_all_btn.click()
        for i in range(2):
            assert widget._plane_list.item(i).checkState() == Qt.CheckState.Checked

    def test_unselect_all_planes(self, widget):
        """'Unselect All' unchecks every plane in the list."""
        widget.populate_planes(['P1', 'P2'], enabled=[True, True])
        widget._planes_unselect_all_btn.click()
        for i in range(2):
            assert widget._plane_list.item(i).checkState() == Qt.CheckState.Unchecked

    # ------------------------------------------------------------------
    # set_state / get_state round-trip
    # ------------------------------------------------------------------

    def test_set_state_get_state_roundtrip_scalar(self, widget):
        """set_state / get_state round-trip preserves quad_scalar."""
        widget.set_state({'quad_scalar': 'Vmag'})
        assert widget.get_state()['quad_scalar'] == 'Vmag'

    def test_set_state_get_state_roundtrip_opacity(self, widget):
        """set_state / get_state round-trip preserves quad_opacity."""
        widget.set_state({'quad_opacity': 0.5})
        assert widget.get_state()['quad_opacity'] == 0.5

    def test_set_state_get_state_roundtrip_show_mesh(self, widget):
        """set_state / get_state round-trip preserves show_quad_mesh."""
        widget.set_state({'show_quad_mesh': True})
        assert widget.get_state()['show_quad_mesh'] is True

    def test_set_state_get_state_roundtrip_contour_levels(self, widget):
        """set_state / get_state round-trip preserves n_quad_contour_levels."""
        widget.set_state({'show_quad_contours': True, 'n_quad_contour_levels': 20})
        assert widget.get_state()['n_quad_contour_levels'] == 20

    def test_set_state_get_state_roundtrip_vector_scale(self, widget):
        """set_state / get_state round-trip preserves quad_vector_scale."""
        widget.set_state({'show_quad_vectors': True, 'quad_vector_scale': 2.5})
        assert widget.get_state()['quad_vector_scale'] == 2.5

    def test_set_state_get_state_roundtrip_planes_enabled(self, widget):
        """set_state / get_state round-trip preserves quad_planes_enabled."""
        widget.set_state({
            'quad_plane_labels':   ['Y=0', 'Y=1'],
            'quad_planes_enabled': [True, False],
        })
        result = widget.get_state()['quad_planes_enabled']
        assert result == [True, False]

    def test_set_state_does_not_emit_state_changed(self, widget, qt_app):
        """set_state() suppresses state_changed to avoid rebuilding the scene."""
        fired = []
        widget.state_changed.connect(lambda: fired.append(1))
        widget.set_state({'quad_scalar': 'Vmag', 'quad_opacity': 0.75})
        assert fired == []

    # ------------------------------------------------------------------
    # Dependent widget enable/disable
    # ------------------------------------------------------------------

    def test_contour_spin_enabled_with_contours(self, widget):
        """n_quad_contours spinbox is enabled when show_quad_contours is True."""
        widget._show_quad_contours_cb.setChecked(True)
        assert widget._n_quad_contours_spin.isEnabled()

    def test_contour_spin_disabled_without_contours(self, widget):
        """n_quad_contours spinbox is disabled after enabling then disabling contours."""
        widget._show_quad_contours_cb.setChecked(True)
        widget._show_quad_contours_cb.setChecked(False)
        assert not widget._n_quad_contours_spin.isEnabled()

    def test_vector_scale_enabled_with_vectors(self, widget):
        """quad_vector_scale spinbox is enabled when show_quad_vectors is True."""
        widget._show_quad_vectors_cb.setChecked(True)
        assert widget._quad_vector_scale_spin.isEnabled()

    def test_vector_scale_disabled_without_vectors(self, widget):
        """quad_vector_scale spinbox is disabled after enabling then disabling vectors."""
        widget._show_quad_vectors_cb.setChecked(True)
        widget._show_quad_vectors_cb.setChecked(False)
        assert not widget._quad_vector_scale_spin.isEnabled()

    # ------------------------------------------------------------------
    # Opacity label sync
    # ------------------------------------------------------------------

    def test_opacity_label_updates_with_slider(self, widget):
        """Moving the opacity slider updates the percentage label."""
        widget._opacity_slider.setValue(75)
        assert '75%' in widget._opacity_label.text()
