"""NWB writer for parsed OpenCap sessions."""

from __future__ import annotations

from datetime import datetime, timezone
from pathlib import Path

import numpy as np
from pynwb import NWBHDF5IO, NWBFile, TimeSeries
from pynwb.file import Subject

from .models import OpenCapSession


def _normalize_sex_for_nwb(sex: str | None) -> str:
    """Map free-text sex to NWB-style compact values when possible."""

    if sex is None:
        return "U"

    cleaned = sex.strip().lower()
    if cleaned in {"m", "male"}:
        return "M"
    if cleaned in {"f", "female"}:
        return "F"
    if cleaned in {"u", "unknown"}:
        return "U"
    if cleaned in {"o", "other"}:
        return "O"
    return sex


def write_nwb(session: OpenCapSession, output_path: str | Path) -> Path:
    """Write an OpenCapSession to an NWB file.

    V1 stores pose and joint-angle data as generic TimeSeries inside a
    `behavior` processing module. This keeps the first version simple and avoids
    requiring `ndx-pose` before testing against real OpenCap output.
    """

    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    metadata = session.metadata
    identifier = f"{metadata.subject_id}-{metadata.session_id}-{metadata.trial_id}"

    description_parts = [
        "OpenCap/OpenSim-style movement outputs converted to NWB.",
        "V1 stores pose and joint angles as generic TimeSeries.",
    ]
    if metadata.activity:
        description_parts.append(f"Activity: {metadata.activity}.")
    if metadata.source_video:
        description_parts.append(f"Source video: {metadata.source_video}.")

    nwbfile = NWBFile(
        session_description=" ".join(description_parts),
        identifier=identifier,
        session_start_time=datetime.now(timezone.utc),
        session_id=metadata.session_id,
        experiment_description="OpenCap to NWB conversion.",
        notes="Created by opencap-to-nwb scaffold. Validate against real OpenCap output before scientific use.",
    )

    nwbfile.subject = Subject(
        subject_id=metadata.subject_id,
        sex=_normalize_sex_for_nwb(metadata.sex),
        description=(
            f"height_m={metadata.height_m}; mass_kg={metadata.mass_kg}; "
            f"trial_id={metadata.trial_id}; activity={metadata.activity}"
        ),
        species="Homo sapiens",
    )

    behavior_module = nwbfile.create_processing_module(
        name="behavior",
        description="OpenCap-derived pose and joint-angle data.",
    )

    pose = session.pose
    pose_flat = pose.positions.reshape((pose.positions.shape[0], -1))
    pose_columns = [
        f"{marker}_{axis}" for marker in pose.marker_names for axis in ("x", "y", "z")
    ]
    pose_description = (
        "3D marker/body-point positions parsed from an OpenCap/OpenSim-style TRC file. "
        f"Columns: {', '.join(pose_columns)}. "
        f"Source file: {pose.source_path}."
    )

    pose_ts = TimeSeries(
        name="OpenCapPose3D",
        data=pose_flat,
        unit=pose.units,
        timestamps=pose.time,
        description=pose_description,
        comments="Rows are timepoints. Columns are marker XYZ coordinates flattened as marker_x, marker_y, marker_z.",
    )
    behavior_module.add(pose_ts)

    joint = session.joint_angles
    joint_description = (
        "Joint angle / coordinate time series parsed from an OpenCap/OpenSim-style MOT file. "
        f"Columns: {', '.join(joint.column_names)}. "
        f"Source file: {joint.source_path}."
    )
    joint_ts = TimeSeries(
        name="OpenCapJointAngles",
        data=joint.data,
        unit=joint.units,
        timestamps=joint.time,
        description=joint_description,
        comments="Rows are timepoints. Columns are MOT columns excluding time.",
    )
    behavior_module.add(joint_ts)

    source_summary = {
        "input_dir": str(session.input_dir),
        "trc": str(pose.source_path) if pose.source_path else None,
        "mot": str(joint.source_path) if joint.source_path else None,
        "source_video": metadata.source_video,
    }
    if hasattr(nwbfile, "add_scratch"):
        try:
            nwbfile.add_scratch(
                np.asarray([str(source_summary)], dtype="S"),
                name="opencap_source_files",
                description="Source-file references used during conversion.",
            )
        except Exception:
            # Scratch is helpful, not essential. Avoid failing conversion because
            # of a PyNWB version-specific scratch behavior.
            pass

    with NWBHDF5IO(str(output_path), "w") as io:
        io.write(nwbfile)

    return output_path
