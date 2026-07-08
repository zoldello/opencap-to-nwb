from __future__ import annotations

import pytest

from opencap_to_nwb.convert import convert_session

from conftest import example_session_dir


def test_convert_session_writes_readable_nwb(tmp_path):
    pytest.importorskip("pynwb")
    from pynwb import NWBHDF5IO

    output = tmp_path / "session_001.nwb"
    convert_session(example_session_dir(), output)

    assert output.exists()

    with NWBHDF5IO(str(output), "r") as io:
        nwbfile = io.read()
        assert nwbfile.subject.subject_id == "sub-001"
        assert "behavior" in nwbfile.processing

        behavior = nwbfile.processing["behavior"]
        assert "OpenCapPose3D" in behavior.data_interfaces
        assert "OpenCapJointAngles" in behavior.data_interfaces

        pose_ts = behavior.data_interfaces["OpenCapPose3D"]
        joint_ts = behavior.data_interfaces["OpenCapJointAngles"]

        assert pose_ts.data.shape == (5, 9)
        assert joint_ts.data.shape == (5, 6)
