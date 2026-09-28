# milliQan Analysis

(Partially filled in -- simulation section only so far.)

## Repos

| Repo | Branch | Purpose |
| --- | --- | --- |
| `github.com/CollinKa/milliQanSim` | `MacArmGeant4P11` | Geant4 slab/bar simulation (macOS + Geant4 11 port) and the flattening script |

## Simulation (local macOS)

Skills: `milliqan-sim-env-setup` -> `milliqan-sim-slab-bench` -> `milliqan-sim-flatten-npe`
(see `.claude/skills/README.md`). Each pins the milliQanSim commit it was tested on.

## Per-machine settings

`milliqan/CLAUDE.local.md` (gitignored) holds this person's paths. Keys:

- `sim_repo` -- local milliQanSim clone
- `sim_conda_env` -- conda env the sim and Geant4 were built in
- `geant4_install` -- Geant4 11.2.2 install prefix built in that env

If it is missing, ask the user and create it (the env-setup skill does this).

## To be filled in

- Offline data / SPE waveform analysis on lxplus (repos, EOS paths, container)
- Reference clones (`ref/`), scratch dirs
