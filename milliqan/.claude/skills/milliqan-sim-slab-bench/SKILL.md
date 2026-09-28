---
name: milliqan-sim-slab-bench
description: Build and run the MilliQan slab bench-test Geant4 simulation locally on macOS -- the Bench executable from benchSetupSlab.sh in CollinKa/milliQanSim (MacArmGeant4P11), with a radioactive gamma source such as Cd-109 at the slab. Use when asked to make/run a slab bench test, a Cd-109 (or Na-22, Mn-54, Ba-133, Co-57, Am-241, Eu-152) source simulation, a bench sim on the slab detector, or to rebuild the Bench sim. Produces a Geant4 ROOT file for milliqan-sim-flatten-npe.
---

# Slab bench-test simulation (local)

**Code:** `CollinKa/milliQanSim` branch `MacArmGeant4P11` @ `9def340`
(tested 2026-09-27: build + 10 and 20 Cd-109 events). Needs the environment from
`milliqan-sim-env-setup`; per-machine paths (`sim_repo`, `sim_conda_env`,
`geant4_install`) are in `milliqan/CLAUDE.local.md` -- if that file or those keys are
missing, run `milliqan-sim-env-setup` first.

## 0. Pinned commit check

`git -C <sim_repo> log -1 --format=%h`. If HEAD isn't `9def340` or a descendant of
it, tell the user which commit the skill was tested on before building.

## 1. Ask what to run

Ask, offering the defaults, before building or running anything:

| Setting | Default | Where it lives in the macro |
| --- | --- | --- |
| Source | Cd-109 | `/gps/hist/point` lines under `#Cd109` |
| Number of events | 10 (smoke test); the reference sample used 1M | `/run/beamOn` |
| Source position | slab centre, `-1300 0 -1250 mm` | `/gps/position` |
| Run name | e.g. `Cd109_center_1M` | `/run/fname` (added by this skill) |

The template macro is `runMac/sidegammaBench_no_vis.mac`: isotropic (`/gps/ang/type
iso`) gammas with a user-defined energy histogram. Cd-109 is the active spectrum
(3.08, 22.1, 25.0, 88.0 keV lines); Na-22, Mn-54, Ba-133, Co-57, Am-241 and Eu-152 are
commented alternatives in the same file. For a non-default source, swap which block is
commented and show the user the resulting lines before running.

Rough cost on an M-series Mac: ~0.4 s/event including startup for small runs; time a
1k-event run before promising how long 1M will take, and run large jobs in the
background.

## 2. Build

From `sim_repo`, in one shell:

```bash
source ./setup_mac_g4p11.sh        # with MQ_CONDA_ENV / MQ_GEANT4_INSTALL if non-default
bash benchSetupSlab.sh
```

`benchSetupSlab.sh` copies the `Bench` variant sources over the top-level files,
configures and builds `build/Bench`, then runs the default 10-event Cd-109 macro as a
smoke test (~20 s). Success: exit 0 and a new `build/Sim_<number>MilliQan.root`.

- Skip the rebuild if `build/Bench` exists and the last build was also the Bench
  variant (`cmp -s CMakeLists.txt CMakeLists.txt.Bench`). Another setup script
  (`beamSetupSlab.sh`, `cosmicSetupSlab.sh`, ...) run since then means rebuild.
- **Expected `git status` noise after building -- never commit these:**
  `CMakeLists.txt`, `include/mq{MuonTrack,ROOTEvent,TrackingAction,UserEventInformation}.hh`,
  `src/mq{DetectorConstruction,EventAction,MuonTrack,PMTSD,ScintSD,SteppingAction,TrackingAction,UserEventInformation}.cc`.
  The repo holds many sim variants and each setup script overwrites these. Real
  source changes go in the variant files (`CMakeLists.txt.Bench`, `src/Bench/`,
  `include/Bench/`).

## 3. Run

Write a run-specific macro into `build/` (gitignored), so the tracked template stays
untouched, then run it:

```bash
cd build
sed -e 's|^/run/beamOn .*|/run/fname <run_name>_\n/run/beamOn <N>|' \
    ../runMac/sidegammaBench_no_vis.mac > run_<run_name>.mac
# edit /gps/position or the source block in run_<run_name>.mac if requested
./Bench run_<run_name>.mac > run_<run_name>.log 2>&1
```

Output: `build/<run_name>_MilliQan.root` (tree `Events`). Always set `/run/fname`:
without it the name comes from an uninitialised `sprintf("Sim_%d")` in
`src/mqRunAction.cc`, giving an arbitrary number that differs between builds.

Report the output path and event count, then offer `milliqan-sim-flatten-npe`.

## Failures and harmless warnings

| Symptom | Cause / fix |
| --- | --- |
| `ConfigFileReadError ... config/onepc.ini: cannot open file` | `build/` exists without `build/config/` (`buildsetup.sh` only copies config into a brand-new `build/`). Delete `build/` and rerun. |
| `Undefined symbols ... TObject::...` linking `libBenchCore` | Code older than `55027d5` (no ROOT link on macOS). Update the clone. |
| `cp: -r: No such file or directory` | Code older than `2dd6449`. Update the clone. |
| CMake picks Geant4 11.3 or 10.7 | `setup_mac_g4p11.sh` not sourced in this shell; source it, delete `build/CMakeCache.txt`, rebuild. |
| Run sits for >10 min on a 10-event job | Seen once (2026-09-24) inside Geant4 energy-loss code, not reproduced. Ctrl-C and rerun. |

Harmless: `Run0111` duplicated-process warnings (eIoni etc.), one `GeomVol1002`
overlap warning, `mutex lock failure ... Non-critical` at exit, ROOT "already
associated" rootmap warnings from stale `libBenchDict.*` in `build/`.
