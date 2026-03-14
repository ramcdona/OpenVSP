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
Smoke tests for pyvspaero_viewer.

These tests verify that the package imports cleanly and the CLI entry
point responds to --help without error.  They are marked with
``@pytest.mark.smoke`` and should complete in a few seconds.
"""

import subprocess
import sys

import pytest


@pytest.mark.smoke
def test_viewer_package_imports():
    """pyvspaero_viewer is importable after installation."""
    import pyvspaero_viewer  # noqa: F401


@pytest.mark.smoke
def test_viewer_app_importable():
    """VSPAEROViewerApp is importable from the public API."""
    from pyvspaero_viewer import VSPAEROViewerApp  # noqa: F401


@pytest.mark.smoke
def test_viewer_help_exits_zero():
    """``python -m pyvspaero_viewer --help`` exits with code 0."""
    result = subprocess.run(
        [sys.executable, '-m', 'pyvspaero_viewer', '--help'],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, (
        f'--help exited {result.returncode}.\nstdout: {result.stdout}\nstderr: {result.stderr}'
    )


@pytest.mark.smoke
def test_viewer_help_mentions_adb_path():
    """``--help`` output mentions the adb_path positional argument."""
    result = subprocess.run(
        [sys.executable, '-m', 'pyvspaero_viewer', '--help'],
        capture_output=True,
        text=True,
    )
    assert 'adb_path' in result.stdout
