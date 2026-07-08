from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest

from opencap_to_nwb.convert import convert_session


def example_session_dir() -> Path:
    return Path(__file__).resolve().parents[1] / "test_data" / "session_001"


def test_inspect_cli_prints_nwb_summary(tmp_path):
    pytest.importorskip("pynwb")

    output = tmp_path / "session_001.nwb"
    convert_session(example_session_dir(), output)

    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "opencap_to_nwb.cli",
            "inspect",
            str(output),
        ],
        check=False,
        capture_output=True,
        text=True,
    )

    assert result.returncode == 0, result.stderr

    stdout = result.stdout

    assert "NWB file:" in stdout
    assert "identifier:" in stdout
    assert "session_id:" in stdout
    assert "subject_id:" in stdout
    assert "session_description:" in stdout
    assert "Processing modules:" in stdout
    assert "Behavior interfaces:" in stdout
    assert "OpenCapPose3D" in stdout
    assert "OpenCapJointAngles" in stdout
    assert "shape:" in stdout
    assert "timestamps:" in stdout
    assert "first_timestamp:" in stdout
    assert "final_timestamp:" in stdout


def test_inspect_cli_fails_for_missing_file(tmp_path):
    missing = tmp_path / "missing.nwb"

    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "opencap_to_nwb.cli",
            "inspect",
            str(missing),
        ],
        check=False,
        capture_output=True,
        text=True,
    )

    assert result.returncode == 1
    assert "NWB file does not exist" in result.stderr
