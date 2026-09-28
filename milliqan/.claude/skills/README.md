MilliQan analysis skills. Run Claude Code from `milliqan/` so these load.

Simulation (local macOS, `CollinKa/milliQanSim` @ `MacArmGeant4P11`):

- `milliqan-sim-env-setup` -- one-time: clone, conda env, Geant4 11.2.2 build
- `milliqan-sim-slab-bench` -- build the Bench sim and run a slab source test (Cd-109 by default)
- `milliqan-sim-flatten-npe` -- flatten a sim file with a QE rescale and plot the PMT nPE distribution

Each skill names the code commit it was tested against in its **Code:** line and
checks the user's clone against it before running. Update that line whenever a
skill is revised for newer code.

Per-machine paths live in `milliqan/CLAUDE.local.md` (gitignored); the env-setup
skill writes it.
