from __future__ import annotations

from pathlib import Path

import pytest

from opencap_to_nwb.convert import convert_session
from opencap_to_nwb.nwb_validation import (
    NwbValidationError,
    assert_valid_nwb_file,
    validate_nwb_file,
)


def example_session_dir() -> Path:
    return Path(__file__).resolve().parents[1] / "example_data" / "session_001"


def test_generated_nwb_file_passes_validation(tmp_path):
    output_path = tmp_path / "session_001.nwb"

    convert_session(example_session_dir(), output_path)

    result = validate_nwb_file(output_path)

    assert result.is_valid
    assert result.errors == ()


def test_assert_valid_nwb_file_rejects_missing_file(tmp_path):
    missing_path = tmp_path / "missing.nwb"

    with pytest.raises(NwbValidationError, match="does not exist"):
        assert_valid_nwb_file(missing_path)


def test_assert_valid_nwb_file_rejects_non_nwb_extension(tmp_path):
    bad_path = tmp_path / "not_nwb.txt"
    bad_path.write_text("not an NWB file")

    with pytest.raises(NwbValidationError, match=".nwb extension"):
        assert_valid_nwb_file(bad_path)
