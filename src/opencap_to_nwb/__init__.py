"""Convert OpenCap/OpenSim-style outputs to NWB."""

from .convert import convert_session
from .parsers import parse_metadata, parse_mot, parse_trc

__all__ = [
    "convert_session",
    "parse_metadata",
    "parse_mot",
    "parse_trc",
]

__version__ = "0.1.0"
