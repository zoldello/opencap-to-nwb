# Synthetic raw EMG fixture for subject0

These files are simulated, not human-recorded EMG.

They were generated to be temporally aligned with the existing subject0 OpenCap/OpenSim IK trials:
- Squats_0_synthetic_raw_emg.csv: 1000 Hz, 0.000 s to 13.850 s
- Asym_0_synthetic_raw_emg.csv: 1000 Hz, 0.000 s to 14.966 s

Format:
- CSV
- time_s: seconds from trial start
- sync_pulse: synthetic TTL-like pulse, high for the first and last 200 ms
- EMG channels: zero-centered raw surface-EMG-like voltage in millivolts

Channels:
- emg_r_rectus_femoris_mV
- emg_l_rectus_femoris_mV
- emg_r_vastus_medialis_mV
- emg_l_vastus_medialis_mV
- emg_r_biceps_femoris_mV
- emg_l_biceps_femoris_mV
- emg_r_medial_gastrocnemius_mV
- emg_l_medial_gastrocnemius_mV
- emg_r_tibialis_anterior_mV
- emg_l_tibialis_anterior_mV

Generation notes:
- Sampling rate: 1000 Hz
- Units: mV
- Signals are raw-like, not rectified and not envelope-smoothed.
- Bursts are amplitude-modulated from the subject0 IK knee/hip/ankle trajectories.
- Added realistic nuisances: broadband EMG-like noise, mild 60 Hz line noise, low-frequency motion artifact, and rare transient spikes.

Do not describe this as real EMG support. It is a synthetic fixture for parser/writer development only.
