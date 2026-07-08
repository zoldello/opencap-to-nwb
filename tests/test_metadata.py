from __future__ import annotations

from opencap_to_nwb.parsers import parse_metadata

from conftest import example_session_dir


def test_parse_metadata_loads_subject_and_session_fields():
    metadata = parse_metadata(example_session_dir() / "metadata.yaml")

    assert metadata.subject_id == "sub-001"
    assert metadata.session_id == "ses-001"
    assert metadata.trial_id == "walk-001"
    assert metadata.height_m == 1.76
    assert metadata.mass_kg == 72.5
    assert metadata.sex == "male"
    assert metadata.activity == "walking"
