"""High-level conversion workflow for OpenCap/OpenSim-style sessions."""

from __future__ import annotations

from pathlib import Path

from .models import OpenCapSession
from .nwb_validation import assert_valid_nwb_file
from .nwb_writer import write_nwb
from .parsers import parse_metadata, parse_mot, parse_trc


class ConversionError(RuntimeError):
    """Raised when a session cannot be converted."""


def _find_one_file(input_dir: Path, pattern: str, label: str) -> Path:
    """Find one file matching a pattern inside an input directory.

    V1 chooses the first sorted match if multiple files are found. Later
    versions should support explicit trial/file selection.
    """
    matches = sorted(input_dir.glob(pattern))

    if not matches:
        raise ConversionError(f"No {label} file matching {pattern!r} found in {input_dir}")

    return matches[0]


def convert_session(
    input_dir: str | Path,
    output_path: str | Path,
    *,
    validate_nwb: bool = True,
) -> Path:
    """Convert an OpenCap/OpenSim-style session directory to NWB.

    Expected V1 input files:
    - metadata.yaml
    - one .trc file
    - one .mot file

    Parameters
    ----------
    input_dir:
        Directory containing the OpenCap/OpenSim-style input files.
    output_path:
        Path where the NWB file should be written.
    validate_nwb:
        If True, validate the generated NWB file after writing.

    Returns
    -------
    Path
        The path to the generated NWB file.
    """
    input_dir = Path(input_dir)
    output_path = Path(output_path)

    if not input_dir.exists():
        raise ConversionError(f"Input directory does not exist: {input_dir}")

    if not input_dir.is_dir():
        raise ConversionError(f"Input path is not a directory: {input_dir}")

    metadata_path = input_dir / "metadata.yaml"
    if not metadata_path.exists():
        raise ConversionError(f"Missing metadata file: {metadata_path}")

    trc_path = _find_one_file(input_dir, "*.trc", "TRC")
    mot_path = _find_one_file(input_dir, "*.mot", "MOT")

    metadata = parse_metadata(metadata_path)
    pose = parse_trc(trc_path)
    joint_angles = parse_mot(mot_path)

    session = OpenCapSession(
        metadata=metadata,
        pose=pose,
        joint_angles=joint_angles,
        input_dir=input_dir,
    )

    write_nwb(session, output_path)

    if validate_nwb:
        assert_valid_nwb_file(output_path)

    return output_path
