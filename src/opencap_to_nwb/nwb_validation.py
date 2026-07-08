"""Validation helpers for generated NWB files."""

from __future__ import annotations

import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Any


class NwbValidationError(ValueError):
    """Raised when an NWB file fails schema validation."""


@dataclass(frozen=True, slots=True)
class NwbValidationResult:
    """Result from validating an NWB file."""

    path: Path
    errors: tuple[str, ...]

    @property
    def is_valid(self) -> bool:
        """Return True when the NWB file has no validation errors."""
        return len(self.errors) == 0


def _check_nwb_path(path: Path) -> None:
    """Validate basic file-path requirements before NWB schema validation."""
    if not path.exists():
        raise NwbValidationError(f"NWB file does not exist: {path}")

    if not path.is_file():
        raise NwbValidationError(f"NWB path is not a file: {path}")

    if path.suffix.lower() != ".nwb":
        raise NwbValidationError(f"NWB file should have .nwb extension: {path}")


def _format_validation_errors(errors: Any) -> tuple[str, ...]:
    """Normalize PyNWB validation output into a tuple of strings."""
    if errors is None:
        return ()

    return tuple(str(error) for error in errors)


def _validate_with_pynwb_api(path: Path, *, verbose: bool = False) -> tuple[str, ...]:
    """Validate using the PyNWB Python API.

    PyNWB validation signatures have changed across versions. This function
    first tries the newer path-based call, then falls back to the older IO-based
    call used by PyNWB versions such as 2.8.x.
    """
    try:
        from pynwb import NWBHDF5IO, validate
    except ImportError as exc:
        raise NwbValidationError("Could not import PyNWB validation tools.") from exc

    try:
        errors = validate(path=str(path), verbose=verbose)
        return _format_validation_errors(errors)
    except TypeError:
        pass

    try:
        with NWBHDF5IO(str(path), "r") as io:
            try:
                errors = validate(io=io, verbose=verbose)
            except TypeError:
                errors = validate(io=io)

        return _format_validation_errors(errors)
    except Exception as exc:
        raise NwbValidationError(f"PyNWB validation failed for {path}: {exc}") from exc


def _validate_with_pynwb_cli(path: Path) -> tuple[str, ...]:
    """Validate using the pynwb-validate command-line tool."""
    executable = shutil.which("pynwb-validate")
    if executable is None:
        raise NwbValidationError(
            "Could not find `pynwb-validate` on PATH. "
            "Install or update PyNWB, or use an environment that provides the validator."
        )

    completed = subprocess.run(
        [executable, str(path)],
        check=False,
        capture_output=True,
        text=True,
    )

    output = "\n".join(
        part.strip() for part in (completed.stdout, completed.stderr) if part.strip()
    )

    if completed.returncode == 0:
        return ()

    return (output or f"`pynwb-validate` failed with exit code {completed.returncode}",)


def validate_nwb_file(
    path: str | Path,
    *,
    verbose: bool = False,
    prefer_cli: bool = False,
) -> NwbValidationResult:
    """Validate an NWB file against the NWB schema.

    This checks whether the file is structurally valid NWB. It does not prove
    that the scientific choices are ideal.
    """
    path = Path(path)
    _check_nwb_path(path)

    if prefer_cli:
        errors = _validate_with_pynwb_cli(path)
    else:
        try:
            errors = _validate_with_pynwb_api(path, verbose=verbose)
        except NwbValidationError:
            errors = _validate_with_pynwb_cli(path)

    return NwbValidationResult(path=path, errors=errors)


def assert_valid_nwb_file(
    path: str | Path,
    *,
    verbose: bool = False,
    prefer_cli: bool = False,
) -> None:
    """Raise NwbValidationError if an NWB file fails validation."""
    result = validate_nwb_file(path, verbose=verbose, prefer_cli=prefer_cli)

    if result.is_valid:
        return

    formatted_errors = "\n".join(f"- {error}" for error in result.errors)
    raise NwbValidationError(
        f"NWB validation failed for {result.path} with "
        f"{len(result.errors)} error(s):\n{formatted_errors}"
    )
