"""Internal data models for opencap-to-nwb.

These models are intentionally small. The goal is to parse OpenCap/OpenSim-style
files into a simple common representation before writing NWB.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import numpy as np


@dataclass(slots=True)
class SessionMetadata:
    """Subject/session metadata used by the NWB writer."""

    height_m: float | None = None
    mass_kg: float | None = None
    sex: str | None = None
    subject_id: str = "unknown"
    session_id: str = "unknown-session"
    trial_id: str = "unknown-trial"
    activity: str | None = None
    source_video: str | None = None
    extra: dict[str, Any] = field(default_factory=dict)


@dataclass(slots=True)
class PoseData:
    """3D marker/body-point positions parsed from a TRC file.

    positions shape: (num_timepoints, num_markers, 3)
    units: usually "m" or "mm", depending on the source TRC.
    """

    time: np.ndarray
    marker_names: list[str]
    positions: np.ndarray
    units: str = "unknown"
    source_path: Path | None = None


@dataclass(slots=True)
class JointAngleData:
    """Joint angle / coordinate time series parsed from a MOT file.

    data shape: (num_timepoints, num_columns)
    column_names excludes the time column.
    """

    time: np.ndarray
    column_names: list[str]
    data: np.ndarray
    units: str = "mixed or unknown"
    source_path: Path | None = None


@dataclass(slots=True)
class OpenCapSession:
    """Parsed OpenCap session ready for NWB writing."""

    metadata: SessionMetadata
    pose: PoseData
    joint_angles: JointAngleData
    input_dir: Path
