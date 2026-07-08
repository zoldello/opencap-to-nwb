"""Input discovery helpers for OpenCap/OpenSim-style sessions."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


class DiscoveryError(ValueError):
    """Raised when input files cannot be discovered unambiguously."""


@dataclass(frozen=True, slots=True)
class TrialFiles:
    """Resolved source files for one OpenCap/OpenSim trial."""

    input_dir: Path
    metadata_path: Path
    trc_path: Path
    mot_path: Path
    trial_name: str


@dataclass(frozen=True, slots=True)
class DiscoveredTrial:
    """A trial discovered from an OpenCap-style folder layout."""

    name: str
    trc_path: Path
    mot_path: Path


def _require_existing_file(path: Path, label: str) -> Path:
    """Return path if it exists as a file, otherwise raise DiscoveryError."""
    if not path.exists():
        raise DiscoveryError(f"{label} file does not exist: {path}")
    if not path.is_file():
        raise DiscoveryError(f"{label} path is not a file: {path}")
    return path


def _default_metadata_path(input_dir: Path) -> Path:
    """Return the default metadata path for an input directory."""
    return input_dir / "metadata.yaml"


def _find_direct_files(input_dir: Path, pattern: str, label: str) -> list[Path]:
    """Find files directly inside input_dir without recursive search."""
    matches = sorted(path for path in input_dir.glob(pattern) if path.is_file())
    if not matches:
        raise DiscoveryError(
            f"No {label} file matching {pattern!r} found directly in {input_dir}"
        )
    return matches


def _select_direct_file(
    input_dir: Path, pattern: str, label: str, trial: str | None
) -> Path:
    """Select one direct file, optionally by trial stem."""
    matches = _find_direct_files(input_dir, pattern, label)

    if trial is not None:
        trial_matches = [path for path in matches if path.stem == trial]
        if not trial_matches:
            available = ", ".join(path.stem for path in matches)
            raise DiscoveryError(
                f"No {label} file for trial {trial!r} found directly in {input_dir}. "
                f"Available {label} stems: {available}"
            )
        return trial_matches[0]

    if len(matches) > 1:
        available = ", ".join(path.name for path in matches)
        raise DiscoveryError(
            f"Multiple {label} files found directly in {input_dir}: {available}. "
            "Pass a trial name or explicit file path."
        )

    return matches[0]


def discover_trials(input_dir: str | Path) -> list[DiscoveredTrial]:
    """Discover trial pairs in a real OpenCap-style subject/session folder.

    The V1 real OpenCap layout expects marker trajectories in:

        MarkerData/<trial>.trc

    and inverse-kinematics joint coordinates in:

        OpenSimData/IK/<trial>.mot

    Other .mot files under OpenSimData/Dynamics are intentionally ignored.
    """
    input_dir = Path(input_dir)
    marker_dir = input_dir / "MarkerData"
    ik_dir = input_dir / "OpenSimData" / "IK"

    if not marker_dir.is_dir() or not ik_dir.is_dir():
        return []

    trc_by_stem = {
        path.stem: path for path in sorted(marker_dir.glob("*.trc")) if path.is_file()
    }
    mot_by_stem = {
        path.stem: path for path in sorted(ik_dir.glob("*.mot")) if path.is_file()
    }

    trial_names = sorted(set(trc_by_stem).intersection(mot_by_stem))
    return [
        DiscoveredTrial(
            name=name, trc_path=trc_by_stem[name], mot_path=mot_by_stem[name]
        )
        for name in trial_names
    ]


def resolve_trial_files(
    input_dir: str | Path,
    *,
    trial_name: str | None = None,
    output_trial: str | None = None,
    metadata_path: str | Path | None = None,
    trc_path: str | Path | None = None,
    mot_path: str | Path | None = None,
) -> TrialFiles:
    """Resolve metadata, TRC, and MOT files for conversion.

    Supported modes:
    - explicit paths: --metadata, --trc, --mot
    - real OpenCap layout: MarkerData/<trial>.trc + OpenSimData/IK/<trial>.mot
    - simple flat folder: one .trc and one .mot directly inside input_dir

    Parameters
    ----------
    trial_name:
        Preferred name for the selected trial.
    output_trial:
        Backward-compatible alias for trial_name. New code should use trial_name.
    """
    if (
        trial_name is not None
        and output_trial is not None
        and trial_name != output_trial
    ):
        raise DiscoveryError(
            f"Conflicting trial names supplied: trial_name={trial_name!r}, "
            f"output_trial={output_trial!r}"
        )

    selected_trial_name = trial_name if trial_name is not None else output_trial

    input_dir = Path(input_dir)
    if not input_dir.exists():
        raise DiscoveryError(f"Input directory does not exist: {input_dir}")
    if not input_dir.is_dir():
        raise DiscoveryError(f"Input path is not a directory: {input_dir}")

    resolved_metadata = _require_existing_file(
        (
            Path(metadata_path)
            if metadata_path is not None
            else _default_metadata_path(input_dir)
        ),
        "Metadata",
    )

    if trc_path is not None or mot_path is not None:
        if trc_path is None or mot_path is None:
            raise DiscoveryError(
                "Explicit conversion requires both --trc and --mot paths."
            )

        resolved_trc = _require_existing_file(Path(trc_path), "TRC")
        resolved_mot = _require_existing_file(Path(mot_path), "MOT")
        trial_name = selected_trial_name or resolved_trc.stem
        return TrialFiles(
            input_dir=input_dir,
            metadata_path=resolved_metadata,
            trc_path=resolved_trc,
            mot_path=resolved_mot,
            trial_name=trial_name,
        )

    discovered_trials = discover_trials(input_dir)
    if discovered_trials:
        if selected_trial_name is None:
            if len(discovered_trials) == 1:
                selected = discovered_trials[0]
            else:
                available = ", ".join(trial.name for trial in discovered_trials)
                raise DiscoveryError(
                    "Multiple OpenCap trials found. Pass --trial. "
                    f"Available trials: {available}"
                )
        else:
            matching_trials = [
                trial
                for trial in discovered_trials
                if trial.name == selected_trial_name
            ]
            if not matching_trials:
                available = (
                    ", ".join(trial.name for trial in discovered_trials) or "none"
                )
                raise DiscoveryError(
                    f"No OpenCap trial named {selected_trial_name!r} found in {input_dir}. "
                    f"Available trials: {available}"
                )
            selected = matching_trials[0]

        return TrialFiles(
            input_dir=input_dir,
            metadata_path=resolved_metadata,
            trc_path=selected.trc_path,
            mot_path=selected.mot_path,
            trial_name=selected.name,
        )

    resolved_trc = _select_direct_file(input_dir, "*.trc", "TRC", selected_trial_name)
    resolved_mot = _select_direct_file(input_dir, "*.mot", "MOT", selected_trial_name)
    trial_name = selected_trial_name or resolved_trc.stem

    if resolved_trc.stem != resolved_mot.stem and selected_trial_name is None:
        raise DiscoveryError(
            f"Direct TRC/MOT stems do not match: {resolved_trc.name} vs {resolved_mot.name}. "
            "Pass --trial or explicit --trc/--mot paths if this pairing is intentional."
        )

    return TrialFiles(
        input_dir=input_dir,
        metadata_path=resolved_metadata,
        trc_path=resolved_trc,
        mot_path=resolved_mot,
        trial_name=trial_name,
    )
