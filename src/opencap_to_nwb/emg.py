"""Parser for optional raw EMG CSV files."""

from __future__ import annotations

import csv
from pathlib import Path

import numpy as np

from .models import EMGData
from .parsers import ParseError

_SYNC_COLUMN_NAMES = {"sync", "sync_pulse", "trigger", "ttl"}
_TIME_COLUMN_NAMES = {"time", "time_s", "timestamp", "timestamp_s"}


def parse_emg_csv(path: str | Path, units: str = "mV") -> EMGData:
    """Parse a simple raw EMG CSV file.

    Expected format:
        time,channel_1,channel_2,...
        0.000,...
        0.001,...

    The first column may be named ``time`` or ``time_s``. Known sync/trigger
    columns are ignored so that ``RawEMG`` contains only EMG signal channels.
    Timestamps are preserved as written. No synchronization, resampling,
    or time warping is performed.
    """

    path = Path(path)

    try:
        with path.open(newline="") as f:
            reader = csv.reader(f)
            rows = list(reader)
    except OSError as exc:
        raise ParseError(f"Could not read EMG file {path}: {exc}") from exc

    if not rows:
        raise ParseError(f"Empty EMG file: {path}")

    header = [col.strip() for col in rows[0]]

    if len(header) < 2:
        raise ParseError(
            f"EMG file must contain time plus at least one channel: {path}"
        )

    time_column = header[0].lower()
    if time_column not in _TIME_COLUMN_NAMES:
        raise ParseError(
            f"EMG first column must be one of {sorted(_TIME_COLUMN_NAMES)}: {path}"
        )

    channel_indices = [
        index
        for index, name in enumerate(header[1:], start=1)
        if name.strip().lower() not in _SYNC_COLUMN_NAMES
    ]
    channel_names = [header[index] for index in channel_indices]

    if any(not name for name in channel_names):
        raise ParseError(f"EMG file contains empty channel names: {path}")

    if len(set(channel_names)) != len(channel_names):
        raise ParseError(f"EMG file contains duplicate channel names: {path}")

    if not channel_names:
        raise ParseError(f"EMG file has no EMG signal channels: {path}")

    numeric_rows: list[list[float]] = []

    for line_number, row in enumerate(rows[1:], start=2):
        if not row or not any(cell.strip() for cell in row):
            continue

        if len(row) != len(header):
            raise ParseError(
                f"EMG row has {len(row)} columns but expected {len(header)} "
                f"in {path} at line {line_number}"
            )

        try:
            numeric_rows.append([float(cell) for cell in row])
        except ValueError as exc:
            raise ParseError(
                f"Non-numeric EMG data row in {path} at line {line_number}: {row}"
            ) from exc

    if not numeric_rows:
        raise ParseError(f"No numeric EMG data rows found in {path}")

    arr = np.asarray(numeric_rows, dtype=float)

    if not np.all(np.isfinite(arr)):
        raise ParseError(f"EMG file contains NaN or infinite values: {path}")

    time = arr[:, 0]
    data = arr[:, channel_indices]

    if time.size >= 2 and np.any(np.diff(time) <= 0):
        raise ParseError(f"EMG time column must be strictly increasing: {path}")

    return EMGData(
        time=time,
        channel_names=channel_names,
        data=data,
        units=units,
        source_path=path,
    )
