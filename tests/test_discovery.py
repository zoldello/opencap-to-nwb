from __future__ import annotations

from pathlib import Path

import pytest

from opencap_to_nwb.discovery import DiscoveryError, discover_trials, resolve_trial_files


def _write_file(path: Path, text: str = "x") -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    return path


def test_discover_trials_from_real_opencap_layout(tmp_path):
    input_dir = tmp_path / "subject0"
    _write_file(input_dir / "metadata.yaml", "subject_id: sub-001\n")
    _write_file(input_dir / "MarkerData" / "Squats_0.trc")
    _write_file(input_dir / "MarkerData" / "Asym_0.trc")
    _write_file(input_dir / "MarkerData" / "neutral.trc")
    _write_file(input_dir / "OpenSimData" / "IK" / "Squats_0.mot")
    _write_file(input_dir / "OpenSimData" / "IK" / "Asym_0.mot")
    _write_file(input_dir / "OpenSimData" / "Dynamics" / "Squats_0_1" / "kinetics.mot")

    trials = discover_trials(input_dir)

    assert [trial.name for trial in trials] == ["Asym_0", "Squats_0"]
    assert all("OpenSimData/IK" in trial.mot_path.as_posix() for trial in trials)


def test_resolve_trial_files_requires_trial_when_multiple_real_trials(tmp_path):
    input_dir = tmp_path / "subject0"
    _write_file(input_dir / "metadata.yaml", "subject_id: sub-001\n")
    _write_file(input_dir / "MarkerData" / "Squats_0.trc")
    _write_file(input_dir / "MarkerData" / "Asym_0.trc")
    _write_file(input_dir / "OpenSimData" / "IK" / "Squats_0.mot")
    _write_file(input_dir / "OpenSimData" / "IK" / "Asym_0.mot")

    with pytest.raises(DiscoveryError, match="Multiple OpenCap trials"):
        resolve_trial_files(input_dir)


def test_resolve_trial_files_selects_named_real_trial(tmp_path):
    input_dir = tmp_path / "subject0"
    metadata = _write_file(input_dir / "metadata.yaml", "subject_id: sub-001\n")
    trc = _write_file(input_dir / "MarkerData" / "Squats_0.trc")
    mot = _write_file(input_dir / "OpenSimData" / "IK" / "Squats_0.mot")

    trial_files = resolve_trial_files(input_dir, trial_name="Squats_0")

    assert trial_files.metadata_path == metadata
    assert trial_files.trc_path == trc
    assert trial_files.mot_path == mot
    assert trial_files.trial_name == "Squats_0"


def test_resolve_trial_files_supports_explicit_paths(tmp_path):
    input_dir = tmp_path / "input"
    metadata = _write_file(input_dir / "my_metadata.yaml", "subject_id: sub-001\n")
    trc = _write_file(input_dir / "custom.trc")
    mot = _write_file(input_dir / "custom_angles.mot")

    trial_files = resolve_trial_files(
        input_dir,
        metadata_path=metadata,
        trc_path=trc,
        mot_path=mot,
    )

    assert trial_files.metadata_path == metadata
    assert trial_files.trc_path == trc
    assert trial_files.mot_path == mot
    assert trial_files.trial_name == "custom"


def test_resolve_trial_files_rejects_multiple_flat_files_without_trial(tmp_path):
    input_dir = tmp_path / "input"
    _write_file(input_dir / "metadata.yaml", "subject_id: sub-001\n")
    _write_file(input_dir / "a.trc")
    _write_file(input_dir / "b.trc")
    _write_file(input_dir / "a.mot")
    _write_file(input_dir / "b.mot")

    with pytest.raises(DiscoveryError, match="Multiple TRC files"):
        resolve_trial_files(input_dir)
