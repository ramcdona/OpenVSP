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
Top-level pytest fixtures shared across all pyvspaero test modules.
"""

import os

import pytest

from pyvspaero import ADBFile

_DATA_DIR = os.path.normpath(
    os.path.join(os.path.dirname(__file__), 'data')
)


@pytest.fixture(scope='session')
def adb_path():
    """Absolute path to WingBodyDisk.adb -- the standard multi-case test file."""
    return os.path.join(_DATA_DIR, 'WingBodyDisk.adb')


@pytest.fixture(scope='session')
def adb_file(adb_path):
    """Open ADBFile for WingBodyDisk.adb -- Panel model, 3 surfaces, propulsion, quad data."""
    return ADBFile(adb_path)


@pytest.fixture(scope='session')
def wing_adb_path():
    """Absolute path to Wing.adb -- simple VLM half-wing, no propulsion."""
    return os.path.join(_DATA_DIR, 'Wing.adb')


@pytest.fixture(scope='session')
def wing_adb_file(wing_adb_path):
    """Open ADBFile for Wing.adb -- VLM model, 2 surfaces, no propulsion."""
    return ADBFile(wing_adb_path)


@pytest.fixture(scope='session')
def wingbody_adb_path():
    """Absolute path to WingBody.adb -- Panel model, 3 surfaces, no propulsion."""
    return os.path.join(_DATA_DIR, 'WingBody.adb')


@pytest.fixture(scope='session')
def wingbody_adb_file(wingbody_adb_path):
    """Open ADBFile for WingBody.adb -- Panel model, 3 surfaces, no propulsion."""
    return ADBFile(wingbody_adb_path)

