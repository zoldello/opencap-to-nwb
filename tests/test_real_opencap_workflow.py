from __future__ import annotations

from pathlib import Path

import pytest

from opencap_to_nwb.discovery import discover_trials


def real_subject_dir() -> Path:
    return Path(__file__).resolve().parents[1] / "test_data" / "subject0"


def test_real_opencap_subject0_squats_workflow(tmp_path):
    pytest.importorskip("pynwb")
    from pynwb import NWBHDF5IO

    from opencap_to_nwb.convert import convert_session

    input_dir = real_subject_dir()

    trials = discover_trials(input_dir)
    trial_names = {trial.name for trial in trials}

    assert "Squats_0" in trial_names
    assert "Asym_0" in trial_names
    assert "neutral" not in trial_names

    output_path = tmp_path / "subject0_squats.nwb"

    with pytest.warns(UserWarning):
        convert_session(
            input_dir=input_dir,
            output_path=output_path,
            trial="Squats_0",
        )
    assert output_path.exists()

    with NWBHDF5IO(str(output_path), "r") as io:
        nwb = io.read()

        assert nwb.subject is not None
        assert nwb.subject.subject_id == "subject0"
        assert nwb.session_id == "subject0"
        assert "Squats_0" in nwb.identifier
        assert "Activity: squat." in nwb.session_description
        assert "trial_id=Squats_0" in nwb.subject.description
        assert "activity=squat" in nwb.subject.description

        behavior = nwb.processing["behavior"]

        assert "OpenCapPose3D" in behavior.data_interfaces
        assert "OpenCapJointAngles" in behavior.data_interfaces

        pose = behavior["OpenCapPose3D"]
        joint_angles = behavior["OpenCapJointAngles"]

        assert pose.data.shape == (832, 189)
        assert joint_angles.data.shape == (832, 35)

        pose_timestamps = pose.timestamps[:]
        joint_timestamps = joint_angles.timestamps[:]

        assert len(pose_timestamps) == 832
        assert len(joint_timestamps) == 832
        assert pose_timestamps[0] == pytest.approx(0.0)
        assert joint_timestamps[0] == pytest.approx(0.0)
        assert pose_timestamps[-1] == pytest.approx(joint_timestamps[-1])

        assert "Source file:" in pose.description
        assert "MarkerData/Squats_0.trc" in pose.description
        assert "Source file:" in joint_angles.description
        assert "OpenSimData/IK/Squats_0.mot" in joint_angles.description


def test_real_opencap_subject0_squats_workflow_with_emg(tmp_path):
    pytest.importorskip("pynwb")
    from pynwb import NWBHDF5IO

    from opencap_to_nwb.convert import convert_session

    input_dir = real_subject_dir()
    emg_path = input_dir / "EMG" / "Squats_0_synthetic_raw_emg.csv"
    output_path = tmp_path / "subject0_squats_with_emg.nwb"

    with pytest.warns(UserWarning):
        convert_session(
            input_dir=input_dir,
            output_path=output_path,
            trial="Squats_0",
            emg_path=emg_path,
        )

    with NWBHDF5IO(str(output_path), "r") as io:
        nwb = io.read()

        assert "RawEMG" in nwb.acquisition

        emg = nwb.acquisition["RawEMG"]
        emg_timestamps = emg.timestamps[:]

        assert emg.data.shape[0] == len(emg_timestamps)
        assert emg.data.shape[1] == 10
        assert emg.unit == "mV"
        assert emg_timestamps[0] == pytest.approx(0.0)
        assert "No automatic synchronization" in emg.description
        assert "Squats_0_synthetic_raw_emg.csv" in emg.comments
