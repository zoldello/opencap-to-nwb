# OpenCap to NWB Converter

## Purpose

NWB is a structured data format used in neuroscience to organize, share, and reuse experimental data. It supports reproducible research workflows, FAIR data practices, and data sharing through platforms such as DANDI.

`opencap-to-nwb` is a Python command-line tool for converting OpenCap/OpenSim-style movement output files into NWB. It brings OpenCap-derived movement data closer to the neuroscience ecosystem by allowing movement outputs to be stored, inspected, analyzed, and shared alongside other experimental signals.

This is useful for researchers working with multimodal human-movement data, including EMG, neural recordings, behavioral events, task metadata, and clinical context. Instead of treating OpenCap outputs as standalone biomechanics files, this tool helps integrate them into reproducible scientific datasets and data analysis pipelines.

The goal is to make OpenCap movement data more useful for neuroscience, neuromuscular research, neurorehabilitation, motor-control, and brain-computer interface workflows. NWB has a rich and growing ecosystem of tools, and this project makes it more convenient for scientists and engineers to use OpenCap data within that ecosystem.

## Status

This project is currently a prototype. It is intended for evaluation and development use, not production scientific use.

The current version targets regular OpenCap/OpenSim-style output files. OpenCap Monocular support is planned, but has not yet been validated against real OpenCap Monocular output.

## Current Scope

* OpenCap/OpenSim-style `.trc` files for 3D marker/body-point positions
* OpenCap/OpenSim-style `.mot` files for joint-angle / coordinate time series
* YAML session metadata
* Real OpenCap-style folder layouts using `MarkerData/` and `OpenSimData/IK/`
* NWB writing with:

  * subject/session metadata
  * pose time series
  * joint-angle time series
  * source-file references
* Parser, integration, validation, and CLI tests

## Future Features

The list is sorted roughly by priority. Bold items are expected before a stronger public release.

* **EMG support**
* **OpenCap Monocular support**
* **Validation against real OpenCap Monocular output**
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
  --input test_dat/session_001 \
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

Or convert using explicit files:

```bash
opencap-to-nwb \
  --input test_dat/session_001 \
  --metadata test_dat/session_001/metadata.yaml \
  --trc test_dat/session_001/trial_001.trc \
  --mot test_dat/session_001/trial_001.mot \
  --output session_001.nwb
```

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

## Planned Next Steps

1. Test against additional real regular OpenCap output.
2. Improve metadata extraction from real OpenCap folders.
3. Add raw EMG storage as optional V2 input.
4. Add optional EMG sync offset metadata.
5. Test against real OpenCap Monocular output.
6. Add DANDI-oriented packaging or upload helpers.
7. Consider `ndx-pose` once the basic mapping is stable.


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

This checks that the package tests pass, lists the available trials in a real OpenCap-style folder, converts one trial to NWB, and opens the inspection notebook.

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

Convert the `Asym_0` trial:

```bash
opencap-to-nwb \
  --input test_data/subject0 \
  --trial Asym_0 \
  --output test_output/subject0_asym.nwb
```

## Current Limitations

This project is still a prototype. The current implementation is focused on OpenCap/OpenSim-style `.trc` marker trajectories, `.mot` joint-angle / coordinate time series, YAML metadata, and real OpenCap-style folder layouts.

Current limitations include:

* OpenCap Monocular output has not yet been validated.
* EMG import is not implemented yet.
* DANDI packaging helpers are not implemented yet.
* `ndx-pose` support is not implemented yet.
* Activity labels are inferred conservatively only when obvious from the trial name.
* Metadata fallback logic fills missing NWB-facing fields from folder and trial context, but it does not replace a formal OpenCap metadata schema.
