from __future__ import annotations

import numpy as np

from opencap_to_nwb.parsers import parse_trc

from conftest import example_session_dir


def test_parse_trc_returns_time_markers_and_positions():
    pose = parse_trc(example_session_dir() / "trial_001.trc")

    assert pose.units == "m"
    assert pose.marker_names == ["Hip_R", "Knee_R", "Ankle_R"]
    assert pose.time.shape == (5,)
    assert pose.positions.shape == (5, 3, 3)

    np.testing.assert_allclose(pose.time[:2], [0.0, 0.033333])
    np.testing.assert_allclose(pose.positions[0, 0, :], [0.140, 0.920, 0.030])
