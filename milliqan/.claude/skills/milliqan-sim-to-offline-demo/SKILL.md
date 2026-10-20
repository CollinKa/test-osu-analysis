---
name: milliqan-sim-to-offline-demo
description: Turn a raw MilliQan slab Geant4 simulation file into an offline (Ntuple) file on the OSU T3 or UCSB cluster, in three stages -- pulse injection with waveinject_slab.C (CollinKa/milliQanSim, SlabPulseInjectLogNormalSPE), with or without zero-bias waveforms; conversion to MilliDAQ digitizer format; and runOfflineFactory.py --slabsim from milliqanOffline. Use when asked to make offline files / ntuples / offline trees from slab sim output, inject pulses or zero bias into a sim file, run waveinject_slab, slabGlobalEventDemo or runOfflineFactory on sim, or turn Geant4 sim into data-like files. Not for lxplus. Always asks for the input file and the zero-bias file.
---

# Slab sim -> offline file (demo, OSU T3 / UCSB)

**Code tested** (2026-10-19, OSU T3; update when the skill is revised):

| Repo | Branch | Commit |
| --- | --- | --- |
| `github.com/CollinKa/milliQanSim` | `SlabPulseInjectLogNormalSPE` | `edc9935` |
| `gitlab.cern.ch/MilliQan/milliqanOffline` | `SlabPulseInjectionSim` | `f405f4b` |
| `gitlab.cern.ch/MilliQan/MilliDAQ` | `SlabPulseInjectionSim` | `543ae94` (`nDigitizers 7`) |
| container `milliqan_offline:x86` | -- | ROOT 6.24/08 |

**Status:** all three stages were run on the OSU T3 (login node cms-in0, inside the
container) on a 1k-event beam-muon file, **without zero bias**:
- stage 1 output matched the Mac output of the same commit: 997/1000 events
  bit-identical, the other 3 differ by 2e-10 mV in one sample;
- stage 2 output = -(stage 1) in every sample, channels 80-95 filled, digitizer 6
  zero, `numChan` 96;
- stage 3 tree `t` had 1000 entries, pulses in all 96 channels, and a mean
  `sidebandRMS` of 0.96 mV (1 mV Gaussian noise).

Zero bias has been tested only on the Mac (stage 1, synthetic file); no real
zero-bias file yet.

**Where:** OSU T3 or UCSB only. `Run3Detector/setup.sh` recognizes only those two
hostnames (and the `compile.sh` build depends on that setup), so it does not work on
lxplus. A move to a newer cluster is planned; when it happens, add it here.

Per-machine paths come from `milliqan/CLAUDE.local.md`: `cluster` (`osu-t3` or
`ucsb`), `cluster_host`, `cluster_sim_repo`, `cluster_offline_repo`,
`cluster_millidaq_repo`, `cluster_work_dir`, `cluster_container`, `cluster_bind`,
`cluster_offline_exe`. If a key is missing, ask and add it.

## Container (OSU T3)

All three stages run inside the milliqanOffline image; stages 2-3 must. On the OSU T3,
ROOT is not on the host PATH at all, so stage 1 needs the image too. The image may change
on the new cluster; it is the `cluster_container` key:

```
/cvmfs/unpacked.cern.ch/registry.hub.docker.com/carriganm95/milliqan_offline:x86
```

Interactively czheng uses `singularity shell -B <binds> <image>`. From a script, use
`singularity exec`. The container sees only bound host paths, so bind every directory the
run touches: the repos and work dir, and the input file's location. `cluster_bind` is a
comma-separated list (OSU T3: `/share/scratch0,/data`; `/data/users/<user>` holds sim
inputs). Before each run, check that the input file's directory is under one of the binds.
If it isn't, ask the user and add it. Never hardcode binds in the commands:

```bash
singularity exec -B <cluster_bind> <cluster_container> bash -c 'cd ... && ...'
```

Inside the image, ROOT 6.24/08 is `/usr/bin/root`, with PyROOT and pandas.

## Running commands on the OSU T3

- Needs the OSU VPN. Probe first: `ssh -o BatchMode=yes -o ConnectTimeout=15 <cluster_host> true`;
  on timeout, ask the user to connect the VPN. Don't retry in a loop.
- The login shell is tcsh, so send every command as a bash script:
  `ssh <cluster_host> 'bash -s' <<'EOF' ... EOF`.
- Work and write output only under `~/scratch0` (`/share/scratch0/<user>`), not `/home`.
- `interactive-0-2` (cms-in2) has no external network: an `xrdcp` from EOS fails
  there (stage 3 may need one), and uncached `/cvmfs` files fail to read.
- UCSB: access and paths are not written down yet (ask Dariush). `setup.sh` on UCSB
  uses `MILLIDAQDIR=/homes/milliqan/MilliDAQ` -- check that it is built with
  `nDigitizers 7` before stage 2.

## Setting up a fresh area (once)

Don't reuse old clones -- make a new directory and clone all three repos side by side
(MilliDAQ must sit next to milliqanOffline, see stage 3). The 2026-10-19 test area is
`/share/scratch0/czheng/CMSSW_12_4_11_patch3/src/CosmicSim/G4ToNtupleSkillTest`.

1. **milliQanSim** -- clone on the cluster:
   `git clone -b SlabPulseInjectLogNormalSPE https://github.com/CollinKa/milliQanSim.git`
2. **milliqanOffline and MilliDAQ** (CERN GitLab, branch `SlabPulseInjectionSim`):
   - `ssh://git@gitlab.cern.ch:7999/MilliQan/<repo>.git` on the T3 uses the T3's SSH key
     (`~/.ssh/id_ed25519`): it asks for **that key's passphrase**, not the CERN password,
     and the key must be registered in the user's GitLab account. Run it in a terminal
     tab the user can type into, never in a non-interactive shell.
   - What worked on 2026-10-19: clone on the user's Mac over HTTPS (GitLab login already
     cached there), then copy to the cluster. Set `COPYFILE_DISABLE=1`, or macOS tar adds
     `._*` files that show up as thousands of untracked files:
     ```bash
     COPYFILE_DISABLE=1 tar czf - milliqanOffline MilliDAQ | ssh <cluster_host> 'cd <test dir> && tar xzf -'
     ```
     If `._*` files did get in: `find milliqanOffline MilliDAQ -name '._*' -delete`.
3. **Build inside the container** (about 1 min in total):
   ```bash
   cd <test dir>/MilliDAQ && make shared          # only libMilliDAQ.so is needed
   cd <test dir>/milliqanOffline/Run3Detector && source setup.sh && ./compile.sh <exe>.exe
   ```
   Use `make shared`, not `make` / `sudo make install` / `systemctl start MilliDAQ.service`:
   those are the README's steps for running the digitizer DAQ (CAEN libraries, root
   access). `setup.sh` needs no edit when MilliDAQ sits next to milliqanOffline: on OSU it
   sets `MILLIDAQDIR=../../MilliDAQ` relative to `Run3Detector`.
4. **Record the commits** in the table at the top: `cd <repo> && git log -1 --format=%h`.

## 0. Ask -- every run, even if values were used before

1. **Input Geant4 file** (tree `Events`, branch `ROOTEvent`): its path on the
   cluster. If it is local, offer to copy it to `<cluster_work_dir>` with `scp`
   (335 MB for 1k beam-muon events).
2. **Zero bias**: "Path to the zero-bias file, or none?" None means 1 mV Gaussian
   noise on every channel. A zero-bias file needs:
   - tree `ZBwaveform` with branch `waveform` of type `Float_t[6][16][1024]`
     (other names: pass them as arguments 6-7 below);
   - index = summing-amp channel `digitizer*16 + channel`, raw digitizer output
     (no baseline subtraction, data polarity -- don't flip it when making the file);
   - sign: stage 1 writes sim pulses positive and *subtracts* the zero bias; stage 2
     flips every sample, so the digitizer file holds (sim x -1) + raw zero bias;
   - pairing: sim event *i* gets zero-bias entry *i % N*, where N is the number of
     zero-bias events. With N >= sim events it is 1-to-1; otherwise entries are
     reused and the macro prints a warning -- tell the user how many times.
3. **Run name** for output files (default: input base name). Outputs go in
   `<cluster_work_dir>`: `<run>_vector.root`, `<run>_digitizer.root`, `<run>_offline.root`.

## 1. Check the code

On the cluster, for each repo, `git -C <repo> rev-parse --abbrev-ref HEAD` and
`git log -1 --format=%h`. The branch must match the table above, and HEAD must be the
pinned commit or a descendant (`git merge-base --is-ancestor <pin> HEAD`). If not,
tell the user and stop. Also check:

- `grep nDigitizers <cluster_millidaq_repo>/interface/DemonstratorConstants.h` says 7
  and `<cluster_millidaq_repo>/libMilliDAQ.so` exists (build: see Setting up;
  after changing branch, `make clean && make shared`).
- `<cluster_sim_repo>/include/libMilliQanCore.so` exists. It is tracked in milliQanSim
  (x86-64 Linux, holds the `mqROOTEvent` dictionary), so there is nothing to build.

## 2. Stage 1 -- Geant4 -> vectorized waveforms (`waveinject_slab.C`)

Run the `root` command below inside the container (see Container).

The macro loads `../build/libMilliQanCore.dylib` (macOS) and includes
`../include/...`, both relative paths. Make a run copy one level below the repo root
so those paths still resolve, loading the tracked Linux library
`../include/libMilliQanCore.so` instead; the tracked macro stays untouched:

```bash
cd <cluster_sim_repo>
mkdir -p runInject
sed 's|\.\./build/libMilliQanCore\.dylib|../include/libMilliQanCore.so|' inputData/waveinject_slab.C > runInject/waveinject_slab.C
grep -n R__LOAD_LIBRARY runInject/waveinject_slab.C   # the active line must name ../include/libMilliQanCore.so
cd runInject
root -l -b -q 'waveinject_slab.C("<input>", "<work>/<run>_vector.root", "../inputData/SPElognormal.txt", "../inputData/SPEmeans.txt", "<zb file or empty>")' > <work>/<run>_stage1.log 2>&1
```

Pass `""` as the 5th argument for no zero bias. `runInject/` is untracked scratch;
don't commit it. Interpreted ROOT only -- `waveinject_slab.C+` (ACLiC) fails to link
against the sim library.

**Timing:** 1k beam-muon events took ~12 min on the T3 login node cms-in0 (old CPU),
versus ~60 s on a Mac. Run it in the background and check progress with
`grep "Processing Event" <run>_stage1.log | tail -1`. For much larger files, ask the
user about a Condor job; this skill doesn't cover that yet.

**Harmless on the T3** (output verified against the Mac):
- `Error in <TCling::LoadPCM>: ROOT PCM .../include/libMilliQanDict_rdict.pcm file does not exist`
- `fatal error: '/net/cms18/.../include/mqGammaTrack.hh' file not found` and
  `Error in <TInterpreter::AutoParse>: Error parsing payload code for class mqROOTEvent`
  -- the tracked library was built at UCSB; the macro includes the headers itself;
- tens of `Error in <TList::Clear>` / `<THashList::Delete>: A list is accessing an
  object ... already deleted`.

**Check** before stage 2:
- the log ends with `final`;
- with zero bias, the log has `Loaded <N> zero bias events; Gaussian noise off`, plus the
  reuse warning if N < sim events;
- `Events` in the output has as many entries as the input, and `waveform` has length
  98304 (`t->GetLeaf("waveform")->GetLen()`).

## 3. Stage 2 -- vectorized -> digitizer format (`slabGlobalEventDemo.C`)

Use the copy shipped with this skill. The upstream macro
(`Run3Detector/analysis/simConversion/utils/slabGlobalEventDemo.C`) is fixed as of
`f405f4b` and gave identical output in the test, but it still hardcodes
`/share/scratch0/czheng/.../CosmicSim/MilliDAQ`, so in any other area it would load a
different MilliDAQ. The skill's copy takes the MilliDAQ path from `@MILLIDAQ_DIR@`,
checks the branch shape, and keeps the sign flip (sim pulses positive -> slab data
negative).

Copy it to the cluster with the MilliDAQ path filled in, then run inside the container:

```bash
sed 's|@MILLIDAQ_DIR@|<cluster_millidaq_repo>|g' slabGlobalEventDemo.C | ssh <cluster_host> 'cat > <work>/slabGlobalEventDemo.C'
# on the cluster, inside the container (libMilliDAQ.so must be built with the image's ROOT):
cd <work>
root -l -b -q 'slabGlobalEventDemo.C("<run>_vector.root", "<run>_digitizer.root")' > <run>_stage2.log 2>&1
```

`<cluster_millidaq_repo>` must be an absolute path. ~1 min for 1k events on cms-in0;
a few `TList::Clear` errors are harmless. **Check:** log line `Converted <n> events`,
with `<n>` equal to the stage-1 count; `Metadata` has `numChan` 96.

## 4. Stage 3 -- digitizer -> offline file (`runOfflineFactory.py --slabsim`)

Run from `<cluster_offline_repo>/Run3Detector` -- `setup.sh` paths and the
configuration lookups are relative to it, and it crashes elsewhere. `setup.sh`
is bash, which the `bash -s` heredoc already provides.

All of this runs inside the container (see above).

```bash
cd <cluster_offline_repo>/Run3Detector
source setup.sh                      # prints "Working on OSU T3" / "UCSB cluster"
./compile.sh <exe>.exe               # every run: must print "Compiled macro <exe>.exe"
python3 scripts/runOfflineFactory.py \
  --inputFile <work>/<run>_digitizer.root \
  --outputFile <work>/<run>_offline.root \
  --exe ./<exe>.exe \
  --slabsim > <work>/<run>_stage3.log 2>&1
```

- `setup.sh` on OSU sets `MILLIDAQDIR=../../MilliDAQ`, so MilliDAQ must sit next to
  milliqanOffline (`<cluster_offline_repo>/../MilliDAQ`). If it doesn't, stop and
  tell the user.
- Recompile the executable every run, inside the container, so it always matches
  the checked-out milliqanOffline and MilliDAQ code. `compile.sh` prints `FAILED to
  compile macro` but still exits 0, so check the printed line. Ask for the name
  (`<exe>`) the first time and store it as `cluster_offline_exe`.
- `--slabsim` uses `configuration/slabConfigs/simConfig.json` +
  `configuration/pulseFinding/pulseFindingSlab.json`, run 1 / file 1.
- Before running, the script reads `configuration/barConfigs/mqLumis.json`, fetching
  it from `eospublic` with `xrdcp` if it is missing -- that fails on a node without
  external network. On cms-in0 the fetch worked.
- ~1 min for 1k events, including the compile. `Total lumi in this file: 0` and a few
  `TList::Clear` errors are expected for sim.
- For `--slabsim` the script skips its own output validation and always prints
  `Output made successfully`. **Check yourself** (PyROOT in the container): tree `t`
  has as many entries as the stage 1 input; `chan` reaches 95; and without zero bias,
  the mean `sidebandRMS` is about 1 mV.

## 5. Report

Give the three output paths, the event count at each stage, and whether zero bias was
used: the file, N, and how many times entries were reused. Note anything that
differed from this skill so it can be fixed.

## Open items (confirm with czheng, then update this file)

- Zero bias on the cluster: untested, waiting for a real zero-bias file.
- Upstream `slabGlobalEventDemo.C` still hardcodes the MilliDAQ path; if it is made
  portable (e.g. from `MILLIDAQDIR`), the skill could use it instead of its own copy.
- UCSB access and paths.
