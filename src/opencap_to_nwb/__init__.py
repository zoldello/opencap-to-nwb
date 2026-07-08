"""Convert OpenCap/OpenSim-style outputs to NWB."""

from __future__ import annotations

__version__ = "0.1.0"

__all__ = [
    "convert_session",
    "discover_trials",
    "resolve_trial_files",
    "parse_metadata",
    "parse_mot",
    "parse_trc",
]


def __getattr__(name: str):
    """Lazily expose public helpers without importing PyNWB on parser-only use."""
    if name == "convert_session":
        from .convert import convert_session

        return convert_session

    if name in {"discover_trials", "resolve_trial_files"}:
        from .discovery import discover_trials, resolve_trial_files

        return {"discover_trials": discover_trials, "resolve_trial_files": resolve_trial_files}[name]

    if name in {"parse_metadata", "parse_mot", "parse_trc"}:
        from .parsers import parse_metadata, parse_mot, parse_trc

        return {"parse_metadata": parse_metadata, "parse_mot": parse_mot, "parse_trc": parse_trc}[name]

    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
