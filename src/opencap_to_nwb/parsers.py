"""Parsers for OpenCap/OpenSim-style metadata, TRC, and MOT files."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

import numpy as np
import yaml

from .models import JointAngleData, PoseData, SessionMetadata


class ParseError(ValueError):
    """Raised when an input file cannot be parsed."""


@dataclass(frozen=True, slots=True)
class _TrcHeader:
    """Internal representation of the important TRC header fields."""

    marker_names: tuple[str, ...]
    declared_num_frames: int | None
    declared_num_markers: int | None
    units: str
    header_idx: int


@dataclass(frozen=True, slots=True)
class _MotHeader:
    """Internal representation of the important MOT header fields."""

    columns: tuple[str, ...]
    declared_num_rows: int | None
    declared_num_columns: int | None
    header_idx: int


def _read_lines(path: Path, file_label: str) -> list[str]:
    """Read a text file and return lines, raising ParseError on failure."""
    try:
        lines = path.read_text().splitlines()
    except OSError as exc:
        raise ParseError(f"Could not read {file_label} file {path}: {exc}") from exc

    if not lines:
        raise ParseError(f"Empty {file_label} file: {path}")

    return lines


def _ensure_unique_nonempty_names(
    names: list[str], file_label: str, path: Path
) -> None:
    """Validate that parsed column/marker names are non-empty and unique."""
    if not names:
        raise ParseError(f"{file_label} has no data names in {path}")

    empty_names = [name for name in names if not name.strip()]
    if empty_names:
        raise ParseError(f"{file_label} contains empty names in {path}")

    duplicates = sorted({name for name in names if names.count(name) > 1})
    if duplicates:
        raise ParseError(
            f"{file_label} contains duplicate names in {path}: {duplicates}"
        )


def _ensure_finite_array(values: np.ndarray, file_label: str, path: Path) -> None:
    """Validate that all parsed numeric values are finite."""
    if not np.all(np.isfinite(values)):
        bad = np.argwhere(~np.isfinite(values))[0]
        raise ParseError(
            f"{file_label} contains non-finite numeric value in {path} "
            f"at parsed row {int(bad[0])}, column {int(bad[1])}"
        )


def _ensure_strictly_increasing(
    values: np.ndarray, field_name: str, file_label: str, path: Path
) -> None:
    """Validate that a numeric vector is strictly increasing."""
    if values.size < 2:
        return

    diffs = np.diff(values)
    if np.any(diffs <= 0):
        bad_index = int(np.where(diffs <= 0)[0][0])
        raise ParseError(
            f"{file_label} {field_name} must be strictly increasing in {path}. "
            f"Problem around parsed rows {bad_index} and {bad_index + 1}: "
            f"{values[bad_index]} -> {values[bad_index + 1]}"
        )


def _parse_positive_float_or_none(
    value: Any, field_name: str, path: Path
) -> float | None:
    """Parse optional positive numeric metadata fields."""
    if value is None:
        return None

    try:
        parsed = float(value)
    except (TypeError, ValueError) as exc:
        raise ParseError(
            f"Metadata field {field_name!r} must be numeric in {path}"
        ) from exc

    if parsed <= 0:
        raise ParseError(f"Metadata field {field_name!r} must be positive in {path}")

    return parsed


def _parse_optional_positive_int(
    value: str | None, field_name: str, file_label: str, path: Path
) -> int | None:
    """Parse optional positive integer header fields.

    Some OpenSim-style headers store integer counts as values like "5" or "5.0",
    so this accepts float-looking integers.
    """
    if value is None or value == "":
        return None

    try:
        parsed_float = float(value)
    except ValueError as exc:
        raise ParseError(
            f"{file_label} header field {field_name!r} must be numeric in {path}"
        ) from exc

    parsed_int = int(parsed_float)
    if parsed_int <= 0 or parsed_int != parsed_float:
        raise ParseError(
            f"{file_label} header field {field_name!r} must be a positive integer in {path}"
        )

    return parsed_int


def _parse_metadata_string(raw: dict[str, Any], key: str) -> str | None:
    """Return a stripped metadata string or None."""
    value = raw.get(key)
    if value is None:
        return None

    text = str(value).strip()
    return text or None


def parse_metadata(path: str | Path) -> SessionMetadata:
    """Parse a YAML metadata file.

    Required OpenCap-style fields, when present:
    - height_m
    - mass_kg
    - sex

    Extra fields are preserved in metadata.extra.
    """

    path = Path(path)

    try:
        raw = yaml.safe_load(path.read_text()) or {}
    except OSError as exc:
        raise ParseError(f"Could not read metadata file {path}: {exc}") from exc
    except yaml.YAMLError as exc:
        raise ParseError(f"Could not parse YAML metadata file {path}: {exc}") from exc

    if not isinstance(raw, dict):
        raise ParseError(f"Metadata file must contain a YAML mapping: {path}")

    known = {
        "height_m",
        "mass_kg",
        "sex",
        "subject_id",
        "session_id",
        "trial_id",
        "activity",
        "source_video",
    }
    extra = {k: v for k, v in raw.items() if k not in known}

    return SessionMetadata(
        height_m=_parse_positive_float_or_none(raw.get("height_m"), "height_m", path),
        mass_kg=_parse_positive_float_or_none(raw.get("mass_kg"), "mass_kg", path),
        sex=_parse_metadata_string(raw, "sex"),
        subject_id=_parse_metadata_string(raw, "subject_id") or "unknown",
        session_id=_parse_metadata_string(raw, "session_id") or "unknown-session",
        trial_id=_parse_metadata_string(raw, "trial_id") or "unknown-trial",
        activity=_parse_metadata_string(raw, "activity"),
        source_video=_parse_metadata_string(raw, "source_video"),
        extra=extra,
    )


def _get_tab_value(tokens: list[str], values: list[str], field_name: str) -> str | None:
    """Get a value from a tab-delimited header value row."""
    index = tokens.index(field_name)
    if index >= len(values):
        return None
    value = values[index].strip()
    return value or None


def _parse_trc_header(lines: list[str], path: Path) -> _TrcHeader:
    """Parse and validate the TRC header."""
    header_idx = None
    for i, line in enumerate(lines):
        if line.strip().startswith("Frame#"):
            header_idx = i
            break

    if header_idx is None:
        raise ParseError(f"Could not find TRC Frame#/Time header in {path}")

    header_tokens = [
        token.strip() for token in lines[header_idx].split("\t") if token.strip()
    ]
    if len(header_tokens) < 3:
        raise ParseError(f"TRC header has no marker names in {path}")

    if header_tokens[0] != "Frame#" or header_tokens[1].lower() != "time":
        raise ParseError(f"TRC data header must start with Frame# and Time in {path}")

    marker_names = header_tokens[2:]
    _ensure_unique_nonempty_names(marker_names, "TRC marker header", path)

    declared_num_frames = None
    declared_num_markers = None
    units = "unknown"

    for i, line in enumerate(lines):
        tokens = [token.strip() for token in line.split("\t")]
        if "NumFrames" in tokens and "NumMarkers" in tokens and "Units" in tokens:
            if i + 1 >= len(lines):
                raise ParseError(f"TRC metadata header has no value row in {path}")

            values = [token.strip() for token in lines[i + 1].split("\t")]

            declared_num_frames = _parse_optional_positive_int(
                _get_tab_value(tokens, values, "NumFrames"),
                "NumFrames",
                "TRC",
                path,
            )
            declared_num_markers = _parse_optional_positive_int(
                _get_tab_value(tokens, values, "NumMarkers"),
                "NumMarkers",
                "TRC",
                path,
            )
            units = _get_tab_value(tokens, values, "Units") or "unknown"
            break

    if header_idx + 1 >= len(lines):
        raise ParseError(f"TRC coordinate-label row is missing in {path}")

    coordinate_labels = [
        token.strip() for token in lines[header_idx + 1].split("\t") if token.strip()
    ]
    expected_coordinate_labels = len(marker_names) * 3

    if len(coordinate_labels) != expected_coordinate_labels:
        raise ParseError(
            f"TRC coordinate-label count mismatch in {path}: "
            f"found {len(coordinate_labels)}, expected {expected_coordinate_labels} "
            f"for {len(marker_names)} markers"
        )

    return _TrcHeader(
        marker_names=tuple(marker_names),
        declared_num_frames=declared_num_frames,
        declared_num_markers=declared_num_markers,
        units=units,
        header_idx=header_idx,
    )


def _parse_numeric_rows(
    *,
    lines: list[str],
    start_idx: int,
    expected_width: int,
    file_label: str,
    path: Path,
) -> np.ndarray:
    """Parse numeric rows with strict row-width validation."""
    rows: list[list[float]] = []

    for line_number, line in enumerate(lines[start_idx:], start=start_idx + 1):
        if not line.strip():
            continue

        parts = line.split()
        if len(parts) != expected_width:
            raise ParseError(
                f"{file_label} row has {len(parts)} columns but expected {expected_width} "
                f"in {path} at line {line_number}: {line}"
            )

        try:
            rows.append([float(part) for part in parts])
        except ValueError as exc:
            raise ParseError(
                f"Non-numeric {file_label} data row in {path} at line {line_number}: {line}"
            ) from exc

    if not rows:
        raise ParseError(f"No numeric {file_label} data rows found in {path}")

    arr = np.asarray(rows, dtype=float)
    _ensure_finite_array(arr, file_label, path)

    return arr


def _validate_trc_counts(
    header: _TrcHeader, actual_num_frames: int, path: Path
) -> None:
    """Validate parsed TRC row/marker counts against declared header values."""
    actual_num_markers = len(header.marker_names)

    if (
        header.declared_num_frames is not None
        and header.declared_num_frames != actual_num_frames
    ):
        raise ParseError(
            f"TRC NumFrames mismatch in {path}: "
            f"header says {header.declared_num_frames}, parsed {actual_num_frames}"
        )

    if (
        header.declared_num_markers is not None
        and header.declared_num_markers != actual_num_markers
    ):
        raise ParseError(
            f"TRC NumMarkers mismatch in {path}: "
            f"header says {header.declared_num_markers}, parsed {actual_num_markers}"
        )


def parse_trc(path: str | Path) -> PoseData:
    """Parse an OpenSim-style TRC file.

    This parser supports the common TRC layout:

    - metadata/header lines
    - a line beginning with Frame# and Time
    - a coordinate label line
    - numeric rows where the first two columns are frame and time
    - marker coordinates follow as XYZ triplets
    """

    path = Path(path)
    lines = _read_lines(path, "TRC")
    header = _parse_trc_header(lines, path)

    expected_width = 2 + len(header.marker_names) * 3
    arr = _parse_numeric_rows(
        lines=lines,
        start_idx=header.header_idx + 2,
        expected_width=expected_width,
        file_label="TRC",
        path=path,
    )

    frames = arr[:, 0]
    time = arr[:, 1]
    coord_values = arr[:, 2:]

    _ensure_strictly_increasing(frames, "frame column", "TRC", path)
    _ensure_strictly_increasing(time, "time column", "TRC", path)

    positions = coord_values.reshape(
        (coord_values.shape[0], len(header.marker_names), 3)
    )
    _validate_trc_counts(header, actual_num_frames=positions.shape[0], path=path)

    return PoseData(
        time=time,
        marker_names=list(header.marker_names),
        positions=positions,
        units=header.units,
        source_path=path,
    )


def _parse_header_key_value(line: str) -> tuple[str, str] | None:
    """Parse simple MOT header values like `datarows 5` or `nRows=5`."""
    stripped = line.strip()
    if not stripped:
        return None

    if "=" in stripped:
        key, value = stripped.split("=", maxsplit=1)
        return key.strip(), value.strip()

    parts = stripped.split(maxsplit=1)
    if len(parts) == 2:
        return parts[0].strip(), parts[1].strip()

    return None


def _parse_mot_declared_counts(lines: list[str]) -> tuple[int | None, int | None]:
    """Parse optional MOT row/column counts from the header."""
    declared_num_rows = None
    declared_num_columns = None

    for line in lines:
        if line.strip().lower() == "endheader":
            break

        parsed = _parse_header_key_value(line)
        if parsed is None:
            continue

        key, value = parsed
        normalized_key = key.lower()

        if normalized_key in {"datarows", "nrows"}:
            declared_num_rows = _parse_optional_positive_int(
                value, key, "MOT", Path("<header>")
            )
        elif normalized_key in {"datacolumns", "ncolumns"}:
            declared_num_columns = _parse_optional_positive_int(
                value, key, "MOT", Path("<header>")
            )

    return declared_num_rows, declared_num_columns


def _parse_mot_header(lines: list[str], path: Path) -> _MotHeader:
    """Find and validate the MOT column header."""
    declared_num_rows, declared_num_columns = _parse_mot_declared_counts(lines)

    header_idx = None
    for i, line in enumerate(lines):
        if line.strip().lower() == "endheader":
            header_idx = i + 1
            break

    if header_idx is None:
        for i, line in enumerate(lines):
            if line.strip().lower().startswith("time"):
                header_idx = i
                break

    if header_idx is None or header_idx >= len(lines):
        raise ParseError(f"Could not find MOT time header in {path}")

    columns = lines[header_idx].split()
    if not columns or columns[0].lower() != "time":
        raise ParseError(f"MOT first data column must be time in {path}")

    if len(columns) < 2:
        raise ParseError(
            f"MOT file must contain at least one data column after time in {path}"
        )

    _ensure_unique_nonempty_names(columns, "MOT column header", path)

    if declared_num_columns is not None and declared_num_columns != len(columns):
        raise ParseError(
            f"MOT datacolumns mismatch in {path}: "
            f"header says {declared_num_columns}, parsed {len(columns)}"
        )

    return _MotHeader(
        columns=tuple(columns),
        declared_num_rows=declared_num_rows,
        declared_num_columns=declared_num_columns,
        header_idx=header_idx,
    )


def _validate_mot_counts(header: _MotHeader, actual_num_rows: int, path: Path) -> None:
    """Validate parsed MOT row count against declared header value."""
    if (
        header.declared_num_rows is not None
        and header.declared_num_rows != actual_num_rows
    ):
        raise ParseError(
            f"MOT datarows mismatch in {path}: "
            f"header says {header.declared_num_rows}, parsed {actual_num_rows}"
        )


def parse_mot(path: str | Path) -> JointAngleData:
    """Parse an OpenSim-style MOT file.

    The parser looks for `endheader`, then expects a column-name row where the
    first column is `time`.
    """

    path = Path(path)
    lines = _read_lines(path, "MOT")
    header = _parse_mot_header(lines, path)

    arr = _parse_numeric_rows(
        lines=lines,
        start_idx=header.header_idx + 1,
        expected_width=len(header.columns),
        file_label="MOT",
        path=path,
    )

    time = arr[:, 0]
    _ensure_strictly_increasing(time, "time column", "MOT", path)
    _validate_mot_counts(header, actual_num_rows=arr.shape[0], path=path)

    return JointAngleData(
        time=time,
        column_names=list(header.columns[1:]),
        data=arr[:, 1:],
        source_path=path,
    )
