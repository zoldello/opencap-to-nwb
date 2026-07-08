"""Command-line interface for opencap-to-nwb."""

from __future__ import annotations

import argparse
from pathlib import Path

from .convert import ConversionError, convert_session
from .nwb_validation import NwbValidationError
from .parsers import ParseError


def build_parser() -> argparse.ArgumentParser:
    """Build the command-line argument parser."""
    parser = argparse.ArgumentParser(
        description="Convert OpenCap/OpenSim-style movement outputs to NWB."
    )

    parser.add_argument(
        "--input",
        required=True,
        type=Path,
        help="Input session directory containing metadata.yaml, one .trc file, and one .mot file.",
    )

    parser.add_argument(
        "--output",
        required=True,
        type=Path,
        help="Output NWB file path.",
    )

    parser.add_argument(
        "--no-validate-nwb",
        action="store_true",
        help="Skip NWB schema validation after writing.",
    )

    return parser


def main(argv: list[str] | None = None) -> int:
    """Run the command-line interface."""
    parser = build_parser()
    args = parser.parse_args(argv)

    try:
        output = convert_session(
            input_dir=args.input,
            output_path=args.output,
            validate_nwb=not args.no_validate_nwb,
        )
    except (ConversionError, ParseError, NwbValidationError) as exc:
        parser.exit(status=1, message=f"error: {exc}\n")

    print(f"Wrote NWB file: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
