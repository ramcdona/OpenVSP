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
pyvspaero_viewer.ui.solution_tab -- SolutionTab (PySide6 QWidget).

The Solution panel contains only case navigation and component visibility.
All display-mode controls (scalar field, colormap, shading, mesh, wake, etc.)
live in the application menu bar.
"""

from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QComboBox,
    QCheckBox, QButtonGroup, QRadioButton,
    QListWidget, QListWidgetItem,
    QFrame, QGroupBox, QPushButton,
)
from PySide6.QtCore import Signal, Qt


# ---------------------------------------------------------------------------
# Shared utility helpers (also imported by cutplanes_tab)
# ---------------------------------------------------------------------------

def _set_combo_by_value(combo, value):
    for i in range(combo.count()):
        if combo.itemData(i) == value:
            combo.setCurrentIndex(i)
            return


def _radio_row(parent_layout, options):
    """Create a row of QRadioButtons; return (QButtonGroup, {value: button})."""
    group = QButtonGroup()
    row   = QHBoxLayout()
    row.setContentsMargins(0, 0, 0, 0)
    buttons = {}
    for label, value in options:
        btn = QRadioButton(label)
        btn.setProperty('value', value)
        group.addButton(btn)
        row.addWidget(btn)
        buttons[value] = btn
    row.addStretch()
    parent_layout.addLayout(row)
    return group, buttons


# ---------------------------------------------------------------------------
# SolutionTab
# ---------------------------------------------------------------------------

class SolutionTab(QWidget):
    """Case navigation and component visibility panel.

    Signals
    -------
    state_changed : Signal()
        Emitted when the component visibility list changes.
    case_step_requested : Signal(int)
        Emitted with +1 (next) or -1 (prev) when navigation buttons are clicked.
    """

    state_changed       = Signal()
    case_step_requested = Signal(int)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._setting_state = False

        layout = QVBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(4)
        layout.addWidget(self._build_case_nav())
        layout.addWidget(self._build_components_group(), 1)

        self._connect_signals()

    # ------------------------------------------------------------------
    # Section builders
    # ------------------------------------------------------------------

    def _build_case_nav(self):
        frame = QFrame()
        frame.setFrameShape(QFrame.Shape.StyledPanel)
        layout = QVBoxLayout(frame)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(2)

        nav_row = QHBoxLayout()
        self._prev_btn   = QPushButton('<')
        self._prev_btn.setFixedWidth(32)
        self._case_label = QLabel('Case 1 / 1')
        self._case_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._next_btn   = QPushButton('>')
        self._next_btn.setFixedWidth(32)
        nav_row.addWidget(self._prev_btn)
        nav_row.addWidget(self._case_label, 1)
        nav_row.addWidget(self._next_btn)
        layout.addLayout(nav_row)

        mab_row = QHBoxLayout()
        self._mach_label  = QLabel('M=0.0000')
        self._alpha_label = QLabel('Alpha=0.00 deg')
        self._beta_label  = QLabel('Beta=0.00 deg')
        for lbl in (self._mach_label, self._alpha_label, self._beta_label):
            lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
            mab_row.addWidget(lbl)
        layout.addLayout(mab_row)
        return frame

    def _build_components_group(self):
        gb = QGroupBox('Components')
        layout = QVBoxLayout(gb)
        layout.setSpacing(4)

        btn_row = QHBoxLayout()
        self._select_all_btn   = QPushButton('Select All')
        self._unselect_all_btn = QPushButton('Unselect All')
        btn_row.addWidget(self._select_all_btn)
        btn_row.addWidget(self._unselect_all_btn)
        layout.addLayout(btn_row)

        self._component_list = QListWidget()
        layout.addWidget(self._component_list, 1)
        return gb

    # ------------------------------------------------------------------
    # Signal wiring
    # ------------------------------------------------------------------

    def _connect_signals(self):
        self._prev_btn.clicked.connect(lambda: self.case_step_requested.emit(-1))
        self._next_btn.clicked.connect(lambda: self.case_step_requested.emit(+1))
        self._select_all_btn.clicked.connect(self._select_all)
        self._unselect_all_btn.clicked.connect(self._unselect_all)
        self._component_list.itemChanged.connect(self._emit_changed)

    def _select_all(self):
        self._component_list.blockSignals(True)
        for i in range(self._component_list.count()):
            self._component_list.item(i).setCheckState(Qt.CheckState.Checked)
        self._component_list.blockSignals(False)
        self._emit_changed()

    def _unselect_all(self):
        self._component_list.blockSignals(True)
        for i in range(self._component_list.count()):
            self._component_list.item(i).setCheckState(Qt.CheckState.Unchecked)
        self._component_list.blockSignals(False)
        self._emit_changed()

    def _emit_changed(self, *_):
        if not self._setting_state:
            self.state_changed.emit()

    # ------------------------------------------------------------------
    # Public interface
    # ------------------------------------------------------------------

    def get_state(self) -> dict:
        """Return component visibility list."""
        surface_items = []
        for i in range(self._component_list.count()):
            item = self._component_list.item(i)
            surface_items.append({
                'id':      item.data(Qt.ItemDataRole.UserRole),
                'name':    item.text(),
                'visible': item.checkState() == Qt.CheckState.Checked,
            })
        return {'surface_items': surface_items}

    def set_state(self, state: dict) -> None:
        """Populate widgets from *state* without triggering rebuilds."""
        self._setting_state = True
        try:
            self.populate_components(state.get('surface_items', []))
            self.update_case_display(
                state.get('case_index', 1),
                state.get('n_cases', 1),
                state.get('case_mach', 0.0),
                state.get('case_alpha', 0.0),
                state.get('case_beta', 0.0),
            )
        finally:
            self._setting_state = False

    def populate_components(self, surface_items: list) -> None:
        """Rebuild the component visibility list."""
        self._component_list.blockSignals(True)
        self._component_list.clear()
        for item_dict in surface_items:
            item = QListWidgetItem(item_dict['name'])
            item.setData(Qt.ItemDataRole.UserRole, item_dict['id'])
            item.setFlags(
                Qt.ItemFlag.ItemIsEnabled | Qt.ItemFlag.ItemIsUserCheckable
            )
            item.setCheckState(
                Qt.CheckState.Checked if item_dict.get('visible', True)
                else Qt.CheckState.Unchecked
            )
            self._component_list.addItem(item)
        self._component_list.blockSignals(False)

    def update_case_display(self, case_index: int, n_cases: int,
                            mach: float, alpha: float, beta: float) -> None:
        """Update the case navigation labels (does not trigger state_changed)."""
        self._case_label.setText(f'Case {case_index} / {n_cases}')
        self._mach_label.setText(f'M={mach:.4f}')
        self._alpha_label.setText(f'Alpha={alpha:.2f} deg')
        self._beta_label.setText(f'Beta={beta:.2f} deg')
        self._prev_btn.setEnabled(case_index > 1)
        self._next_btn.setEnabled(case_index < n_cases)

    def update_data_range_label(self, lo: float, hi: float) -> None:
        """No-op: data range is shown in the application status bar."""
