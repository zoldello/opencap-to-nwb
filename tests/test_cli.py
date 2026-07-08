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
