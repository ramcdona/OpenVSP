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
pyvspaero_viewer.app -- VSPAERODesktopApp (PySide6 QMainWindow).

:class:`VSPAERODesktopApp` is the main application window.  It combines:

- A :class:`pyvistaqt.QtInteractor` as the central 3-D render widget.
- A :class:`~pyvspaero_viewer.scene._SceneManager` that translates a flat
  state dictionary into pyvista actors.
- A menu bar that exposes all display options.
- A right-side dock with two sidebar tabs:

  - :class:`~pyvspaero_viewer.ui.solution_tab.SolutionTab` -- case navigation
    and per-surface visibility.
  - :class:`~pyvspaero_viewer.ui.cutplanes_tab.CutPlanesTab` -- cutting-plane
    controls (visible only when ``*.quad.cases`` data is present).

State management
----------------
All viewer state is stored in a single flat ``dict`` (``self._state``).
Whenever a menu action or panel widget changes, the handler updates ``_state``
and calls ``_scene.rebuild(state)`` to redraw the scene.  This one-way flow
(state -> scene) keeps the scene stateless and easy to reason about.

The state dictionary keys are documented in the initial population inside
``_init_state()``.
"""

import os

from PySide6.QtWidgets import (
    QMainWindow, QDockWidget, QTabWidget, QFileDialog, QApplication,
    QDialog, QDialogButtonBox, QFormLayout, QDoubleSpinBox, QSpinBox,
    QPushButton, QInputDialog, QMessageBox,
)
from PySide6.QtCore import Qt
from PySide6.QtGui import QAction, QActionGroup, QKeySequence
from pyvistaqt import QtInteractor

from pyvspaero import ADBFile
from pyvspaero_viewer.scene import _SceneManager
from pyvspaero_viewer.ui.solution_tab import SolutionTab
from pyvspaero_viewer.ui.cutplanes_tab import CutPlanesTab


_SCALAR_ITEMS = [
    ('Off',                'Off'),
    ('Shaded',             'Shaded'),
    ('Pressures (Cp)',     'Cp'),
    ('Steady Pressures',   'CpSteady'),
    ('Unsteady Pressures', 'CpUnsteady'),
    ('Vorticity',          'Gamma'),
]

_CMAP_ITEMS = [
    ('Red-Blue (default)', 'RdBu_r'),
    ('Red-Blue (fwd)',     'RdBu'),
    ('Cool-Warm',          'coolwarm'),
    ('Seismic',            'seismic'),
    ('Viridis',            'viridis'),
    ('Plasma',             'plasma'),
    ('Jet',                'jet'),
    ('Rainbow',            'rainbow'),
]

_VIEW_ITEMS = [
    ('Iso',    'iso',    'F9'),
    ('Top',    'top',    'F1'),
    ('Bottom', 'bottom', 'F2'),
    ('Front',  'front',  'F3'),
    ('Rear',   'rear',   'F4'),
    ('Left',   'left',   'F5'),
    ('Right',  'right',  'F6'),
]


class VSPAERODesktopApp(QMainWindow):
    """Desktop viewer for a VSPAERO *.adb file.

    Parameters
    ----------
    adb_path : str
        Path to the *.adb file to open.
    case_index : int, optional
        1-based case to display on startup (default 1).

    Examples
    --------
    >>> from PySide6.QtWidgets import QApplication
    >>> import sys
    >>> qt_app = QApplication(sys.argv)
    >>> viewer = VSPAERODesktopApp('path/to/file.adb')
    >>> viewer.show()
    >>> qt_app.exec()
    """

    def __init__(self, adb_path: str, *, case_index: int = 1):
        super().__init__()

        self._adb      = ADBFile(adb_path)
        self._adb_path = adb_path
        n = self._adb.NumberOfCases
        if case_index < 1 or case_index > n:
            raise ValueError(
                f'case_index {case_index} is out of range -- file has {n} case(s).'
            )

        self.setWindowTitle(f'VSPAERO Viewer -- {os.path.basename(adb_path)}')
        self.resize(1400, 800)

        # Central 3-D render widget (also a pv.Plotter subclass).
        self._interactor = QtInteractor(self)
        self.setCentralWidget(self._interactor)

        # Scene manager: receives the QtInteractor as the plotter.
        self._scene = _SceneManager(self._adb, case_index, plotter=self._interactor)

        # Build initial state dict.
        self._state: dict = {}
        self._init_state(case_index)

        # Menu bar (replaces toolbar; all display options live in menus).
        self._build_menus()

        # Keyboard shortcuts for mesh level stepping.
        # pyvista binds +/- to increment_point_size_and_line_width by default;
        # clear those before registering our callbacks so they don't both fire.
        self._interactor.iren.clear_events_for_key('plus')
        self._interactor.iren.clear_events_for_key('minus')
        self._interactor.add_key_event('plus',  self._mesh_level_up)
        self._interactor.add_key_event('equal', self._mesh_level_up)
        self._interactor.add_key_event('minus', self._mesh_level_down)

        # Sync menu check states to the initial state.
        self._sync_menus_to_state()

        # Right dock: case navigation + component list (Solution tab) and
        # optionally Cut Planes tab.
        self._build_dock()

        # Wire panel signals.
        self._solution_tab.state_changed.connect(self._on_panel_state_changed)
        self._solution_tab.case_step_requested.connect(self._on_case_step)
        if self._adb.QuadData is not None:
            self._cutplanes_tab.state_changed.connect(self._on_panel_state_changed)

        # Populate panels with initial state.
        self._solution_tab.set_state(self._state)
        if self._adb.QuadData is not None:
            self._cutplanes_tab.set_state(self._state)

        # Initial scene build.
        self._scene.rebuild(self._state)
        self._scene.set_view('iso')
        self._render()

    # ------------------------------------------------------------------
    # State initialisation
    # ------------------------------------------------------------------

    def _init_state(self, case_index: int) -> None:
        adb    = self._adb
        n      = adb.NumberOfCases
        header = adb.Header
        soln   = self._scene.soln
        geom   = self._scene.geom

        clim_min, clim_max = adb.global_cp_auto_range()

        surface_items = [
            {'id': i, 'name': header.SurfaceNameList[i], 'visible': True}
            for i in range(1, header.NumberOfSurfaces + 1)
        ]

        self._state.update({
            # File / case metadata
            'n_cases':        n,
            'case_index':     case_index,
            'case_mach':      float(soln.Mach),
            'case_alpha':     float(soln.Alpha),
            'case_beta':      float(soln.Beta),
            'n_mesh_levels':  geom.NumberOfMeshLevels,
            'has_symmetry':   bool(header.SymmetryFlag),
            # Surface rendering
            'scalar_mode':    'Cp',
            'shading_style':  'smooth',
            'surface_style':  'solid',
            # Geometry toggles
            'show_mesh_edges':      True,
            'mesh_level':           1,
            'show_triangulation':   False,
            'show_wake':            True,
            'wake_mode':            'tubes',
            'extend_wake_infinity': False,
            'wake_color_mode':      'uniform',
            'show_propulsion':      True,
            'reflect':              bool(header.SymmetryFlag),
            # Background / overlays
            'bg_dark':          False,
            'show_annotations': True,
            'show_cg':          False,
            'show_axes':        False,
            # Colormap
            'clim_auto':       True,
            'clim_min':        float(clim_min),
            'clim_max':        float(clim_max),
            'scalar_data_min': float(clim_min),
            'scalar_data_max': float(clim_max),
            'show_colorbar':   True,
            'cmap_name':       'RdBu_r',
            # Contours
            'show_contours':    False,
            'n_contour_levels': 10,
            # Line weight
            'line_weight':    1.5,
            # Component visibility
            'surface_items':  surface_items,
            # Quad / cutting planes
            'quad_available': adb.QuadData is not None,
            'quad_planes_enabled': (
                [False] * adb.QuadData.NumberOfPlanes
                if adb.QuadData is not None else []
            ),
            'quad_scalar':   'Cp',
            'quad_shading':  'smooth',
            'quad_opacity':  1.0,
            'show_quad_mesh':        False,
            'show_quad_contours':    False,
            'n_quad_contour_levels': 10,
            'show_quad_vectors':     False,
            'quad_vector_scale':     1.0,
            'quad_plane_labels': (
                [
                    f'{adb.QuadData.PlaneList[i].axis_name}='
                    f'{adb.QuadData.PlaneList[i].value:.2f}'
                    for i in range(1, adb.QuadData.NumberOfPlanes + 1)
                ]
                if adb.QuadData is not None else []
            ),
        })

    # ------------------------------------------------------------------
    # Menu bar
    # ------------------------------------------------------------------

    def _build_menus(self) -> None:
        mb = self.menuBar()
        mb.setNativeMenuBar(False)   # keep menu bar inside the window on macOS

        # -- File ------------------------------------------------------
        file_menu = mb.addMenu('&File')

        act = QAction('Open ADB...', self)
        act.setShortcut(QKeySequence('Ctrl+O'))
        act.triggered.connect(self._open_adb_dialog)
        file_menu.addAction(act)

        file_menu.addSeparator()

        act = QAction('Save PNG...', self)
        act.setShortcut(QKeySequence('Ctrl+S'))
        act.triggered.connect(self._save_png_dialog)
        file_menu.addAction(act)

        act = QAction('Save Animation Frames...', self)
        act.triggered.connect(self._save_frames_dialog)
        file_menu.addAction(act)

        file_menu.addSeparator()

        act = QAction('Quit', self)
        act.setShortcut(QKeySequence('Ctrl+Q'))
        act.triggered.connect(self.close)
        file_menu.addAction(act)

        # -- View ------------------------------------------------------
        view_menu = mb.addMenu('&View')

        for label, name, key in _VIEW_ITEMS:
            act = QAction(label, self)
            act.setShortcut(QKeySequence(key))
            act.triggered.connect(
                lambda _=False, n=name: self._on_view_preset(n)
            )
            view_menu.addAction(act)

        view_menu.addSeparator()

        self._act_bg_dark = QAction('Dark Background', self)
        self._act_bg_dark.setCheckable(True)
        self._act_bg_dark.toggled.connect(
            lambda checked: self._state_update({'bg_dark': checked})
        )
        view_menu.addAction(self._act_bg_dark)

        self._act_annotations = QAction('Show Annotations', self)
        self._act_annotations.setCheckable(True)
        self._act_annotations.toggled.connect(
            lambda checked: self._state_update({'show_annotations': checked})
        )
        view_menu.addAction(self._act_annotations)

        # -- Aero ------------------------------------------------------
        aero_menu = mb.addMenu('&Aero')

        scalar_group = QActionGroup(self)
        scalar_group.setExclusive(True)
        for label, value in _SCALAR_ITEMS:
            act = QAction(label, self)
            act.setCheckable(True)
            act.setData(value)
            scalar_group.addAction(act)
            aero_menu.addAction(act)
        self._scalar_action_group = scalar_group
        scalar_group.triggered.connect(
            lambda act: self._state_update({'scalar_mode': act.data()})
        )

        aero_menu.addSeparator()

        self._act_triangulation = QAction('Triangulation', self)
        self._act_triangulation.setCheckable(True)
        self._act_triangulation.toggled.connect(
            lambda checked: self._state_update({'show_triangulation': checked})
        )
        aero_menu.addAction(self._act_triangulation)

        self._act_mesh_edges = QAction('Computational Mesh', self)
        self._act_mesh_edges.setCheckable(True)
        self._act_mesh_edges.toggled.connect(
            lambda checked: self._state_update({'show_mesh_edges': checked})
        )
        aero_menu.addAction(self._act_mesh_edges)

        self._act_show_wake = QAction('Trailing Wakes', self)
        self._act_show_wake.setCheckable(True)
        self._act_show_wake.toggled.connect(
            lambda checked: self._state_update({'show_wake': checked})
        )
        aero_menu.addAction(self._act_show_wake)

        self._act_show_propulsion = QAction('Propulsion Elements', self)
        self._act_show_propulsion.setCheckable(True)
        self._act_show_propulsion.toggled.connect(
            lambda checked: self._state_update({'show_propulsion': checked})
        )
        aero_menu.addAction(self._act_show_propulsion)

        self._act_reflect = QAction('Reflect Geometry', self)
        self._act_reflect.setCheckable(True)
        self._act_reflect.toggled.connect(
            lambda checked: self._state_update({'reflect': checked})
        )
        aero_menu.addAction(self._act_reflect)

        # -- Options ---------------------------------------------------
        options_menu = mb.addMenu('&Options')

        shading_group = QActionGroup(self)
        shading_group.setExclusive(True)
        for label, value in [('Smooth Shading', 'smooth'), ('Flat Shading', 'flat')]:
            act = QAction(label, self)
            act.setCheckable(True)
            act.setData(value)
            shading_group.addAction(act)
            options_menu.addAction(act)
        self._shading_action_group = shading_group
        shading_group.triggered.connect(
            lambda act: self._state_update({'shading_style': act.data()})
        )

        options_menu.addSeparator()

        surface_group = QActionGroup(self)
        surface_group.setExclusive(True)
        for label, value in [
            ('Solid',       'solid'),
            ('Transparent', 'transparent'),
            ('Wireframe',   'wireframe'),
        ]:
            act = QAction(label, self)
            act.setCheckable(True)
            act.setData(value)
            surface_group.addAction(act)
            options_menu.addAction(act)
        self._surface_style_action_group = surface_group
        surface_group.triggered.connect(
            lambda act: self._state_update({'surface_style': act.data()})
        )

        options_menu.addSeparator()

        # Mesh Level submenu -- populated dynamically in _build_mesh_level_menu().
        self._mesh_level_menu          = options_menu.addMenu('Mesh Level')
        self._mesh_level_action_group  = QActionGroup(self)
        self._mesh_level_action_group.setExclusive(True)
        self._mesh_level_action_group.triggered.connect(
            lambda act: self._state_update({'mesh_level': act.data()})
        )

        options_menu.addSeparator()

        wake_style_menu  = options_menu.addMenu('Wake Style')
        wake_style_group = QActionGroup(self)
        wake_style_group.setExclusive(True)
        for label, value in [
            ('Lines',   'lines'),
            ('Tubes',   'tubes'),
            ('Points',  'points'),
            ('Surface', 'surface'),
        ]:
            act = QAction(label, self)
            act.setCheckable(True)
            act.setData(value)
            wake_style_group.addAction(act)
            wake_style_menu.addAction(act)
        self._wake_style_action_group = wake_style_group
        wake_style_group.triggered.connect(
            lambda act: self._state_update({'wake_mode': act.data()})
        )

        wake_color_menu  = options_menu.addMenu('Wake Color')
        wake_color_group = QActionGroup(self)
        wake_color_group.setExclusive(True)
        for label, value in [
            ('Uniform',  'uniform'),
            ('By Wing',  'by_wing'),
            ('By Span',  'by_span'),
        ]:
            act = QAction(label, self)
            act.setCheckable(True)
            act.setData(value)
            wake_color_group.addAction(act)
            wake_color_menu.addAction(act)
        self._wake_color_action_group = wake_color_group
        wake_color_group.triggered.connect(
            lambda act: self._state_update({'wake_color_mode': act.data()})
        )

        self._act_extend_wake = QAction('Extend Wake to Infinity', self)
        self._act_extend_wake.setCheckable(True)
        self._act_extend_wake.toggled.connect(
            lambda checked: self._state_update({'extend_wake_infinity': checked})
        )
        options_menu.addAction(self._act_extend_wake)

        options_menu.addSeparator()

        self._act_show_contours = QAction('Line Contours', self)
        self._act_show_contours.setCheckable(True)
        self._act_show_contours.toggled.connect(
            lambda checked: self._state_update({'show_contours': checked})
        )
        options_menu.addAction(self._act_show_contours)

        options_menu.addSeparator()

        line_weight_menu  = options_menu.addMenu('Line Weight')
        line_weight_group = QActionGroup(self)
        line_weight_group.setExclusive(True)
        for v in [0.5, 1.0, 1.5, 2.0, 3.0]:
            act = QAction(str(v), self)
            act.setCheckable(True)
            act.setData(v)
            line_weight_group.addAction(act)
            line_weight_menu.addAction(act)
        self._line_weight_action_group = line_weight_group
        line_weight_group.triggered.connect(
            lambda act: self._state_update({'line_weight': act.data()})
        )

        # -- Legend ----------------------------------------------------
        legend_menu = mb.addMenu('&Legend')

        self._act_show_colorbar = QAction('Colorbar', self)
        self._act_show_colorbar.setCheckable(True)
        self._act_show_colorbar.toggled.connect(
            lambda checked: self._state_update({'show_colorbar': checked})
        )
        legend_menu.addAction(self._act_show_colorbar)

        legend_menu.addSeparator()

        self._act_clim_auto = QAction('Auto Color Range', self)
        self._act_clim_auto.setCheckable(True)
        self._act_clim_auto.toggled.connect(self._on_clim_auto_toggled)
        legend_menu.addAction(self._act_clim_auto)

        act = QAction('Contour Settings...', self)
        act.triggered.connect(self._contour_settings_dialog)
        legend_menu.addAction(act)

        legend_menu.addSeparator()

        cmap_menu  = legend_menu.addMenu('Colormap')
        cmap_group = QActionGroup(self)
        cmap_group.setExclusive(True)
        for label, value in _CMAP_ITEMS:
            act = QAction(label, self)
            act.setCheckable(True)
            act.setData(value)
            cmap_group.addAction(act)
            cmap_menu.addAction(act)
        self._cmap_action_group = cmap_group
        cmap_group.triggered.connect(
            lambda act: self._state_update({'cmap_name': act.data()})
        )

        legend_menu.addSeparator()

        self._act_show_cg = QAction('CG Marker', self)
        self._act_show_cg.setCheckable(True)
        self._act_show_cg.toggled.connect(
            lambda checked: self._state_update({'show_cg': checked})
        )
        legend_menu.addAction(self._act_show_cg)

        self._act_show_axes = QAction('Axis Triad', self)
        self._act_show_axes.setCheckable(True)
        self._act_show_axes.toggled.connect(
            lambda checked: self._state_update({'show_axes': checked})
        )
        legend_menu.addAction(self._act_show_axes)

    def _build_mesh_level_menu(self) -> None:
        """Populate the Mesh Level submenu from n_mesh_levels in state."""
        self._mesh_level_menu.clear()
        for act in list(self._mesh_level_action_group.actions()):
            self._mesh_level_action_group.removeAction(act)

        n_levels = self._state.get('n_mesh_levels', 1)
        current  = self._state.get('mesh_level', 1)
        for lvl in range(1, n_levels + 1):
            act = QAction(str(lvl), self)
            act.setCheckable(True)
            act.setData(lvl)
            act.setChecked(lvl == current)
            self._mesh_level_action_group.addAction(act)
            self._mesh_level_menu.addAction(act)

    def _sync_menus_to_state(self) -> None:
        """Set all menu check states to match self._state."""
        s = self._state

        self._act_bg_dark.setChecked(s.get('bg_dark', False))
        self._act_annotations.setChecked(s.get('show_annotations', True))

        scalar = s.get('scalar_mode', 'Cp')
        for act in self._scalar_action_group.actions():
            act.setChecked(act.data() == scalar)

        self._act_triangulation.setChecked(s.get('show_triangulation', False))
        self._act_mesh_edges.setChecked(s.get('show_mesh_edges', True))
        self._act_show_wake.setChecked(s.get('show_wake', True))
        self._act_show_propulsion.setChecked(s.get('show_propulsion', True))
        self._act_reflect.setChecked(s.get('reflect', False))

        shading = s.get('shading_style', 'smooth')
        for act in self._shading_action_group.actions():
            act.setChecked(act.data() == shading)

        surface = s.get('surface_style', 'solid')
        for act in self._surface_style_action_group.actions():
            act.setChecked(act.data() == surface)

        self._build_mesh_level_menu()

        wake_mode = s.get('wake_mode', 'tubes')
        for act in self._wake_style_action_group.actions():
            act.setChecked(act.data() == wake_mode)

        wake_color = s.get('wake_color_mode', 'uniform')
        for act in self._wake_color_action_group.actions():
            act.setChecked(act.data() == wake_color)

        self._act_extend_wake.setChecked(s.get('extend_wake_infinity', False))
        self._act_show_contours.setChecked(s.get('show_contours', False))

        lw = s.get('line_weight', 1.5)
        for act in self._line_weight_action_group.actions():
            act.setChecked(act.data() == lw)

        self._act_show_colorbar.setChecked(s.get('show_colorbar', True))
        self._act_clim_auto.setChecked(s.get('clim_auto', True))

        cmap = s.get('cmap_name', 'RdBu_r')
        for act in self._cmap_action_group.actions():
            act.setChecked(act.data() == cmap)

        self._act_show_cg.setChecked(s.get('show_cg', False))
        self._act_show_axes.setChecked(s.get('show_axes', False))

    # ------------------------------------------------------------------
    # Dock (right panel)
    # ------------------------------------------------------------------

    def _build_dock(self) -> None:
        self._dock = QDockWidget('', self)
        self._dock.setAllowedAreas(Qt.DockWidgetArea.RightDockWidgetArea)
        self._dock.setFeatures(QDockWidget.DockWidgetFeature.NoDockWidgetFeatures)

        self._tab_widget    = QTabWidget()
        self._solution_tab  = SolutionTab(self)
        self._cutplanes_tab = CutPlanesTab(self)

        self._tab_widget.addTab(self._solution_tab, 'Solution')
        if self._adb.QuadData is not None:
            self._tab_widget.addTab(self._cutplanes_tab, 'Cut Planes')

        self._dock.setWidget(self._tab_widget)
        self._dock.setMinimumWidth(260)
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, self._dock)

    # ------------------------------------------------------------------
    # Slots
    # ------------------------------------------------------------------

    def _state_update(self, updates: dict) -> None:
        """Apply *updates* to state, refresh auto range, rebuild, and render."""
        self._state.update(updates)
        self._refresh_auto_range()
        self._scene.rebuild(self._state)
        self._render()

    def _refresh_auto_range(self) -> None:
        """Recompute scalar data range; update clim if auto-range is on."""
        scalar_mode = self._state.get('scalar_mode', 'Cp')
        if scalar_mode in ('Off', 'Shaded'):
            return
        rng = self._scene.scalar_auto_range(scalar_mode)
        if rng is None:
            return
        self._state['scalar_data_min'] = rng[0]
        self._state['scalar_data_max'] = rng[1]
        self.statusBar().showMessage(f'Data range: {rng[0]:.3f} to {rng[1]:.3f}')
        if self._state.get('clim_auto', True):
            self._state['clim_min'] = rng[0]
            self._state['clim_max'] = rng[1]

    def _on_panel_state_changed(self) -> None:
        """Triggered when the Solution or Cut Planes panel changes."""
        self._state.update(self._solution_tab.get_state())
        if self._adb.QuadData is not None:
            self._state.update(self._cutplanes_tab.get_state())
        self._refresh_auto_range()
        self._scene.rebuild(self._state)
        self._render()

    def _on_case_step(self, delta: int) -> None:
        n       = self._adb.NumberOfCases
        new_idx = max(1, min(n, self._state['case_index'] + delta))
        if new_idx == self._state['case_index']:
            return
        self._scene.load_case(new_idx)
        soln = self._scene.soln
        self._state['case_index'] = new_idx
        self._state['case_mach']  = float(soln.Mach)
        self._state['case_alpha'] = float(soln.Alpha)
        self._state['case_beta']  = float(soln.Beta)
        self._solution_tab.update_case_display(
            new_idx, n, soln.Mach, soln.Alpha, soln.Beta,
        )
        self._on_panel_state_changed()

    def _on_view_preset(self, name: str) -> None:
        self._scene.set_view(name)
        self._render()

    def _on_clim_auto_toggled(self, checked: bool) -> None:
        self._state['clim_auto'] = checked
        if checked:
            rng = self._scene.scalar_auto_range(self._state.get('scalar_mode', 'Cp'))
            if rng is not None:
                self._state['clim_min'] = rng[0]
                self._state['clim_max'] = rng[1]
                self.statusBar().showMessage(
                    f'Data range: {rng[0]:.3f} to {rng[1]:.3f}'
                )
        self._scene.rebuild(self._state)
        self._render()

    def _open_adb_dialog(self) -> None:
        adb_dir = os.path.dirname(os.path.abspath(self._adb_path))
        path, _ = QFileDialog.getOpenFileName(
            self, 'Open ADB File', adb_dir, 'VSPAERO ADB files (*.adb);;All files (*)',
        )
        if path:
            self._load_adb(path)

    def _load_adb(self, path: str) -> None:
        try:
            new_adb = ADBFile(path)
        except Exception as exc:
            QMessageBox.critical(self, 'Error Opening File', str(exc))
            return

        self._adb      = new_adb
        self._adb_path = path
        self.setWindowTitle(f'VSPAERO Viewer -- {os.path.basename(path)}')

        # Disconnect cut planes signal if currently connected.
        try:
            self._cutplanes_tab.state_changed.disconnect(self._on_panel_state_changed)
        except RuntimeError:
            pass

        self._scene = _SceneManager(self._adb, 1, plotter=self._interactor)
        self._state = {}
        self._init_state(1)
        self._sync_menus_to_state()

        # Update Cut Planes tab visibility.
        cp_idx = self._tab_widget.indexOf(self._cutplanes_tab)
        if cp_idx >= 0:
            self._tab_widget.removeTab(cp_idx)
        if self._adb.QuadData is not None:
            self._tab_widget.addTab(self._cutplanes_tab, 'Cut Planes')
            self._cutplanes_tab.state_changed.connect(self._on_panel_state_changed)
            self._cutplanes_tab.set_state(self._state)

        self._solution_tab.set_state(self._state)
        self._scene.rebuild(self._state)
        self._scene.set_view('iso')
        self._render()

    def _save_png_dialog(self) -> None:
        default = self._default_png_path()
        path, _ = QFileDialog.getSaveFileName(
            self, 'Save PNG', default, 'PNG files (*.png)',
        )
        if path:
            self._scene.plotter.screenshot(path)
            print(f'Saved {path}')

    def _save_frames_dialog(self) -> None:
        stem     = os.path.splitext(os.path.basename(self._adb_path))[0]
        adb_dir  = os.path.dirname(os.path.abspath(self._adb_path))
        proposed = os.path.join(adb_dir, f'{stem}_frames')
        out_dir, ok = QInputDialog.getText(
            self, 'Save Animation Frames', 'Output directory:', text=proposed,
        )
        if ok and out_dir.strip():
            self._save_frames(out_dir.strip())

    def _save_frames(self, out_dir: str) -> None:
        os.makedirs(out_dir, exist_ok=True)
        n         = self._adb.NumberOfCases
        orig_case = self._state.get('case_index', 1)

        for i in range(1, n + 1):
            self._scene.load_case(i)
            soln = self._scene.soln
            self._state['case_index'] = i
            self._state['case_mach']  = float(soln.Mach)
            self._state['case_alpha'] = float(soln.Alpha)
            self._state['case_beta']  = float(soln.Beta)
            self._refresh_auto_range()
            self._scene.rebuild(self._state)
            self._render()
            frame_path = os.path.join(out_dir, f'frame_{i:04d}.png')
            self._scene.plotter.screenshot(frame_path)
            self.statusBar().showMessage(f'Saved frame {i}/{n}: {frame_path}')
            QApplication.processEvents()

        # Restore original case.
        self._scene.load_case(orig_case)
        soln = self._scene.soln
        self._state['case_index'] = orig_case
        self._state['case_mach']  = float(soln.Mach)
        self._state['case_alpha'] = float(soln.Alpha)
        self._state['case_beta']  = float(soln.Beta)
        self._refresh_auto_range()
        self._scene.rebuild(self._state)
        self._solution_tab.update_case_display(orig_case, n, soln.Mach, soln.Alpha, soln.Beta)
        self._render()

        stem      = os.path.splitext(os.path.basename(self._adb_path))[0]
        movie_path = os.path.join(out_dir, f'{stem}_animation.mp4')
        ffmpeg_cmd = (
            f'ffmpeg -framerate 24 -i '
            f'"{os.path.join(out_dir, "frame_%04d.png")}" '
            f'-c:v libx264 -pix_fmt yuv420p "{movie_path}"'
        )
        QMessageBox.information(
            self, 'Frames Saved',
            f'Saved {n} frame(s) to:\n{out_dir}\n\n'
            f'To create a movie file, run:\n\n{ffmpeg_cmd}',
        )

    def _mesh_level_up(self) -> None:
        current = self._state.get('mesh_level', 1)
        max_lvl = self._state.get('n_mesh_levels', 1)
        new_lvl = min(current + 1, max_lvl)
        if new_lvl != current:
            self._state_update({'mesh_level': new_lvl})
            for act in self._mesh_level_action_group.actions():
                act.setChecked(act.data() == new_lvl)

    def _mesh_level_down(self) -> None:
        current = self._state.get('mesh_level', 1)
        new_lvl = max(current - 1, 1)
        if new_lvl != current:
            self._state_update({'mesh_level': new_lvl})
            for act in self._mesh_level_action_group.actions():
                act.setChecked(act.data() == new_lvl)

    def _contour_settings_dialog(self) -> None:
        """Open the Contour Settings dialog (colour range + N contour levels)."""
        dlg = QDialog(self)
        dlg.setWindowTitle('Contour Settings')

        layout = QFormLayout(dlg)
        layout.setFieldGrowthPolicy(
            QFormLayout.FieldGrowthPolicy.AllNonFixedFieldsGrow
        )

        max_spin = QDoubleSpinBox()
        max_spin.setRange(-1000.0, 1000.0)
        max_spin.setDecimals(3)
        max_spin.setSingleStep(0.1)
        max_spin.setValue(self._state.get('clim_max', 0.5))

        min_spin = QDoubleSpinBox()
        min_spin.setRange(-1000.0, 1000.0)
        min_spin.setDecimals(3)
        min_spin.setSingleStep(0.1)
        min_spin.setValue(self._state.get('clim_min', -1.5))

        auto_btn = QPushButton('Auto Determine Range')

        def _reset_range():
            min_spin.setValue(self._state.get('scalar_data_min', min_spin.value()))
            max_spin.setValue(self._state.get('scalar_data_max', max_spin.value()))

        auto_btn.clicked.connect(_reset_range)

        n_spin = QSpinBox()
        n_spin.setRange(2, 50)
        n_spin.setValue(self._state.get('n_contour_levels', 10))

        layout.addRow('Maximum:', max_spin)
        layout.addRow('Minimum:', min_spin)
        layout.addRow('', auto_btn)
        layout.addRow('Line Contour Levels:', n_spin)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok
            | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(dlg.accept)
        buttons.rejected.connect(dlg.reject)
        layout.addRow(buttons)

        if dlg.exec() == QDialog.DialogCode.Accepted:
            self._state['clim_min']         = min_spin.value()
            self._state['clim_max']         = max_spin.value()
            self._state['n_contour_levels'] = n_spin.value()
            self._state['clim_auto']        = False
            self._act_clim_auto.setChecked(False)
            self._scene.rebuild(self._state)
            self._render()

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _default_png_path(self) -> str:
        stem    = os.path.splitext(os.path.basename(self._adb_path))[0]
        out_dir = os.path.dirname(os.path.abspath(self._adb_path))
        idx     = self._state.get('case_index', 1)
        return os.path.join(out_dir, f'{stem}_case{idx}_view.png')

    # ------------------------------------------------------------------
    # Public interface
    # ------------------------------------------------------------------

    @property
    def scene(self) -> _SceneManager:
        """The _SceneManager that owns the pyvista plotter."""
        return self._scene

    def _render(self) -> None:
        """Force VTK to render and flush the Qt paint event immediately."""
        self._interactor.GetRenderWindow().Render()
        QApplication.processEvents()

    def run(self) -> None:
        """Show the main window (call QApplication.exec() after this)."""
        self.show()


# Alias so existing code that imports VSPAEROViewerApp still works.
VSPAEROViewerApp = VSPAERODesktopApp
