# tests/test_validation.py

from __future__ import annotations

import pytest

from opencap_to_nwb.parsers import ParseError, parse_metadata, parse_mot, parse_trc


def test_parse_metadata_rejects_non_mapping_yaml(tmp_path):
    path = tmp_path / "metadata.yaml"
    path.write_text("- not\n- a\n- mapping\n")

    with pytest.raises(ParseError, match="YAML mapping"):
        parse_metadata(path)


def test_parse_metadata_rejects_negative_height(tmp_path):
    path = tmp_path / "metadata.yaml"
    path.write_text("height_m: -1.76\nmass_kg: 72.5\nsex: male\n")

    with pytest.raises(ParseError, match="height_m"):
        parse_metadata(path)


def test_parse_metadata_rejects_negative_mass(tmp_path):
    path = tmp_path / "metadata.yaml"
    path.write_text("height_m: 1.76\nmass_kg: -72.5\nsex: male\n")

    with pytest.raises(ParseError, match="mass_kg"):
        parse_metadata(path)


def test_parse_trc_rejects_missing_frame_time_header(tmp_path):
    path = tmp_path / "bad.trc"
    path.write_text("not a real trc\n")

    with pytest.raises(ParseError, match="Frame#/Time"):
        parse_trc(path)


def test_parse_trc_rejects_non_increasing_frame_column(tmp_path):
    path = tmp_path / "bad_frame.trc"
    path.write_text(
        "\n".join(
            [
                "PathFileType\t4\t(X/Y/Z)\tbad_frame.trc",
                "DataRate\tCameraRate\tNumFrames\tNumMarkers\tUnits\tOrigDataRate\tOrigDataStartFrame\tOrigNumFrames",
                "30.00\t30.00\t2\t1\tm\t30.00\t1\t2",
                "Frame#\tTime\tHip_R",
                "\t\tX1\tY1\tZ1",
                "1\t0.000000\t0.140\t0.920\t0.030",
                "1\t0.033333\t0.142\t0.921\t0.031",
            ]
        )
    )

    with pytest.raises(ParseError, match="TRC frame column must be strictly increasing"):
        parse_trc(path)


def test_parse_trc_rejects_non_increasing_time(tmp_path):
    path = tmp_path / "bad_time.trc"
    path.write_text(
        "\n".join(
            [
                "PathFileType\t4\t(X/Y/Z)\tbad_time.trc",
                "DataRate\tCameraRate\tNumFrames\tNumMarkers\tUnits\tOrigDataRate\tOrigDataStartFrame\tOrigNumFrames",
                "30.00\t30.00\t2\t1\tm\t30.00\t1\t2",
                "Frame#\tTime\tHip_R",
                "\t\tX1\tY1\tZ1",
                "1\t0.000000\t0.140\t0.920\t0.030",
                "2\t0.000000\t0.142\t0.921\t0.031",
            ]
        )
    )

    with pytest.raises(ParseError, match="TRC time column must be strictly increasing"):
        parse_trc(path)


def test_parse_trc_rejects_num_markers_mismatch(tmp_path):
    path = tmp_path / "bad_marker_count.trc"
    path.write_text(
        "\n".join(
            [
                "PathFileType\t4\t(X/Y/Z)\tbad_marker_count.trc",
                "DataRate\tCameraRate\tNumFrames\tNumMarkers\tUnits\tOrigDataRate\tOrigDataStartFrame\tOrigNumFrames",
                "30.00\t30.00\t2\t2\tm\t30.00\t1\t2",
                "Frame#\tTime\tHip_R",
                "\t\tX1\tY1\tZ1",
                "1\t0.000000\t0.140\t0.920\t0.030",
                "2\t0.033333\t0.142\t0.921\t0.031",
            ]
        )
    )

    with pytest.raises(ParseError, match="TRC NumMarkers mismatch"):
        parse_trc(path)


def test_parse_trc_rejects_num_frames_mismatch(tmp_path):
    path = tmp_path / "bad_frame_count.trc"
    path.write_text(
        "\n".join(
            [
                "PathFileType\t4\t(X/Y/Z)\tbad_frame_count.trc",
                "DataRate\tCameraRate\tNumFrames\tNumMarkers\tUnits\tOrigDataRate\tOrigDataStartFrame\tOrigNumFrames",
                "30.00\t30.00\t3\t1\tm\t30.00\t1\t3",
                "Frame#\tTime\tHip_R",
                "\t\tX1\tY1\tZ1",
                "1\t0.000000\t0.140\t0.920\t0.030",
                "2\t0.033333\t0.142\t0.921\t0.031",
            ]
        )
    )

    with pytest.raises(ParseError, match="TRC NumFrames mismatch"):
        parse_trc(path)


def test_parse_trc_rejects_bad_coordinate_label_count(tmp_path):
    path = tmp_path / "bad_coordinate_labels.trc"
    path.write_text(
        "\n".join(
            [
                "PathFileType\t4\t(X/Y/Z)\tbad_coordinate_labels.trc",
                "DataRate\tCameraRate\tNumFrames\tNumMarkers\tUnits\tOrigDataRate\tOrigDataStartFrame\tOrigNumFrames",
                "30.00\t30.00\t1\t1\tm\t30.00\t1\t1",
                "Frame#\tTime\tHip_R",
                "\t\tX1\tY1",
                "1\t0.000000\t0.140\t0.920\t0.030",
            ]
        )
    )

    with pytest.raises(ParseError, match="coordinate-label count mismatch"):
        parse_trc(path)


def test_parse_trc_rejects_row_width_mismatch(tmp_path):
    path = tmp_path / "bad_row_width.trc"
    path.write_text(
        "\n".join(
            [
                "PathFileType\t4\t(X/Y/Z)\tbad_row_width.trc",
                "DataRate\tCameraRate\tNumFrames\tNumMarkers\tUnits\tOrigDataRate\tOrigDataStartFrame\tOrigNumFrames",
                "30.00\t30.00\t1\t1\tm\t30.00\t1\t1",
                "Frame#\tTime\tHip_R",
                "\t\tX1\tY1\tZ1",
                "1\t0.000000\t0.140\t0.920",
            ]
        )
    )

    with pytest.raises(ParseError, match="TRC row has"):
        parse_trc(path)


def test_parse_trc_rejects_non_numeric_row(tmp_path):
    path = tmp_path / "bad_numeric.trc"
    path.write_text(
        "\n".join(
            [
                "PathFileType\t4\t(X/Y/Z)\tbad_numeric.trc",
                "DataRate\tCameraRate\tNumFrames\tNumMarkers\tUnits\tOrigDataRate\tOrigDataStartFrame\tOrigNumFrames",
                "30.00\t30.00\t1\t1\tm\t30.00\t1\t1",
                "Frame#\tTime\tHip_R",
                "\t\tX1\tY1\tZ1",
                "1\t0.000000\tbad\t0.920\t0.030",
            ]
        )
    )

    with pytest.raises(ParseError, match="Non-numeric TRC data row"):
        parse_trc(path)


def test_parse_mot_rejects_missing_time_header(tmp_path):
    path = tmp_path / "bad.mot"
    path.write_text("name bad\nendheader\nnot_time\tknee_angle_r\n0.0\t1.0\n")

    with pytest.raises(ParseError, match="first data column must be time"):
        parse_mot(path)


def test_parse_mot_rejects_missing_data_column_after_time(tmp_path):
    path = tmp_path / "bad_only_time.mot"
    path.write_text(
        "\n".join(
            [
                "name bad_only_time",
                "datacolumns 1",
                "datarows 1",
                "endheader",
                "time",
                "0.000000",
            ]
        )
    )

    with pytest.raises(ParseError, match="at least one data column"):
        parse_mot(path)


def test_parse_mot_rejects_non_increasing_time(tmp_path):
    path = tmp_path / "bad_time.mot"
    path.write_text(
        "\n".join(
            [
                "name bad_time",
                "datacolumns 3",
                "datarows 2",
                "range 0.000000 0.033333",
                "endheader",
                "time\tknee_angle_r\thip_flexion_r",
                "0.000000\t12.4\t25.0",
                "0.000000\t13.1\t26.2",
            ]
        )
    )

    with pytest.raises(ParseError, match="MOT time column must be strictly increasing"):
        parse_mot(path)


def test_parse_mot_rejects_row_width_mismatch(tmp_path):
    path = tmp_path / "bad_row_width.mot"
    path.write_text(
        "\n".join(
            [
                "name bad_row_width",
                "datacolumns 3",
                "datarows 2",
                "range 0.000000 0.033333",
                "endheader",
                "time\tknee_angle_r\thip_flexion_r",
                "0.000000\t12.4\t25.0",
                "0.033333\t13.1",
            ]
        )
    )

    with pytest.raises(ParseError, match="MOT row has"):
        parse_mot(path)


def test_parse_mot_rejects_non_numeric_row(tmp_path):
    path = tmp_path / "bad_numeric.mot"
    path.write_text(
        "\n".join(
            [
                "name bad_numeric",
                "datacolumns 2",
                "datarows 1",
                "endheader",
                "time\tknee_angle_r",
                "0.000000\tbad",
            ]
        )
    )

    with pytest.raises(ParseError, match="Non-numeric MOT data row"):
        parse_mot(path)


def test_parse_mot_rejects_datarows_mismatch(tmp_path):
    path = tmp_path / "bad_datarows.mot"
    path.write_text(
        "\n".join(
            [
                "name bad_datarows",
                "datacolumns 2",
                "datarows 3",
                "endheader",
                "time\tknee_angle_r",
                "0.000000\t12.4",
                "0.033333\t13.1",
            ]
        )
    )

    with pytest.raises(ParseError, match="MOT datarows mismatch"):
        parse_mot(path)


def test_parse_mot_rejects_datacolumns_mismatch(tmp_path):
    path = tmp_path / "bad_datacolumns.mot"
    path.write_text(
        "\n".join(
            [
                "name bad_datacolumns",
                "datacolumns 3",
                "datarows 2",
                "endheader",
                "time\tknee_angle_r",
                "0.000000\t12.4",
                "0.033333\t13.1",
            ]
        )
    )

    with pytest.raises(ParseError, match="MOT datacolumns mismatch"):
        parse_mot(path)
    