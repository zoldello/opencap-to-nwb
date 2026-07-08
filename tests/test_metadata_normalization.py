from __future__ import annotations

from opencap_to_nwb.metadata_normalization import (
    infer_activity_from_trial_name,
    normalize_session_metadata,
)
from opencap_to_nwb.models import SessionMetadata


def test_normalize_session_metadata_fills_missing_fields_from_context(tmp_path):
    input_dir = tmp_path / "subject0"
    input_dir.mkdir()

    metadata = SessionMetadata(
        height_m=1.78,
        mass_kg=72.0,
        sex="M",
    )

    normalized = normalize_session_metadata(
        metadata,
        input_dir=input_dir,
        trial_name="Squats_0",
        warn=False,
    )

    assert normalized.subject_id == "subject0"
    assert normalized.session_id == "subject0"
    assert normalized.trial_id == "Squats_0"
    assert normalized.activity == "squat"


def test_normalize_session_metadata_does_not_overwrite_explicit_values(tmp_path):
    input_dir = tmp_path / "subject0"
    input_dir.mkdir()

    metadata = SessionMetadata(
        subject_id="sub-001",
        session_id="ses-001",
        trial_id="walk-001",
        activity="walking",
        height_m=1.76,
        mass_kg=72.5,
        sex="M",
    )

    normalized = normalize_session_metadata(
        metadata,
        input_dir=input_dir,
        trial_name="Squats_0",
        warn=False,
    )

    assert normalized.subject_id == "sub-001"
    assert normalized.session_id == "ses-001"
    assert normalized.trial_id == "walk-001"
    assert normalized.activity == "walking"


def test_normalize_session_metadata_preserves_extra_metadata(tmp_path):
    input_dir = tmp_path / "subject0"
    input_dir.mkdir()

    metadata = SessionMetadata(
        extra={
            "openSimModel": "LaiArnoldModified2017_poly_withArms_weldHand",
            "iphoneModel": {"Cam0": "iPhone12,1"},
        }
    )

    normalized = normalize_session_metadata(
        metadata,
        input_dir=input_dir,
        trial_name="Squats_0",
        warn=False,
    )

    assert normalized.extra == metadata.extra


def test_infer_activity_from_trial_name():
    assert infer_activity_from_trial_name("Squats_0") == "squat"
    assert infer_activity_from_trial_name("Asym_0") == "asymmetric squat"
    assert infer_activity_from_trial_name("trial_001") is None
