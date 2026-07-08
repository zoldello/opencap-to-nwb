# Opencap to NWB Converter

## Purpose

NWB is a structured data format used in neuroscience to organize, share, and reuse experimental data. It supports reproducible research workflows, FAIR data practices, and data sharing through platforms such as DANDI.

`opencap-to-nwb` is a Python command-line tool for converting OpenCap/OpenSim-style movement output files into NWB. It brings OpenCap-derived movement data closer to the neuroscience ecosystem by allowing movement outputs to be stored, inspected, analyzed, and shared alongside other experimental signals.

This is useful for researchers working with multimodal human-movement data, including EMG, neural recordings, behavioral events, task metadata, and clinical context. Instead of treating OpenCap outputs as standalone biomechanics files, this tool helps integrate them into reproducible scientific datasets and data analysis pipelines.

The goal is to make OpenCap movement data more useful for neuroscience, neuromuscular research, neurorehabilitation, motor-control, and brain-computer interface workflows. NWB has a rich and growing ecosystem of tools, and this project makes it more convenient for scientists and engineers to use OpenCap data within that ecosystem.


## Status

**This is still a prototype, not yet ready for prime-time use. Use only for evaluation for now**

This was made for Opencap. An extension for opencap monocular will be made prior to release for prime-time use.




## Current scope

- OpenCap/OpenSim-style `.trc` files for 3D marker/body-point positions
- OpenCap/OpenSim-style `.mot` files for joint-angle / coordinate time series
- YAML metadata
- NWB writing with:
  - subject/session metadata
  - pose time series
  - joint-angle time series
  - source-file references in descriptions/comments
- parser, integration, and CLI tests

## Future features 

The list is sorted by priority. Bold items are must-have features needed for a final release

- **EMG**
- **Opencap Monocular support**
- DANDI upload
- automatic synchronization
- `ndx-pose`
- full OpenCap Monocular validation
- EEG/ECoG/LFP/spikes


## Why regular OpenCap first?

The version targets generic OpenCap/OpenSim-style `.trc` and `.mot` files.
OpenCap Monocular also outputs `.trc`, `.mot`, `mono.json`, and `*_scaled.osim`,
but this scaffold has not yet been validated against real Monocular output data.

## Install locally with Conda

### Recommended:
```bash
conda env create -f environment.yml
conda activate opencap-to-nwb
```

The environment installs the package in editable mode with:

```yaml
pip:
  - -e .
```

So changes you make to files under `src/opencap_to_nwb/` are picked up without reinstalling.

If you later change `environment.yml`, update the environment with:

```bash
conda env update -f environment.yml --prune
conda activate opencap-to-nwb
```

### Alternative using `mamba`:

```bash
mamba env create -f environment.yml
conda activate opencap-to-nwb
```

## Run tests

```bash
pytest
```

## Run the converter

```bash
opencap-to-nwb \
  --input example_data/session_001 \
  --output session_001.nwb
```

Or with explicit files:

```bash
opencap-to-nwb \
  --input example_data/session_001 \
  --metadata example_data/session_001/metadata.yaml \
  --trc example_data/session_001/trial_001.trc \
  --mot example_data/session_001/trial_001.mot \
  --output session_001.nwb
```

## Example metadata

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

## Planned next steps

Recommended roadmap:

1. Test against real regular OpenCap sample output.
2. Add raw EMG storage as optional V2 input.
3. Add optional EMG sync offset metadata.
4. Test against real OpenCap Monocular output.
5. Add DANDI/NWB validation helpers.
6. Consider `ndx-pose` once the basic mapping is stable.
