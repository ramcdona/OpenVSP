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
pyvspaero_viewer.scene -- _SceneManager (pyvista scene rebuild logic).

``_SceneManager`` owns the pyvista plotter and contains the logic that
translates the viewer's state variables into pyvista actors.  When any
state variable changes, the application calls ``rebuild()`` to clear and
repopulate the scene.

Camera state is snapshotted before ``clear()`` and restored afterward so
that user-initiated rotations and zooms are preserved across rebuilds.
"""

import numpy as np
import pyvista as pv
import vtk

from pyvspaero.adb_plot import (
    _build_surface_mesh,
    _build_mesh_edges,
    _build_wake,
    _build_wake_surface,
    _build_quad_mesh,
    _add_propulsion_elements_to_plotter,
    _reflect_polydata,
    CMAP_VIEWER,
)

_DEFAULT_WINDOW_SIZE = (1600, 900)


class _SceneManager:
    """Manages the pyvista scene for the VSPAERO viewer.

    All scene geometry is rebuilt from scratch each time ``rebuild()``
    is called; the camera is preserved across rebuilds unless
    ``set_view()`` is called explicitly.

    Parameters
    ----------
    adb : ADBFile
        Open ADB file to read geometry and solutions from.
    case_index : int
        1-based case to load initially.
    plotter : pv.Plotter, optional
        Plotter to use.  When ``None`` (default) an off-screen
        ``pv.Plotter`` is created, which is suitable for headless PNG
        export.  Pass a ``pyvistaqt.QtInteractor`` for the desktop
        viewer.

    Attributes
    ----------
    plotter : pv.Plotter
        The pyvista plotter (off-screen or a ``QtInteractor``).
    geom : ADBGeometry
        Geometry for the currently loaded case.
    soln : ADBSolution
        Solution for the currently loaded case.
    """

    def __init__(self, adb, case_index=1, *, plotter=None):
        self._adb         = adb
        self._case_index  = case_index
        self._view_name   = 'iso'
        self._camera_set  = False

        if plotter is None:
            self.plotter = pv.Plotter(
                off_screen=True,
                window_size=list(_DEFAULT_WINDOW_SIZE),
            )
        else:
            self.plotter = plotter
        self.plotter.background_color = 'white'

        self.geom, self.soln = adb.LoadCase(case_index)
        self._global_clim    = adb.global_cp_auto_range()
        self._surface_mesh, self._surface_id = _build_surface_mesh(self.geom)
        self._surface_cell_ids = self._compute_surface_cell_ids()
        self._cam_bounds     = self._compute_cam_bounds()
        self._quad_planes    = self._load_quad_planes(case_index)
        self._filament_surface_ids = self._compute_filament_surface_ids()

    # ------------------------------------------------------------------
    # Public methods
    # ------------------------------------------------------------------

    def load_case(self, case_index):
        """Load a different case from the ADB file.

        Parameters
        ----------
        case_index : int
            1-based case index.  Must be in ``[1, adb.NumberOfCases]``.

        Raises
        ------
        ValueError
            If ``case_index`` is out of range.
        """
        n = self._adb.NumberOfCases
        if case_index < 1 or case_index > n:
            raise ValueError(
                f'case_index {case_index} is out of range -- '
                f'file has {n} case(s).'
            )
        self._case_index = case_index
        self.geom, self.soln = self._adb.LoadCase(case_index)
        self._surface_mesh, self._surface_id = _build_surface_mesh(self.geom)
        self._surface_cell_ids = self._compute_surface_cell_ids()
        self._cam_bounds = self._compute_cam_bounds()
        self._quad_planes = self._load_quad_planes(case_index)
        self._filament_surface_ids = self._compute_filament_surface_ids()

    def scalar_auto_range(self, scalar_mode):
        """Return (min, max) for the given scalar field in the current case.

        Parameters
        ----------
        scalar_mode : str
            One of ``'Cp'``, ``'CpSteady'``, ``'CpUnsteady'``, ``'Gamma'``.
            Returns ``None`` for ``'Off'`` or ``'Shaded'``.
        """
        scalar_map = {
            'Cp':         self.soln.Cp,
            'CpSteady':   self.soln.CpSteady,
            'CpUnsteady': self.soln.CpUnsteady,
            'Gamma':      self.soln.Gamma,
        }
        arr = scalar_map.get(scalar_mode)
        if arr is None:
            return None
        vals = np.asarray(arr[self._surface_id], dtype=np.float64)
        return float(vals.min()), float(vals.max())

    def rebuild(self, state=None):
        """Clear all actors and rebuild the scene from the current state.

        Parameters
        ----------
        state : dict, optional
            Viewer state variables.  If ``None``, a default state is
            used (Cp surface, mesh edges on, wake tubes on, etc.).
            Any key absent from ``state`` falls back to the default.

        Notes
        -----
        Camera position is preserved across rebuilds.  Call
        ``set_view()`` before ``rebuild()`` to apply a preset.
        """
        s = _StateProxy(state)

        # Background colour (must be set before clear() to persist).
        self.plotter.background_color = '#0000e6' if s.bg_dark else 'white'

        # Snapshot the camera so we can restore it after clear().
        # parallel_scale controls zoom in parallel-projection mode and is NOT
        # included in camera_position, so it must be saved and restored separately.
        cam_pos        = self.plotter.camera_position           if self._camera_set else None
        parallel_scale = self.plotter.camera.GetParallelScale() if self._camera_set else None

        self.plotter.clear()

        self._add_surface(s)
        if s.show_mesh_edges:
            self._add_mesh_edges(s)
        if s.show_triangulation:
            self._add_triangulation(s)
        if s.show_wake:
            self._add_wake(s)
        if s.show_propulsion:
            self._add_propulsion(s)
        if s.show_cg:
            self._add_cg_marker(s)
        if s.show_axes:
            self._add_axes_triad()
        if s.show_annotations:
            self._add_annotations(s)
        if s.quad_available and self._quad_planes is not None:
            self._add_quad_slices(s)
        if s.show_colorbar and s.scalar_mode not in ('Off', 'Shaded'):
            self._add_colorbar(s)

        # Restore or initialise the camera.
        if cam_pos is not None:
            self.plotter.camera_position = cam_pos
        else:
            self.set_view(self._view_name)

        # enable_parallel_projection() recalculates parallel_scale from the
        # camera distance, so it must run before we restore the saved scale.
        self.plotter.enable_parallel_projection()

        # Restore zoom last -- parallel_scale is the zoom parameter in parallel
        # projection and is not part of camera_position.
        if parallel_scale is not None:
            self.plotter.camera.SetParallelScale(parallel_scale)

    def set_view(self, name):
        """Apply a named camera preset.

        Parameters
        ----------
        name : str
            One of ``'iso'``, ``'top'``, ``'bottom'``, ``'front'``,
            ``'rear'``, ``'left'``, ``'right'``.
        """
        self._view_name = name
        if name == 'iso':
            self.plotter.view_isometric()
        else:
            # OpenVSP convention: +x nose->tail, +y right wing, +z up.
            # pyvista view_vector(v) places the camera at focal + v*distance,
            # so v points FROM the scene TOWARD the camera.
            # 'front'  -- camera in front of nose  (-x from scene)
            # 'rear'   -- camera behind tail       (+x from scene)
            # 'left'   -- camera on port side      (-y from scene)
            # 'right'  -- camera on starboard side (+y from scene)
            vec, up = {
                'top':    (( 0,  0,  1), ( 1,  0,  0)),
                'bottom': (( 0,  0, -1), ( 1,  0,  0)),
                'front':  ((-1,  0,  0), ( 0,  0,  1)),
                'rear':   (( 1,  0,  0), ( 0,  0,  1)),
                'left':   (( 0, -1,  0), ( 0,  0,  1)),
                'right':  (( 0,  1,  0), ( 0,  0,  1)),
            }[name]
            self.plotter.view_vector(vec, viewup=up)

        self.plotter.reset_camera(bounds=self._cam_bounds)
        self.plotter.camera_set = True
        self.plotter.enable_parallel_projection()
        self._camera_set = True

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _load_quad_planes(self, case_index):
        """Load quad cutting-plane data for *case_index*, or return None."""
        qd = self._adb.QuadData
        if qd is None:
            return None
        try:
            return qd.LoadCase(case_index)
        except (IndexError, FileNotFoundError):
            return None

    def _compute_filament_surface_ids(self):
        """Return a list (1-indexed, [0]=0) mapping each wake filament to its surface ID.

        Uses the coarse mesh level-1 edge SurfaceID to build a node->surface
        lookup, then resolves each filament's trailing-edge node (te_node).
        Filaments whose te_node is not found in the map get surface ID 0.
        """
        n = self.soln.NumberOfTrailingVortexEdges
        result = [0] * (n + 1)
        if n == 0 or self.geom.NumberOfMeshLevels < 1:
            return result
        level = self.geom.CoarseMeshLevels[1]
        if level is None:
            return result
        n_ce     = level.NumberOfEdges
        sids     = level.EdgeList_SurfaceID[1:n_ce + 1]
        node1s   = level.EdgeList_node1[1:n_ce + 1]
        node2s   = level.EdgeList_node2[1:n_ce + 1]
        mask     = sids > 0
        node_sid = {}
        for sid, n1, n2 in zip(sids[mask].tolist(),
                                node1s[mask].tolist(),
                                node2s[mask].tolist()):
            node_sid[n1] = sid
            node_sid[n2] = sid
        for i in range(1, n + 1):
            wf = self.soln.WakeFilaments[i]
            result[i] = node_sid.get(int(wf.te_node), 0)
        return result

    def _compute_surface_cell_ids(self):
        """Return an int array of surface IDs, one per cell in the surface mesh."""
        return np.asarray(
            self.geom.TriList_surface_id[self._surface_id], dtype=np.int32
        )

    def _compute_cam_bounds(self):
        """Return padded bounding box used for reset_camera() calls.

        Expands the surface mesh bounding box by 15% of the maximum span so
        the geometry is not clipped to the edge of the viewport after a reset.
        """
        b   = list(self._surface_mesh.bounds)
        span = max(b[1]-b[0], b[3]-b[2], max(b[5]-b[4], 1e-3))
        pad  = span * 0.15
        return [b[0]-pad, b[1]+pad, b[2]-pad, b[3]+pad, b[4]-pad, b[5]+pad]

    def _add_surface(self, s):
        if s.scalar_mode == 'Off':
            return

        # Apply per-component visibility filter.
        surface_items = s.surface_items
        if surface_items:
            visible_ids = {item['id'] for item in surface_items if item['visible']}
            mask = np.isin(self._surface_cell_ids, list(visible_ids))
            cell_indices = np.where(mask)[0]
            if len(cell_indices) == 0:
                return
            mesh = self._surface_mesh.extract_cells(cell_indices)
            # Remap surface_id for scalar indexing.
            t_idx_filtered = self._surface_id[cell_indices]
        else:
            mesh = self._surface_mesh.copy()
            t_idx_filtered = self._surface_id

        mesh = mesh.copy()

        if s.clim_auto and s.scalar_mode not in ('Off', 'Shaded'):
            auto = self.scalar_auto_range(s.scalar_mode)
            clim = auto if auto is not None else (float(s.clim_min), float(s.clim_max))
        else:
            clim = (float(s.clim_min), float(s.clim_max))

        surface_style = s.surface_style or 'solid'
        opacity = 0.25 if surface_style == 'transparent' else 1.0

        if surface_style == 'wireframe':
            # Edge-only rendering: draw triangle edges regardless of scalar mode.
            edges = mesh.extract_all_edges()
            self.plotter.add_mesh(edges, color='dimgray', line_width=s.line_weight)
            if s.reflect:
                self.plotter.add_mesh(_reflect_polydata(edges), color='dimgray', line_width=s.line_weight)
            return

        if s.scalar_mode == 'Shaded':
            self.plotter.add_mesh(
                mesh, color='lightgray', show_edges=False,
                show_scalar_bar=False, opacity=opacity,
            )
            if s.reflect:
                self.plotter.add_mesh(
                    _reflect_polydata(mesh), color='lightgray',
                    show_edges=False, show_scalar_bar=False, opacity=opacity,
                )
            return

        scalar_map = {
            'Cp':          self.soln.Cp,
            'CpSteady':    self.soln.CpSteady,
            'CpUnsteady':  self.soln.CpUnsteady,
            'Gamma':       self.soln.Gamma,
        }
        arr = scalar_map.get(s.scalar_mode)
        if arr is None:
            return

        mesh.cell_data['scalar'] = np.array(arr[t_idx_filtered], dtype=np.float64)

        smooth = (s.shading_style == 'smooth')
        cmap   = s.cmap_name or CMAP_VIEWER

        # For smooth interpolation the scalar must be in point data so VTK
        # can interpolate colour values across triangle boundaries.
        if smooth:
            mesh = mesh.cell_data_to_point_data()

        mesh_kw = dict(
            scalars                = 'scalar',
            cmap                   = cmap,
            clim                   = clim,
            smooth_shading         = smooth,
            interpolate_before_map = smooth,
            opacity                = opacity,
            show_scalar_bar        = False,
            show_edges             = False,
        )
        self.plotter.add_mesh(mesh, **mesh_kw)
        if s.reflect:
            # _reflect_polydata copies the mesh (including all arrays).
            self.plotter.add_mesh(_reflect_polydata(mesh), **mesh_kw)

        # Iso-line contours overlaid on the scalar surface.
        if s.show_contours:
            self._add_contours(mesh, clim, s)

    def _add_mesh_edges(self, s):
        lvl = s.mesh_level
        if lvl < 1 or lvl > self.geom.NumberOfMeshLevels:
            return
        level = self.geom.CoarseMeshLevels[lvl]
        if level is None:
            return
        visible_ids = None
        surface_items = s.surface_items
        if surface_items:
            visible_ids = {item['id'] for item in surface_items if item['visible']}
        em = _build_mesh_edges(level, visible_ids=visible_ids)
        if em.n_points == 0:
            return
        self.plotter.add_mesh(em, color='black', line_width=s.line_weight)
        if s.reflect:
            self.plotter.add_mesh(_reflect_polydata(em), color='black', line_width=s.line_weight)

    def _add_triangulation(self, s):
        edges = self._surface_mesh.extract_all_edges()
        self.plotter.add_mesh(edges, color='black', line_width=s.line_weight)
        if s.reflect:
            self.plotter.add_mesh(_reflect_polydata(edges), color='black', line_width=s.line_weight)

    # Distinct colours for wake-by-wing mode (Tableau-10 palette).
    _WING_COLORS = [
        '#4e79a7', '#f28e2b', '#e15759', '#76b7b2', '#59a14f',
        '#edc948', '#b07aa1', '#ff9da7', '#9c755f', '#bab0ac',
    ]

    def _add_wake(self, s):
        extend = bool(s.extend_wake_infinity)

        # Build filament_ids filter from visible surface items.
        filament_ids = None
        surface_items = s.surface_items
        if surface_items:
            visible_ids = {item['id'] for item in surface_items if item['visible']}
            filament_ids = {
                i for i, sid in enumerate(self._filament_surface_ids[1:], 1)
                if sid in visible_ids
            }

        if s.wake_mode == 'surface':
            # Wake surface triangles don't carry per-surface IDs so component
            # filtering is not supported in this mode.
            wmesh = _build_wake_surface(self.geom, extend_to_infinity=extend)
            if wmesh is None:
                return
            self.plotter.add_mesh(wmesh, color='steelblue', opacity=0.4, show_edges=False)
            if s.reflect:
                self.plotter.add_mesh(
                    _reflect_polydata(wmesh), color='steelblue', opacity=0.4, show_edges=False,
                )
            return

        # Tube radius (used only for tube mode).
        b    = self._surface_mesh.bounds
        diag = ((b[1]-b[0])**2 + (b[3]-b[2])**2 + (b[5]-b[4])**2)**0.5
        r    = 0.002 * diag

        # Build groups: for uniform mode one group with all filaments;
        # for by_wing mode one group per surface ID;
        # for by_span mode a single mesh with Y-coord cell scalars.
        lw = float(s.line_weight)

        if s.wake_color_mode == 'by_span':
            wmesh = _build_wake(self.soln, extend_to_infinity=extend, filament_ids=filament_ids)
            if wmesh is not None:
                span_y = self._compute_wake_span_scalars(filament_ids, extend)
                if len(span_y) == wmesh.n_cells:
                    wmesh.cell_data['span_y'] = np.array(span_y, dtype=np.float32)
                    self._render_wake_mesh_scalar(wmesh, 'span_y', s.wake_mode, r, s.reflect, lw)
                else:
                    self._render_wake_mesh(wmesh, s.wake_mode, 'steelblue', r, s.reflect, lw)
        elif s.wake_color_mode == 'by_wing':
            groups = {}
            n = len(self._filament_surface_ids) - 1
            for i in range(1, n + 1):
                if filament_ids is not None and i not in filament_ids:
                    continue
                sid = self._filament_surface_ids[i]
                groups.setdefault(sid, set()).add(i)
            for idx, (_, ids) in enumerate(sorted(groups.items())):
                color = self._WING_COLORS[idx % len(self._WING_COLORS)]
                wmesh = _build_wake(self.soln, extend_to_infinity=extend, filament_ids=ids)
                if wmesh is not None:
                    self._render_wake_mesh(wmesh, s.wake_mode, color, r, s.reflect, lw)
        else:
            wmesh = _build_wake(self.soln, extend_to_infinity=extend, filament_ids=filament_ids)
            if wmesh is not None:
                self._render_wake_mesh(wmesh, s.wake_mode, 'steelblue', r, s.reflect, lw)

    def _compute_wake_span_scalars(self, filament_ids, extend):
        """Return a list of per-filament Y coordinates for span colouring.

        Iterates in the same order as ``_build_wake`` so the resulting list
        is aligned with the PolyData cells returned by that function.
        """
        soln = self.soln
        n    = soln.NumberOfTrailingVortexEdges
        y_vals = []
        for i in range(1, n + 1):
            if filament_ids is not None and i not in filament_ids:
                continue
            wf    = soln.WakeFilaments[i]
            n_use = wf.NumberOfNodes if extend else wf.NumberOfNodes - 1
            if n_use < 2:
                continue
            y_vals.append(float(wf.y[1]))
        return y_vals

    def _render_wake_mesh_scalar(self, wmesh, scalar_key, wake_mode, r, reflect, lw):
        """Render a wake PolyData coloured by a cell-data scalar array."""
        kw = dict(scalars=scalar_key, cmap='viridis', show_scalar_bar=False)
        if wake_mode == 'tubes':
            self.plotter.add_mesh(wmesh.tube(radius=r), **kw)
        elif wake_mode == 'points':
            pts = pv.PolyData(wmesh.points)
            self.plotter.add_mesh(pts, render_points_as_spheres=True, point_size=5, **kw)
        else:
            self.plotter.add_mesh(wmesh, line_width=lw, **kw)
        if reflect:
            rm = _reflect_polydata(wmesh)
            if wake_mode == 'tubes':
                self.plotter.add_mesh(rm.tube(radius=r), **kw)
            elif wake_mode == 'points':
                self.plotter.add_mesh(
                    pv.PolyData(rm.points),
                    render_points_as_spheres=True, point_size=5, **kw,
                )
            else:
                self.plotter.add_mesh(rm, line_width=lw, **kw)

    def _render_wake_mesh(self, wmesh, wake_mode, color, r, reflect, lw):
        """Add a single wake PolyData to the plotter in the requested style."""
        if wake_mode == 'tubes':
            self.plotter.add_mesh(wmesh.tube(radius=r), color=color)
        elif wake_mode == 'points':
            pts = pv.PolyData(wmesh.points)
            self.plotter.add_mesh(pts, color=color, render_points_as_spheres=True, point_size=5)
        else:  # 'lines'
            self.plotter.add_mesh(wmesh, color=color, line_width=lw)
        if reflect:
            rm = _reflect_polydata(wmesh)
            if wake_mode == 'tubes':
                self.plotter.add_mesh(rm.tube(radius=r), color=color)
            elif wake_mode == 'points':
                rpts = pv.PolyData(rm.points)
                self.plotter.add_mesh(rpts, color=color, render_points_as_spheres=True, point_size=5)
            else:
                self.plotter.add_mesh(rm, color=color, line_width=lw)

    def _add_quad_slices(self, s):
        planes  = self._quad_planes
        enabled = s.quad_planes_enabled
        scalar  = s.quad_scalar or 'Cp'
        opacity = s.quad_opacity if s.quad_opacity is not None else 1.0
        smooth  = (s.quad_shading == 'smooth')
        cmap    = s.cmap_name or CMAP_VIEWER

        # Collect enabled plane meshes first so we can compute a unified clim.
        enabled_meshes = []
        for i, plane in enumerate(planes):
            if plane is None:
                continue
            if enabled and i > 0 and i <= len(enabled) and not enabled[i - 1]:
                continue
            mesh = _build_quad_mesh(plane)
            scalar_key = scalar if scalar in mesh.point_data.keys() else 'Cp'
            enabled_meshes.append((mesh, scalar_key))

        if not enabled_meshes:
            return

        # Unified clim: for Cp share the viewer surface range; for Vmag
        # compute across all enabled planes.
        if scalar == 'Cp':
            if s.clim_auto:
                auto = self.scalar_auto_range('Cp')
                clim = list(auto) if auto is not None else [float(s.clim_min), float(s.clim_max)]
            else:
                clim = [float(s.clim_min), float(s.clim_max)]
        else:
            all_vals = np.concatenate([m.point_data[sk] for m, sk in enabled_meshes])
            lo, hi = float(all_vals.min()), float(all_vals.max())
            if lo == hi:
                delta = max(abs(lo) * 0.05, 0.1)
                lo, hi = lo - delta, hi + delta
            clim = [lo, hi]

        show_quad_mesh     = bool(s.show_quad_mesh)
        show_quad_contours = bool(s.show_quad_contours)
        n_quad_contours    = max(2, int(s.n_quad_contour_levels or 10))
        show_quad_vectors  = bool(s.show_quad_vectors)
        vec_scale          = float(s.quad_vector_scale or 1.0)

        # Pre-compute a shared normalisation for velocity glyphs across planes.
        if show_quad_vectors:
            all_vmag = np.concatenate([
                m.point_data['Vmag'] for m, _ in enabled_meshes
                if 'Vmag' in m.point_data.keys()
            ] or [np.array([1.0])])
            vmax = float(all_vmag.max()) or 1.0
            # Size arrows relative to the quad mesh itself (not the surface),
            # which can be much larger. Target: max arrow ~ avg node spacing.
            all_pts = np.concatenate([m.points for m, _ in enabled_meshes])
            pt_span = all_pts.max(axis=0) - all_pts.min(axis=0)
            quad_diag = float(np.linalg.norm(pt_span)) or 1.0
            avg_spacing = quad_diag / max(1.0, len(all_pts) ** 0.5)
            glyph_factor = vec_scale * avg_spacing / (3.0 * vmax)

        for mesh, scalar_key in enabled_meshes:
            m = mesh.copy()
            # Quad meshes carry scalars as point data.  For flat rendering
            # convert to cell data so VTK assigns one colour per cell.
            if not smooth:
                m = m.point_data_to_cell_data()
            mesh_kw = dict(
                scalars                = scalar_key,
                clim                   = clim,
                cmap                   = cmap,
                opacity                = opacity,
                smooth_shading         = smooth,
                interpolate_before_map = smooth,
                show_scalar_bar        = False,
                show_edges             = False,
            )
            self.plotter.add_mesh(m, **mesh_kw)
            if show_quad_mesh:
                self.plotter.add_mesh(
                    m.extract_all_edges(), color='black', line_width=s.line_weight,
                )
            if show_quad_contours:
                cm = mesh.copy()
                lo, hi = clim[0], clim[1]
                if lo < hi:
                    levels = np.linspace(lo, hi, n_quad_contours + 2)[1:-1].tolist()
                    contours = cm.contour(isosurfaces=levels, scalars=scalar_key)
                    if contours.n_points > 0:
                        self.plotter.add_mesh(contours, color='black', line_width=s.line_weight)
                        if s.reflect:
                            self.plotter.add_mesh(
                                _reflect_polydata(contours), color='black', line_width=s.line_weight,
                            )
            if show_quad_vectors and 'Velocity' in mesh.point_data.keys():
                gm = mesh.copy()
                glyphs = gm.glyph(orient='Velocity', scale='Vmag', factor=glyph_factor)
                self.plotter.add_mesh(glyphs, color='black')
                if s.reflect:
                    self.plotter.add_mesh(_reflect_polydata(glyphs), color='black')
            if s.reflect:
                rm = _reflect_polydata(m)
                self.plotter.add_mesh(rm, **mesh_kw)
                if show_quad_mesh:
                    self.plotter.add_mesh(
                        rm.extract_all_edges(), color='black', line_width=s.line_weight,
                    )

    def _add_contours(self, mesh, clim, s):
        """Overlay iso-line contours on the surface scalar field."""
        n = max(2, int(s.n_contour_levels or 10))
        lo, hi = clim
        if lo >= hi:
            return
        levels = np.linspace(lo, hi, n + 2)[1:-1].tolist()
        # mesh already has 'scalar' in point_data (smooth) or cell_data (flat);
        # pyvista contour() requires point data.
        if 'scalar' not in mesh.point_data.keys():
            c_mesh = mesh.cell_data_to_point_data()
        else:
            c_mesh = mesh
        contours = c_mesh.contour(isosurfaces=levels, scalars='scalar')
        if contours.n_points == 0:
            return
        self.plotter.add_mesh(contours, color='black', line_width=s.line_weight)
        if s.reflect:
            self.plotter.add_mesh(_reflect_polydata(contours), color='black', line_width=s.line_weight)

    def _add_cg_marker(self, s):
        """Render a small red sphere at the CG location."""
        header = self._adb.Header
        b    = self._surface_mesh.bounds
        diag = ((b[1]-b[0])**2 + (b[3]-b[2])**2 + (b[5]-b[4])**2)**0.5
        r    = 0.015 * diag
        cg   = pv.Sphere(radius=r, center=[header.Xcg, header.Ycg, header.Zcg])
        self.plotter.add_mesh(cg, color='red', show_scalar_bar=False)

    def _add_axes_triad(self):
        """Add an orientation axis triad at the model origin.

        Uses vtkAxesActor added directly to the renderer rather than
        vtkOrientationMarkerWidget (which adds a second overlay renderer
        that is harder to manage).
        """
        b    = self._surface_mesh.bounds
        diag = ((b[1]-b[0])**2 + (b[3]-b[2])**2 + (b[5]-b[4])**2)**0.5
        axes = vtk.vtkAxesActor()
        axes.SetTotalLength(diag * 0.15, diag * 0.15, diag * 0.15)
        axes.SetShaftTypeToCylinder()
        axes.SetAxisLabels(True)
        self.plotter.renderer.AddActor(axes)

    def _add_propulsion(self, s):
        _add_propulsion_elements_to_plotter(
            self.plotter, self.geom, reflect=s.reflect,
        )

    def _add_annotations(self, s):
        label = (
            f'M={self.soln.Mach:.4f}  '
            f'Alpha={self.soln.Alpha:.2f} deg  '
            f'Beta={self.soln.Beta:.2f} deg'
        )
        text_color = 'white' if s.bg_dark else 'black'
        self.plotter.add_text(
            label, position='upper_left', font_size=11, color=text_color,
        )

    def _add_colorbar(self, s):
        # The scalar bar is attached to the last surface actor's mapper.
        actors = [
            a for a in self.plotter.renderer.actors.values()
            if hasattr(a, 'mapper') and a.mapper is not None
               and a.mapper.scalar_range != (0.0, 1.0)
        ]
        if not actors:
            return
        text_color = 'white' if s.bg_dark else 'black'
        self.plotter.add_scalar_bar(
            title      = s.scalar_mode,
            mapper     = actors[0].mapper,
            vertical   = True,
            n_labels   = 5,
            fmt        = '%.2f',
            position_x = 0.87,
            position_y = 0.10,
            width      = 0.04,
            height     = 0.80,
            color      = text_color,
        )


# ---------------------------------------------------------------------------
# Helper: uniform attribute-style access to a state dict
# ---------------------------------------------------------------------------

class _StateProxy:
    """Provides attribute-style access to a state dict.

    Falls back to sensible viewer defaults for any key that is absent.
    """

    _DEFAULTS = {
        'scalar_mode':           'Cp',
        'shading_style':         'smooth',
        'surface_style':         'solid',
        'show_mesh_edges':       True,
        'mesh_level':            1,
        'show_triangulation':    False,
        'show_wake':             True,
        'wake_mode':             'tubes',
        'extend_wake_infinity':  False,
        'wake_color_mode':       'uniform',
        'show_propulsion':       True,
        'reflect':               False,
        'bg_dark':               False,
        'clim_auto':          True,
        'clim_min':           -1.5,
        'clim_max':            0.5,
        'scalar_data_min':    -1.5,
        'scalar_data_max':     0.5,
        'show_colorbar':      True,
        'show_annotations':   True,
        'show_contours':      False,
        'n_contour_levels':   10,
        'show_cg':            False,
        'show_axes':          False,
        'line_weight':        1.5,
        'surface_items':      [],   # empty = show all
        'quad_available':     False,
        'quad_planes_enabled': [],
        'quad_scalar':        'Cp',
        'quad_shading':       'smooth',
        'quad_opacity':       1.0,
        'show_quad_mesh':        False,
        'show_quad_contours':    False,
        'n_quad_contour_levels': 10,
        'show_quad_vectors':     False,
        'quad_vector_scale':     1.0,
        'cmap_name':          'RdBu_r',
    }

    def __init__(self, state):
        self._state = state

    def __getattr__(self, key):
        if key.startswith('_'):
            raise AttributeError(key)
        if self._state is None:
            return self._DEFAULTS.get(key)
        if isinstance(self._state, dict):
            return self._state.get(key, self._DEFAULTS.get(key))
        return self._DEFAULTS.get(key)
