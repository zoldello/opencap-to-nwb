from __future__ import annotations

import numpy as np

from opencap_to_nwb.parsers import parse_mot

from conftest import example_session_dir


def test_parse_mot_returns_time_columns_and_data():
    joint = parse_mot(example_session_dir() / "trial_001.mot")

    assert joint.column_names == [
        "pelvis_tx",
        "pelvis_ty",
        "pelvis_tz",
        "hip_flexion_r",
        "knee_angle_r",
        "ankle_angle_r",
    ]
    assert joint.time.shape == (5,)
    assert joint.data.shape == (5, 6)

    np.testing.assert_allclose(joint.time[:2], [0.0, 0.033333])
    np.testing.assert_allclose(joint.data[0, 4], 12.4)
