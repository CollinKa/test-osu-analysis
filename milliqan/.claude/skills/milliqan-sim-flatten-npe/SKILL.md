---
name: milliqan-sim-flatten-npe
description: Flatten a MilliQan slab Geant4 output file into the lightweight data-like tree with flatlightwithphotonslab_latest.py (CollinKa/milliQanSim, MacArmGeant4P11), applying a QE rescale, and make the PMT nPE distribution. Use when asked to flatten sim output, get the nPE distribution or nPE spectrum from a slab/bench/Cd-109 simulation, apply a QE rescale or quantum-efficiency scaling to sim photons, or compare simulated nPE to data. Always asks for the input file and the QE value.
---

# Flatten slab sim output and collect nPE

**Code:** `CollinKa/milliQanSim` branch `MacArmGeant4P11` @ `9def340`
(`--input/--output/--qe/--seed` were added in `ded9dec`; tested 2026-09-27 on 10- and
20-event Cd-109 files). Per-machine paths come from `milliqan/CLAUDE.local.md`
(`sim_repo`, `sim_conda_env`, `geant4_install`); see `milliqan-sim-env-setup`.

## 0. Pinned commit check

`git -C <sim_repo> log -1 --format=%h` must be `9def340` or a descendant. If it's
older than `ded9dec`, the script has hardcoded paths and no `--qe` -- tell the user to
update the clone rather than editing the script.

## 1. Always ask -- every time, even if a value was used before

1. **Input file.** List candidates newest first and let the user pick:
   `ls -t <sim_repo>/build/*MilliQan.root | grep -v _flat`
   (names are `<run_name>_MilliQan.root`, or `Sim_<number>MilliQan.root` from the
   setup script's smoke test).
2. **QE rescale.** "Use QE = 1.0 (keep every photon hit), or another value?" Must be
   in (0, 1]; the script rejects anything else.

Also offer, but don't insist on: an output name (default below) and `--seed` for a
reproducible thinning (default unseeded).

## 2. Flatten

Use the env the sim was built with: the script loads `build/libBenchCore.dylib`, and
a different ROOT fails with "no dictionary for class mqROOTEvent".

```bash
cd <sim_repo>
source ./setup_mac_g4p11.sh       # with MQ_CONDA_ENV / MQ_GEANT4_INSTALL if non-default
python README/flatlightwithphotonslab_latest.py --input <file> --qe <qe> [--seed <n>] [--output <out>]
```

Default output: `<input stem>_QE<qe>_flat.root` next to the input, with `.` written
as `p` (`_QE1_flat`, `_QE0p8_flat`). It prints `Wrote <N> events to <out> (QE = <qe>)`.
Run it in the background for large inputs; it loops in Python.

What `--qe` does: each PMT photon hit is kept with probability `qe`
(`populate_vectors_pmt`, the PMT `simToDataScale`). It does **not** touch the
separate scintillator `simToDataScale = 0.682*0.5` / `nPEPerMeV` EDep conversion in
`populate_vectors_scint`.

Output tree `t`: `eventID`, `runNumber`, `scint_*`, and per-PMT vectors `pmt_nPE`,
`pmt_chan` (sim PMT copy number), `pmt_time`, `pmt_layer`, `pmt_row`, `pmt_column`,
`pmt_type`. PMTs with 0 PE are not stored.

## 3. nPE distribution

```bash
python <this skill dir>/plot_npe.py <flat file> [--max-npe 50]
```

Writes `<flat stem>_npe.png` (nPE per PMT hit, total nPE per event; log y) and
`<flat stem>_npe.root` (those two plus nPE vs `pmt_chan`), and prints the event count,
mean nPE per PMT, mean total per event, events with 0 PE, and an overflow warning if
`--max-npe` is too low. Show the PNG to the user and state the QE used alongside
every number -- nPE results are meaningless without it.

Loop over flat trees with `TTree::Draw` (as `plot_npe.py` does) or uproot; iterating
`for e in tree` in PyROOT over these vector branches has segfaulted.
