# Changelog

## v0.1.1

Maintenance update.

- Added Black formatting configuration.
- Added Black formatting check to GitHub Actions.
- Updated development environment dependencies.
- Cleaned package metadata.
- Confirmed test suite passes with 39 tests.

## v0.1.0

First working prototype.

- Added conversion from OpenCap/OpenSim-style `.trc` marker trajectories to NWB.
- Added conversion from OpenCap/OpenSim-style `.mot` joint-angle / coordinate time series to NWB.
- Added YAML metadata parsing.
- Added real OpenCap-style folder discovery using `MarkerData/` and `OpenSimData/IK/`.
- Added metadata fallback normalization for missing subject, session, trial, and activity fields.
- Added NWB validation.
- Added command-line interface.
- Added `inspect` command for generated NWB files.
- Added Jupyter notebook for inspecting generated output.
- Added real OpenCap subject0 fixture and integration test.
- Added GitHub Actions test workflow.
- Added MIT license.

