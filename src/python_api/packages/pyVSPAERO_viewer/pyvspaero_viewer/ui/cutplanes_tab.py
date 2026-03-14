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
pyvspaero_viewer.ui.cutplanes_tab -- CutPlanesTab (PySide6 QWidget).
"""

from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QComboBox,
    QCheckBox, QButtonGroup, QDoubleSpinBox,
    QSpinBox, QListWidget, QListWidgetItem, QScrollArea,
    QFrame, QGroupBox, QSlider, QPushButton,
)
from PySide6.QtCore import Signal, Qt

from pyvspaero_viewer.ui.solution_tab import _set_combo_by_value, _radio_row


class CutPlanesTab(QWidget):
    """Cut planes controls sidebar tab.

    Signals
    -------
    state_changed : Signal()
        Emitted whenever any control changes.
    """

    state_changed = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._setting_state = False

        outer = QVBoxLayout(self)
        outer.setContentsMargins(4, 4, 4, 4)
        outer.setSpacing(4)

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.Shape.NoFrame)
        inner = QWidget()
        content = QVBoxLayout(inner)
        content.setContentsMargins(0, 0, 4, 0)
        content.setSpacing(4)
        content.addWidget(self._build_planes_group())
        content.addWidget(self._build_field_group())
        content.addWidget(self._build_options_group())
        content.addStretch()
        scroll.setWidget(inner)
        outer.addWidget(scroll, 1)

        self._connect_signals()

    # ------------------------------------------------------------------
    # Section builders
    # ------------------------------------------------------------------

    def _build_planes_group(self):
        gb = QGroupBox('Planes')
        layout = QVBoxLayout(gb)
        layout.setSpacing(4)

        btn_row = QHBoxLayout()
        self._planes_select_all_btn   = QPushButton('Select All')
        self._planes_unselect_all_btn = QPushButton('Unselect All')
        btn_row.addWidget(self._planes_select_all_btn)
        btn_row.addWidget(self._planes_unselect_all_btn)
        layout.addLayout(btn_row)

        self._plane_list = QListWidget()
        self._plane_list.setFixedHeight(160)
        layout.addWidget(self._plane_list)
        return gb

    def _build_field_group(self):
        gb = QGroupBox('Field')
        layout = QVBoxLayout(gb)
        layout.setSpacing(4)

        self._quad_scalar_combo = QComboBox()
        for label, value in [('Cp', 'Cp'), ('V magnitude', 'Vmag')]:
            self._quad_scalar_combo.addItem(label, value)
        layout.addWidget(self._quad_scalar_combo)

        self._quad_shading_group, _ = _radio_row(
            layout, [('Smooth', 'smooth'), ('Flat', 'flat')]
        )

        opacity_row = QHBoxLayout()
        opacity_row.addWidget(QLabel('Opacity'))
        self._opacity_label = QLabel('100%')
        self._opacity_label.setFixedWidth(36)
        opacity_row.addStretch()
        opacity_row.addWidget(self._opacity_label)
        layout.addLayout(opacity_row)

        self._opacity_slider = QSlider(Qt.Orientation.Horizontal)
        self._opacity_slider.setRange(0, 100)
        self._opacity_slider.setValue(100)
        self._opacity_slider.setTickInterval(25)
        layout.addWidget(self._opacity_slider)

        self._opacity_slider.valueChanged.connect(
            lambda v: self._opacity_label.setText(f'{v}%')
        )
        return gb

    def _build_options_group(self):
        gb = QGroupBox('Options')
        layout = QVBoxLayout(gb)
        layout.setSpacing(2)

        self._show_quad_mesh_cb = QCheckBox('Show mesh')
        layout.addWidget(self._show_quad_mesh_cb)

        contour_row = QHBoxLayout()
        self._show_quad_contours_cb = QCheckBox('Contour lines   N=')
        self._n_quad_contours_spin  = QSpinBox()
        self._n_quad_contours_spin.setRange(2, 50)
        self._n_quad_contours_spin.setValue(10)
        self._n_quad_contours_spin.setFixedWidth(48)
        contour_row.addWidget(self._show_quad_contours_cb)
        contour_row.addWidget(self._n_quad_contours_spin)
        layout.addLayout(contour_row)

        vec_row = QHBoxLayout()
        self._show_quad_vectors_cb   = QCheckBox('Velocity vectors   scale=')
        self._quad_vector_scale_spin = QDoubleSpinBox()
        self._quad_vector_scale_spin.setRange(0.1, 20.0)
        self._quad_vector_scale_spin.setSingleStep(0.1)
        self._quad_vector_scale_spin.setDecimals(1)
        self._quad_vector_scale_spin.setFixedWidth(56)
        vec_row.addWidget(self._show_quad_vectors_cb)
        vec_row.addWidget(self._quad_vector_scale_spin)
        layout.addLayout(vec_row)

        self._show_quad_contours_cb.toggled.connect(self._n_quad_contours_spin.setEnabled)
        self._show_quad_vectors_cb.toggled.connect(self._quad_vector_scale_spin.setEnabled)
        return gb

    # ------------------------------------------------------------------
    # Signal wiring
    # ------------------------------------------------------------------

    def _connect_signals(self):
        self._planes_select_all_btn.clicked.connect(self._select_all_planes)
        self._planes_unselect_all_btn.clicked.connect(self._unselect_all_planes)

        self._quad_scalar_combo.currentIndexChanged.connect(self._emit_changed)
        self._opacity_slider.sliderReleased.connect(self._emit_changed)

        for cb in (self._show_quad_mesh_cb, self._show_quad_contours_cb,
                   self._show_quad_vectors_cb):
            cb.stateChanged.connect(self._emit_changed)

        for btn in self._quad_shading_group.buttons():
            btn.toggled.connect(lambda checked: checked and self._emit_changed())

        self._n_quad_contours_spin.valueChanged.connect(self._emit_changed)
        self._quad_vector_scale_spin.valueChanged.connect(self._emit_changed)
        self._plane_list.itemChanged.connect(self._emit_changed)

    def _select_all_planes(self):
        self._plane_list.blockSignals(True)
        for i in range(self._plane_list.count()):
            self._plane_list.item(i).setCheckState(Qt.CheckState.Checked)
        self._plane_list.blockSignals(False)
        self._emit_changed()

    def _unselect_all_planes(self):
        self._plane_list.blockSignals(True)
        for i in range(self._plane_list.count()):
            self._plane_list.item(i).setCheckState(Qt.CheckState.Unchecked)
        self._plane_list.blockSignals(False)
        self._emit_changed()

    def _emit_changed(self, *_):
        if not self._setting_state:
            self.state_changed.emit()

    # ------------------------------------------------------------------
    # Public interface
    # ------------------------------------------------------------------

    def get_state(self) -> dict:
        """Return a dict of all cut-planes tab state keys."""
        shading_btn = self._quad_shading_group.checkedButton()

        quad_planes_enabled = []
        for i in range(self._plane_list.count()):
            item = self._plane_list.item(i)
            quad_planes_enabled.append(item.checkState() == Qt.CheckState.Checked)

        return {
            'quad_planes_enabled':    quad_planes_enabled,
            'quad_scalar':            self._quad_scalar_combo.currentData(),
            'quad_shading':           shading_btn.property('value') if shading_btn else 'smooth',
            'quad_opacity':           self._opacity_slider.value() / 100.0,
            'show_quad_mesh':         self._show_quad_mesh_cb.isChecked(),
            'show_quad_contours':     self._show_quad_contours_cb.isChecked(),
            'n_quad_contour_levels':  self._n_quad_contours_spin.value(),
            'show_quad_vectors':      self._show_quad_vectors_cb.isChecked(),
            'quad_vector_scale':      self._quad_vector_scale_spin.value(),
        }

    def set_state(self, state: dict) -> None:
        """Populate all widgets from *state* without triggering rebuilds."""
        self._setting_state = True
        try:
            _set_combo_by_value(self._quad_scalar_combo, state.get('quad_scalar', 'Cp'))

            shading = state.get('quad_shading', 'smooth')
            for btn in self._quad_shading_group.buttons():
                if btn.property('value') == shading:
                    btn.setChecked(True)

            opacity_pct = int(state.get('quad_opacity', 1.0) * 100)
            self._opacity_slider.setValue(opacity_pct)
            self._opacity_label.setText(f'{opacity_pct}%')

            self._show_quad_mesh_cb.setChecked(state.get('show_quad_mesh', False))
            self._show_quad_contours_cb.setChecked(state.get('show_quad_contours', False))
            self._n_quad_contours_spin.setValue(state.get('n_quad_contour_levels', 10))
            self._n_quad_contours_spin.setEnabled(state.get('show_quad_contours', False))
            self._show_quad_vectors_cb.setChecked(state.get('show_quad_vectors', False))
            self._quad_vector_scale_spin.setValue(state.get('quad_vector_scale', 1.0))
            self._quad_vector_scale_spin.setEnabled(state.get('show_quad_vectors', False))

            self.populate_planes(
                state.get('quad_plane_labels', []),
                state.get('quad_planes_enabled', []),
            )
        finally:
            self._setting_state = False

    def populate_planes(self, labels: list, enabled: list = None) -> None:
        """Rebuild the plane list from *labels*."""
        self._plane_list.blockSignals(True)
        self._plane_list.clear()
        for i, label in enumerate(labels):
            item = QListWidgetItem(label)
            item.setFlags(Qt.ItemFlag.ItemIsEnabled | Qt.ItemFlag.ItemIsUserCheckable)
            checked = enabled[i] if enabled and i < len(enabled) else False
            item.setCheckState(
                Qt.CheckState.Checked if checked else Qt.CheckState.Unchecked
            )
            self._plane_list.addItem(item)
        self._plane_list.blockSignals(False)
