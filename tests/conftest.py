from __future__ import annotations

from pathlib import Path


def example_session_dir() -> Path:
    return Path(__file__).resolve().parents[1] / "test_data" / "session_001"
