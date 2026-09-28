---
name: milliqan-sim-env-setup
description: One-time macOS (Apple Silicon) setup for the milliQanSim Geant4 simulation -- cloning CollinKa/milliQanSim on branch MacArmGeant4P11, creating the conda env (ROOT, Qt5, CMake<4, Boost) and building Geant4 11.2.2 with Qt against it. Use when someone wants to install or set up the MilliQan Geant4 sim locally, when `setup_mac_g4p11.sh` says there is no Geant4 install, when a conda env or Geant4 build for the sim is missing or broken, or before running milliqan-sim-slab-bench on a machine that has never built the sim.
---

# milliQanSim local environment (macOS)

**Code:** `CollinKa/milliQanSim` branch `MacArmGeant4P11` @ `9def340`
(written and tested against this commit on 2026-09-27; see "Pinned commit" below).

Tested end to end on 2026-09-27 on an arm64 Mac: fresh clone, fresh env, fresh
Geant4 build, then `benchSetupSlab.sh` ran 10 Cd-109 events. macOS arm64 only -- on
Linux/lxplus use the MilliQan container instead.

## Per-machine settings

Read `milliqan/CLAUDE.local.md` (gitignored) for these keys; ask the user for any that
are missing and write them back there, so the other `milliqan-sim-*` skills find them:

| Key | Default |
| --- | --- |
| `sim_repo` | `~/Desktop/CERN/milliQanSim_Geant4P11/milliQanSim` (ask; no universal location) |
| `sim_conda_env` | `milliqan-sim-g4p11` |
| `geant4_install` | `~/software/geant4-11.2.2-qt-install` |

## Check what already exists first

Don't rebuild what works. Check, in order, and skip steps that pass:

1. `conda env list` contains `sim_conda_env`.
2. `<geant4_install>/bin/geant4-config --version` prints `11.2.2` and
   `--has-feature qt` prints `yes`. If `<geant4_install>/BUILD_INFO.txt` exists, it
   names the env the install was built in -- it must match `sim_conda_env`.
3. `sim_repo` is a git clone of `CollinKa/milliQanSim` on `MacArmGeant4P11`.

## 1. Clone

```bash
git clone -b MacArmGeant4P11 https://github.com/CollinKa/milliQanSim.git <sim_repo>
```

## 2. Conda env

```bash
conda create -y -n milliqan-sim-g4p11 -c conda-forge python=3.11 "cmake<4" ninja git root boost qt-main qt
```

Takes a few minutes. Expected: ROOT 6.38.x, CMake 3.31.x, Qt 5.15. `cmake<4` is
required: Geant4 11.2 does not configure with CMake 4.

## 3. Geant4 11.2.2

Confirm with the user before starting: it downloads ~2 GB of datasets and compiles
for roughly 20-60 minutes. Run it in the background and check on it.

```bash
bash <sim_repo>/build_geant4_mac.sh <sim_conda_env> <geant4_install>
```

The script activates the env, downloads the source to `~/software/src` (not `/tmp`,
which macOS clears), builds with Qt + Qt3D, multithreading and datasets, and writes
`<geant4_install>/BUILD_INFO.txt` with the exact cmake line. Logs:
`~/software/src/geant4-v11.2.2-build/{configure,build,install}.log`.

Rules that matter:

- Build Geant4 **inside the activated env**. The env's Qt/expat/zlib paths are baked
  into the install, so an install only works with the env it was built in. Never
  reuse an install across envs, and never mix with the older Geant4 10.7 setup
  (`milliqan-sim` env, `geant4-10.7.4-qt-install`).
- `GEANT4_USE_OPENGL_X11=OFF` is deliberate: the XQuartz GLX viewer doesn't work on
  macOS; the Qt viewer (OGLSQt) is used instead.
- Changing any flag means a clean build directory; the script always starts clean.
- Any bash script that runs `conda activate` under `set -u` must turn `-u` off around
  it -- conda's compiler activation scripts reference unset variables and abort.

## 4. Activate (every new shell)

From `sim_repo`, **source** (don't execute) the helper:

```bash
MQ_CONDA_ENV=<sim_conda_env> MQ_GEANT4_INSTALL=<geant4_install> source ./setup_mac_g4p11.sh
```

The variables can be omitted when they equal the defaults. It prints
`Activated <env> with Geant4 11.2.2 from <install>` and sets `Geant4_DIR`.

## Pinned commit

Before following this skill, run `git -C <sim_repo> log -1 --format=%h`. If it isn't
`9def340` or a descendant (`git merge-base --is-ancestor 9def340 HEAD`), tell the user
the skill was written for `9def340` and check `build_geant4_mac.sh` and
`setup_mac_g4p11.sh` still exist and take the same arguments before continuing.
When this skill is updated for newer code, update the hash here.
