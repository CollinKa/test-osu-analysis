"""Plot the PMT nPE distribution from a flattened slab sim file.

Usage: python plot_npe.py <flat.root> [--max-npe N]
Writes <flat stem>_npe.png and <flat stem>_npe.root next to the input and prints a summary.
"""

import argparse
from pathlib import Path

import ROOT


parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
parser.add_argument("flat_file", help="output of flatlightwithphotonslab_latest.py")
parser.add_argument("--max-npe", type=int, default=50, help="upper edge of the nPE axis (default 50)")
args = parser.parse_args()

ROOT.gROOT.SetBatch(True)
flat_path = Path(args.flat_file).expanduser().resolve()
in_file = ROOT.TFile.Open(str(flat_path))
if not in_file or in_file.IsZombie():
    raise SystemExit(f"cannot open {flat_path}")
tree = in_file.Get("t")
if not tree:
    raise SystemExit(f"no tree 't' in {flat_path}; is this a flattened file?")

nbins = args.max_npe + 1
lo, hi = -0.5, args.max_npe + 0.5
out_stem = flat_path.with_name(f"{flat_path.stem}_npe")
out_file = ROOT.TFile(f"{out_stem}.root", "RECREATE")

# One entry per PMT with at least one PE (the flattener drops zero-PE channels)
h_pmt = ROOT.TH1D("h_npe_per_pmt", f"{flat_path.stem};nPE per PMT;PMT hits", nbins, lo, hi)
tree.Draw("pmt_nPE>>h_npe_per_pmt", "", "goff")
# One entry per event: nPE summed over all PMTs (0 when no PMT fired)
h_event = ROOT.TH1D("h_npe_per_event", f"{flat_path.stem};total nPE per event;events", nbins, lo, hi)
tree.Draw("Sum$(pmt_nPE)>>h_npe_per_event", "", "goff")
# nPE per data channel, for spotting a single channel dominating
h_chan = ROOT.TH2D("h_npe_vs_chan", f"{flat_path.stem};pmt_chan (PMT copy number);nPE", 500, -0.5, 499.5, nbins, lo, hi)
tree.Draw("pmt_nPE:pmt_chan>>h_npe_vs_chan", "", "goff")

canvas = ROOT.TCanvas("c", "", 1200, 500)
canvas.Divide(2, 1)
for pad, hist in ((1, h_pmt), (2, h_event)):
    canvas.cd(pad)
    ROOT.gPad.SetLogy()
    hist.Draw("hist")
canvas.SaveAs(f"{out_stem}.png")
out_file.Write()

print(f"events: {tree.GetEntries()}")
print(f"PMT hits (nPE>0): {int(h_pmt.GetEntries())}, mean nPE per PMT: {h_pmt.GetMean():.2f}")
print(f"mean total nPE per event: {h_event.GetMean():.2f}, events with 0 PE: {int(h_event.GetBinContent(1))}")
overflow = h_pmt.GetBinContent(nbins + 1) + h_event.GetBinContent(nbins + 1)
if overflow:
    print(f"warning: {int(overflow)} entries above --max-npe {args.max_npe}; rerun with a larger value")
print(f"wrote {out_stem}.png and {out_stem}.root")
