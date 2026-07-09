from __future__ import annotations

import pytest

from opencap_to_nwb.emg import parse_emg_csv
from opencap_to_nwb.parsers import ParseError


def test_parse_emg_csv(tmp_path):
    emg_path = tmp_path / "emg.csv"
    emg_path.write_text(
        "time,TA_R,TA_L\n"
        "0.000,0.01,-0.02\n"
        "0.001,0.03,-0.01\n"
        "0.002,-0.02,0.02\n"
    )

    emg = parse_emg_csv(emg_path)

    assert emg.channel_names == ["TA_R", "TA_L"]
    assert emg.units == "mV"
    assert emg.source_path == emg_path
    assert emg.time.shape == (3,)
    assert emg.data.shape == (3, 2)
    assert emg.time[0] == pytest.approx(0.0)


def test_parse_emg_csv_requires_time_column(tmp_path):
    emg_path = tmp_path / "bad_emg.csv"
    emg_path.write_text("sample,TA_R\n0,0.01\n")

    with pytest.raises(ParseError, match="first column must be"):
        parse_emg_csv(emg_path)


def test_parse_emg_csv_requires_increasing_time(tmp_path):
    emg_path = tmp_path / "bad_time_emg.csv"
    emg_path.write_text("time,TA_R\n0.000,0.01\n0.000,0.02\n")

    with pytest.raises(ParseError, match="strictly increasing"):
        parse_emg_csv(emg_path)


def test_parse_subject0_synthetic_emg_fixture():
    fixture = (
        __import__("pathlib").Path(__file__).resolve().parents[1]
        / "test_data"
        / "subject0"
        / "EMGData"
        / "Squats_0_synthetic_raw_emg.csv"
    )

    emg = parse_emg_csv(fixture)

    assert emg.time[0] == pytest.approx(0.0)
    assert emg.data.shape[0] == len(emg.time)
    assert emg.data.shape[1] == 10
    assert "sync_pulse" not in emg.channel_names
