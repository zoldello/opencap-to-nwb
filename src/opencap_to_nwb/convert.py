"""High-level conversion workflow for OpenCap/OpenSim-style sessions."""

from __future__ import annotations

from pathlib import Path

from .discovery import DiscoveryError, resolve_trial_files
from .metadata_normalization import normalize_session_metadata
from .models import OpenCapSession
from .nwb_validation import assert_valid_nwb_file
from .nwb_writer import write_nwb
from .parsers import ParseError, parse_metadata, parse_mot, parse_trc


class ConversionError(RuntimeError):
    """Raised when a session cannot be converted."""


def convert_session(
    input_dir: str | Path,
    output_path: str | Path,
    *,
    trial: str | None = None,
    metadata_path: str | Path | None = None,
    trc_path: str | Path | None = None,
    mot_path: str | Path | None = None,
    validate_nwb: bool = True,
) -> Path:
    """Convert an OpenCap/OpenSim-style session to NWB.

    Supported input modes:

    1. Simple staged folder:
       input_dir/
         metadata.yaml
         trial_001.trc
         trial_001.mot

    2. Real OpenCap-style folder:
       input_dir/
         metadata.yaml
         MarkerData/<trial>.trc
         OpenSimData/IK/<trial>.mot

    3. Explicit file paths:
       --metadata path/to/metadata.yaml
       --trc path/to/file.trc
       --mot path/to/file.mot

    Parameters
    ----------
    input_dir:
        Input session or subject directory.
    output_path:
        Path where the NWB file should be written.
    trial:
        Optional trial name to select from a real OpenCap-style folder.
    metadata_path:
        Optional explicit metadata YAML path.
    trc_path:
        Optional explicit TRC path.
    mot_path:
        Optional explicit MOT path.
    validate_nwb:
        If True, validate the generated NWB file after writing.

    Returns
    -------
    Path
        Path to the generated NWB file.
    """
    input_dir = Path(input_dir)
    output_path = Path(output_path)

    if not input_dir.exists():
        raise ConversionError(f"Input directory does not exist: {input_dir}")

    if not input_dir.is_dir():
        raise ConversionError(f"Input path is not a directory: {input_dir}")

    try:
        trial_files = resolve_trial_files(
            input_dir=input_dir,
            trial_name=trial,
            metadata_path=metadata_path,
            trc_path=trc_path,
            mot_path=mot_path,
        )
    except DiscoveryError as exc:
        raise ConversionError(str(exc)) from exc

    try:
        metadata = parse_metadata(trial_files.metadata_path)
        metadata = normalize_session_metadata(
            metadata,
            input_dir=input_dir,
            trial_name=trial_files.trial_name,
        )

        pose = parse_trc(trial_files.trc_path)
        joint_angles = parse_mot(trial_files.mot_path)
    except ParseError as exc:
        raise ConversionError(str(exc)) from exc

    session = OpenCapSession(
        metadata=metadata,
        pose=pose,
        joint_angles=joint_angles,
        input_dir=input_dir,
    )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    write_nwb(session, output_path)

    if validate_nwb:
        assert_valid_nwb_file(output_path)

    return output_path
