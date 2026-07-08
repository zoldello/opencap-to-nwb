"""Metadata normalization helpers.

These helpers fill missing NWB-facing metadata from file/folder context without
requiring OpenCap metadata.yaml files to follow a strict schema.
"""

from __future__ import annotations

from dataclasses import replace
from pathlib import Path
import warnings

from .models import SessionMetadata

_UNKNOWN_SUBJECT_ID = "unknown"
_UNKNOWN_SESSION_ID = "unknown-session"
_UNKNOWN_TRIAL_ID = "unknown-trial"


def _is_missing_text(value: str | None, unknown_values: set[str]) -> bool:
    """Return True when a metadata text field is missing or a known placeholder."""
    if value is None:
        return True

    stripped = value.strip()
    if not stripped:
        return True

    return stripped in unknown_values


def infer_activity_from_trial_name(trial_name: str | None) -> str | None:
    """Infer a conservative activity label from a trial name.

    This should stay conservative. If the trial name is unclear, return None.
    """
    if trial_name is None:
        return None

    lower = trial_name.lower()

    if "squat" in lower:
        return "squat"

    if "asym" in lower:
        return "asymmetric squat"

    return None


def normalize_session_metadata(
    metadata: SessionMetadata,
    *,
    input_dir: str | Path,
    trial_name: str | None,
    warn: bool = True,
) -> SessionMetadata:
    """Fill missing metadata fields from conversion context.

    Explicit metadata.yaml values always win. This function only fills values
    that are missing or set to known placeholders.
    """
    input_dir = Path(input_dir)

    subject_id = metadata.subject_id
    session_id = metadata.session_id
    trial_id = metadata.trial_id
    activity = metadata.activity

    if _is_missing_text(subject_id, {_UNKNOWN_SUBJECT_ID}):
        subject_id = input_dir.name
        if warn:
            warnings.warn(
                f"metadata.yaml did not define subject_id. "
                f"Using input folder name: {subject_id}",
                stacklevel=2,
            )

    if _is_missing_text(session_id, {_UNKNOWN_SESSION_ID}):
        session_id = input_dir.name
        if warn:
            warnings.warn(
                f"metadata.yaml did not define session_id. "
                f"Using input folder name: {session_id}",
                stacklevel=2,
            )

    if _is_missing_text(trial_id, {_UNKNOWN_TRIAL_ID}) and trial_name:
        trial_id = trial_name
        if warn:
            warnings.warn(
                f"metadata.yaml did not define trial_id. "
                f"Using resolved trial name: {trial_id}",
                stacklevel=2,
            )

    if _is_missing_text(activity, set()):
        inferred_activity = infer_activity_from_trial_name(trial_name)
        if inferred_activity is not None:
            activity = inferred_activity
            if warn:
                warnings.warn(
                    f"metadata.yaml did not define activity. "
                    f"Inferred from trial name: {activity}",
                    stacklevel=2,
                )

    return replace(
        metadata,
        subject_id=subject_id,
        session_id=session_id,
        trial_id=trial_id,
        activity=activity,
    )
