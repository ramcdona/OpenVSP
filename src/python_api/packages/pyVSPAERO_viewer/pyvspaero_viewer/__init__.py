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
pyvspaero_viewer -- desktop viewer for VSPAERO *.adb files.

Launches a PySide6 desktop window with a pyvistaqt 3-D render panel and
a sidebar of interactive controls.

Quick start::

    python -m pyvspaero_viewer path/to/file.adb

Or after installing with the ``viewer`` extras::

    pip install 'pyvspaero[viewer]'
    vspaero-viewer path/to/file.adb

Public API
----------
VSPAEROViewerApp
    Main application class.  Instantiate, then call ``run()``.
"""

from pyvspaero_viewer.app import VSPAEROViewerApp

__all__ = ['VSPAEROViewerApp']
