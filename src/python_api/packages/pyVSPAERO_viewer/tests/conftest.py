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
Pytest fixtures for pyvspaero_viewer tests.
"""

import os

import pytest

from pyvspaero import ADBFile

_DATA_DIR = os.path.normpath(
    os.path.join(
        os.path.dirname(__file__),
        '..', '..', 'pyVSPAERO', 'tests', 'data',
    )
)


@pytest.fixture(scope='session')
def adb_path():
    """Absolute path to WingBodyDisk.adb -- the standard multi-case test file."""
    return os.path.join(_DATA_DIR, 'WingBodyDisk.adb')


@pytest.fixture(scope='session')
def adb_file(adb_path):
    """Open ADBFile for WingBodyDisk.adb, shared across the entire test session."""
    return ADBFile(adb_path)


@pytest.fixture(scope='session')
def qt_app():
    """Session-scoped QApplication for tests that exercise Qt widgets."""
    from PySide6.QtWidgets import QApplication
    import sys
    return QApplication.instance() or QApplication(sys.argv)
