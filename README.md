[![Tests](https://github.com/zoldello/opencap-to-nwb/actions/workflows/tests.yml/badge.svg)](https://github.com/zoldello/opencap-to-nwb/actions/workflows/tests.yml)
[![Python](https://img.shields.io/badge/python-3.11%2B-blue)](https://www.python.org/)
[![Code style: Black](https://img.shields.io/badge/code%20style-black-000000.svg)](https://github.com/psf/black)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# OpenCap to NWB Converter

## Purpose

NWB is a structured data format used in neuroscience to organize, share, and reuse experimental data. It supports reproducible research workflows, FAIR data practices, and data sharing through platforms such as DANDI.

`opencap-to-nwb` is a Python command-line tool for converting OpenCap/OpenSim-style movement output files into NWB. It brings OpenCap-derived movement data closer to the neuroscience ecosystem by allowing movement outputs to be stored, inspected, analyzed, and shared alongside other experimental signals.

This is useful for researchers working with multimodal human-movement data, including EMG, neural recordings, behavioral events, task metadata, and clinical context. Instead of treating OpenCap outputs as standalone biomechanics files, this tool helps integrate them into reproducible scientific datasets and data analysis pipelines.

The goal is to make OpenCap movement data more useful for neuroscience, neuromuscular research, neurorehabilitation, motor-control, and brain-computer interface workflows. NWB has a rich and growing ecosystem of tools, and this project makes it more convenient for scientists and engineers to use OpenCap data within that ecosystem.

## Status

This project is currently a prototype. It is intended for evaluation and development use, not production scientific use.

The current version targets regular OpenCap/OpenSim-style output files. OpenCap Monocular support is planned, but has not yet been validated against real OpenCap Monocular output.

The current version also includes optional raw EMG CSV import. EMG timestamps are preserved as written. The converter does not automatically synchronize, resample, or time-warp EMG data to match `.trc` or `.mot` data.

## Current Scope

* OpenCap/OpenSim-style `.trc` files for 3D marker/body-point positions
* OpenCap/OpenSim-style `.mot` files for joint-angle / coordinate time series
* Optional raw EMG CSV files
* YAML session metadata
* Real OpenCap-style folder layouts using `MarkerData/`, `OpenSimData/IK/`, and optional `EMGData/`
* NWB writing with:

  * subject/session metadata
  * pose time series
  * joint-angle time series
  * optional raw EMG time series
  * source-file references

* Parser, integration, validation, and CLI tests

## Expected Folder Layout

A real OpenCap-style subject folder can look like this:

```text
test_data/
  subject0/
    MarkerData/
      Squats_0.trc
      Asym_0.trc
    OpenSimData/
      IK/
        Squats_0.mot
        Asym_0.mot
    EMGData/
      Squats_0_synthetic_raw_emg.csv
      Asym_0_synthetic_raw_emg.csv
```

`EMGData/` is optional. Existing `.trc` / `.mot` conversion works without EMG.

## EMG Support

EMG support is currently implemented as an optional raw CSV input.

Expected EMG CSV format:

```csv
time,TA_R,TA_L,MG_R,MG_L,RF_R,RF_L
0.000,0.012,-0.003,0.004,0.010,0.005,-0.001
0.001,0.018,-0.004,0.006,0.013,0.007,-0.002
```

The first column must be a time column. The parser accepts:

* `time`
* `time_s`

All other numeric columns are treated as EMG signal channels, except common sync/trigger columns such as:

* `sync_pulse`
* `sync`
* `trigger`
* `ttl`

Raw EMG is stored in NWB as:

```python
nwbfile.acquisition["RawEMG"]
```

Important EMG behavior:

* EMG is optional.
* EMG is stored as raw acquisition data.
* EMG timestamps are preserved from the CSV file.
* EMG is not automatically synchronized to `.trc` or `.mot`.
* EMG is not resampled.
* EMG is not time-warped.
* EMG units currently default to millivolts (`mV`).

This design keeps EMG usable for multimodal NWB development while avoiding hard-coded assumptions about synchronization across labs or recording systems.

## Future Features

The list is sorted roughly by priority. Bold items are expected before a stronger public release.

* **Validation against additional real OpenCap output**
* **OpenCap Monocular support**
* **Validation against real OpenCap Monocular output**
* Validation against real EMG exports from external acquisition systems
* Optional EMG synchronization metadata
* DANDI-oriented packaging or upload helpers
* `ndx-pose` support for richer pose representation
* Support for additional multimodal signals where appropriate, such as EEG, ECoG, LFP, spikes, or behavioral events

## Why Regular OpenCap First?

This version targets generic OpenCap/OpenSim-style `.trc` and `.mot` files.

OpenCap Monocular also outputs `.trc`, `.mot`, `mono.json`, and `*_scaled.osim`, but this project has not yet been validated against real OpenCap Monocular output data.

Regular OpenCap/OpenSim-style files are a practical first target because they provide a real movement-data workflow for testing the parser, validation logic, NWB writing, and command-line interface before expanding the scope.

## Install Locally with Conda

Recommended:

```bash
conda env create -f environment.yml
conda activate opencap-to-nwb
```

The environment installs the package in editable mode with:

```yaml
pip:
  - -e .
```

So changes made under `src/opencap_to_nwb/` are picked up without reinstalling.

If you later change `environment.yml`, update the environment with:

```bash
conda env update -f environment.yml --prune
conda activate opencap-to-nwb
```

Alternative using `mamba`:

```bash
mamba env create -f environment.yml
conda activate opencap-to-nwb
```

## Run Tests

```bash
pytest
```

## Run the Converter

Convert a simple staged folder:

```bash
opencap-to-nwb \
  --input test_data/session_001 \
  --output session_001.nwb
```

List available trials in a real OpenCap-style subject folder:

```bash
opencap-to-nwb list-trials \
  --input path/to/subject0
```

Convert a specific trial from a real OpenCap-style subject folder:

```bash
opencap-to-nwb \
  --input path/to/subject0 \
  --trial Squats_0 \
  --output subject0_squats.nwb
```

Convert a specific trial with optional EMG:

```bash
opencap-to-nwb \
  --input test_data/subject0 \
  --trial Squats_0 \
  --emg test_data/subject0/EMGData/Squats_0_synthetic_raw_emg.csv \
  --output test_output/subject0_squats_with_emg.nwb
```

Or convert using explicit files:

```bash
opencap-to-nwb \
  --input test_data/session_001 \
  --metadata test_data/session_001/metadata.yaml \
  --trc test_data/session_001/trial_001.trc \
  --mot test_data/session_001/trial_001.mot \
  --emg test_data/session_001/EMGData/trial_001_emg.csv \
  --output session_001.nwb
```

The `--emg` argument is optional. Existing `.trc` / `.mot` conversion works without EMG.

## Example Metadata

```yaml
height_m: 1.76
mass_kg: 72.5
sex: "male"

subject_id: "sub-001"
session_id: "ses-001"
trial_id: "walk-001"
activity: "walking"
source_video: "walk_001.mov"
```

## Example Workflow

A typical local workflow is:

```bash
conda activate opencap-to-nwb
pytest
opencap-to-nwb list-trials --input test_data/subject0
opencap-to-nwb \
  --input test_data/subject0 \
  --trial Squats_0 \
  --output test_output/subject0_squats.nwb
jupyter lab notebooks/002-subject0.ipynb
```

A workflow with optional EMG is:

```bash
conda activate opencap-to-nwb
pytest
opencap-to-nwb \
  --input test_data/subject0 \
  --trial Squats_0 \
  --emg test_data/subject0/EMGData/Squats_0_synthetic_raw_emg.csv \
  --output test_output/subject0_squats_with_emg.nwb
opencap-to-nwb inspect test_output/subject0_squats_with_emg.nwb
```

## Inspecting NWB Output

After converting an OpenCap trial to NWB, inspect the generated file with the example notebook:

```bash
jupyter lab notebooks/002-subject0.ipynb
```

The notebook demonstrates how to:

* open the generated NWB file with PyNWB
* inspect subject and session metadata
* list the behavior processing module and data interfaces
* check pose and joint-angle array shapes
* inspect timestamps
* plot example pose and joint-angle time series

## Inspect an NWB File

After converting a trial, inspect the generated NWB file from the command line:

```bash
opencap-to-nwb inspect test_output/subject0_squats.nwb
```

For a file with EMG:

```bash
opencap-to-nwb inspect test_output/subject0_squats_with_emg.nwb
```

## Real OpenCap Example Commands

List trials in the included real OpenCap-style subject folder:

```bash
opencap-to-nwb list-trials --input test_data/subject0
```

Convert the `Squats_0` trial:

```bash
opencap-to-nwb \
  --input test_data/subject0 \
  --trial Squats_0 \
  --output test_output/subject0_squats.nwb
```

Convert the `Squats_0` trial with optional EMG:

```bash
opencap-to-nwb \
  --input test_data/subject0 \
  --trial Squats_0 \
  --emg test_data/subject0/EMGData/Squats_0_synthetic_raw_emg.csv \
  --output test_output/subject0_squats_with_emg.nwb
```

Convert the `Asym_0` trial:

```bash
opencap-to-nwb \
  --input test_data/subject0 \
  --trial Asym_0 \
  --output test_output/subject0_asym.nwb
```

Convert the `Asym_0` trial with optional EMG:

```bash
opencap-to-nwb \
  --input test_data/subject0 \
  --trial Asym_0 \
  --emg test_data/subject0/EMGData/Asym_0_synthetic_raw_emg.csv \
  --output test_output/subject0_asym_with_emg.nwb
```

## Planned Next Steps

1. Test against additional real regular OpenCap output.
2. Improve metadata extraction from real OpenCap folders.
3. Validate optional raw EMG input against real EMG export files.
4. Add optional EMG sync offset metadata.
5. Test against real OpenCap Monocular output.
6. Add DANDI-oriented packaging or upload helpers.
7. Consider `ndx-pose` once the basic mapping is stable.

## Current Limitations

This project is still a prototype. The current implementation is focused on OpenCap/OpenSim-style `.trc` marker trajectories, `.mot` joint-angle / coordinate time series, optional raw EMG CSV files, YAML metadata, and real OpenCap-style folder layouts.

Current limitations include:

* OpenCap Monocular output has not yet been validated.
* EMG import currently expects a simple CSV file with a time column and numeric signal channels.
* EMG timestamps are preserved as written; automatic synchronization is not performed.
* Real EMG exports from external acquisition systems have not yet been broadly validated.
* DANDI packaging helpers are not implemented yet.
* `ndx-pose` support is not implemented yet.
* Activity labels are inferred conservatively only when obvious from the trial name.
* Metadata fallback logic fills missing NWB-facing fields from folder and trial context, but it does not replace a formal OpenCap metadata schema.

## License

This project is licensed under the MIT License. See `LICENSE` for details.
