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

"""Subprocess tests for pyvspaero_viewer.__main__ (CLI entry point).

These tests invoke the CLI via ``python -m pyvspaero_viewer`` so that the
argument parsing, file-not-found guard, case-index guard, and --output
off-screen rendering path are all exercised end-to-end.
"""

import subprocess
import sys

import pytest


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _run(*args, **kwargs):
    """Run ``python -m pyvspaero_viewer <args>`` and return the CompletedProcess."""
    return subprocess.run(
        [sys.executable, '-m', 'pyvspaero_viewer', *args],
        capture_output=True,
        text=True,
        **kwargs,
    )


# ---------------------------------------------------------------------------
# --output (off-screen PNG) mode
# ---------------------------------------------------------------------------

def test_output_flag_writes_png(adb_path, tmp_path):
    """--output writes a non-empty PNG file and exits 0."""
    out = tmp_path / 'render.png'
    result = _run(adb_path, '1', '--output', str(out))
    assert result.returncode == 0, result.stderr
    assert out.exists(), 'PNG file was not created'
    assert out.stat().st_size > 0, 'PNG file is empty'


def test_output_flag_case_2(adb_path, tmp_path):
    """--output with case_index=2 exits 0 and writes a PNG."""
    out = tmp_path / 'case2.png'
    result = _run(adb_path, '2', '--output', str(out))
    assert result.returncode == 0, result.stderr
    assert out.exists()


def test_output_flag_case_3(adb_path, tmp_path):
    """--output with the last case exits 0 and writes a PNG."""
    out = tmp_path / 'case3.png'
    result = _run(adb_path, '3', '--output', str(out))
    assert result.returncode == 0, result.stderr
    assert out.exists()


# ---------------------------------------------------------------------------
# Missing file guard
# ---------------------------------------------------------------------------

def test_missing_file_exits_nonzero(tmp_path):
    """A non-existent *.adb path causes the CLI to exit with a non-zero code."""
    result = _run(str(tmp_path / 'does_not_exist.adb'))
    assert result.returncode != 0


def test_missing_file_prints_error_message(tmp_path):
    """A non-existent *.adb path prints an error message to stderr."""
    result = _run(str(tmp_path / 'does_not_exist.adb'))
    assert 'Error' in result.stderr or 'error' in result.stderr


# ---------------------------------------------------------------------------
# Invalid case index guard
# ---------------------------------------------------------------------------

def test_case_index_zero_exits_nonzero(adb_path, tmp_path):
    """case_index=0 (below 1-based range) causes the CLI to exit non-zero."""
    out = tmp_path / 'out.png'
    result = _run(adb_path, '0', '--output', str(out))
    assert result.returncode != 0


def test_case_index_too_high_exits_nonzero(adb_path, tmp_path):
    """case_index beyond the number of cases causes the CLI to exit non-zero."""
    out = tmp_path / 'out.png'
    result = _run(adb_path, '99', '--output', str(out))
    assert result.returncode != 0


def test_case_index_too_high_prints_error(adb_path, tmp_path):
    """An out-of-range case_index prints an error message to stderr."""
    out = tmp_path / 'out.png'
    result = _run(adb_path, '99', '--output', str(out))
    assert 'Error' in result.stderr or 'error' in result.stderr
