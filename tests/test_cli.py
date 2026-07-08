from __future__ import annotations

import subprocess
import sys

import pytest

from conftest import example_session_dir


def test_cli_writes_nwb(tmp_path):
    pytest.importorskip("pynwb")

    output = tmp_path / "session_cli.nwb"
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "opencap_to_nwb.cli",
            "--input",
            str(example_session_dir()),
            "--output",
            str(output),
        ],
        check=False,
        capture_output=True,
        text=True,
    )

    assert result.returncode == 0, result.stderr
    assert output.exists()
    assert "Wrote NWB file" in result.stdout


def test_cli_lists_trials(tmp_path):
    input_dir = tmp_path / "subject0"
    (input_dir / "MarkerData").mkdir(parents=True)
    (input_dir / "OpenSimData" / "IK").mkdir(parents=True)
    (input_dir / "metadata.yaml").write_text("subject_id: sub-001\n")
    (input_dir / "MarkerData" / "Squats_0.trc").write_text("placeholder")
    (input_dir / "OpenSimData" / "IK" / "Squats_0.mot").write_text("placeholder")

    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "opencap_to_nwb.cli",
            "list-trials",
            "--input",
            str(input_dir),
        ],
        check=False,
        capture_output=True,
        text=True,
    )

    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == "Squats_0"
