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

"""Unit tests for viewer screenshot and view-preset behaviour."""

import os

import pytest

from pyvspaero_viewer.scene import _SceneManager


# ---------------------------------------------------------------------------
# Fixture: off-screen scene reused across tests
# ---------------------------------------------------------------------------

@pytest.fixture(scope='module')
def scene(adb_file):
    """_SceneManager for WingBodyDisk case 1, rebuilt with defaults."""
    sm = _SceneManager(adb_file, case_index=1)
    sm.rebuild()
    return sm


# ---------------------------------------------------------------------------
# Screenshot tests
# ---------------------------------------------------------------------------

def test_save_png_writes_file(scene, tmp_path):
    """screenshot() writes a non-empty file to the given path."""
    out_path = str(tmp_path / 'test_save.png')
    scene.plotter.screenshot(out_path)
    assert os.path.isfile(out_path)
    assert os.path.getsize(out_path) > 0


def test_save_png_produces_valid_png(scene, tmp_path):
    """The file written by screenshot() starts with the PNG magic bytes."""
    out_path = str(tmp_path / 'test_output.png')
    scene.plotter.screenshot(out_path)
    with open(out_path, 'rb') as f:
        header = f.read(8)
    assert header == b'\x89PNG\r\n\x1a\n', 'File is not a valid PNG'


# ---------------------------------------------------------------------------
# View-preset tests
# ---------------------------------------------------------------------------

def test_view_preset_iso_changes_camera(scene):
    """set_view('iso') updates the camera position."""
    scene.set_view('top')
    pos_top = tuple(scene.plotter.camera_position)
    scene.set_view('iso')
    pos_iso = tuple(scene.plotter.camera_position)
    assert pos_top != pos_iso


@pytest.mark.parametrize('name', ['iso', 'top', 'bottom', 'front', 'rear', 'left', 'right'])
def test_all_view_presets_execute(scene, name):
    """Each view preset can be applied without raising an exception."""
    scene.set_view(name)   # must not raise
