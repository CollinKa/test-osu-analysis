#include "TCanvas.h"
#include "TChain.h"
#include "TFile.h"
#include "TF1.h"
#include "TFitResultPtr.h"
#include "TGraphErrors.h"
#include "TH1.h"
#include "TAxis.h"
#include "TMath.h"
#include "TPaveStats.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <vector>


void FillSlabFlatSumNPE(TH1D* Ch18NPE, TH1D* Ch19NPE, TH1D* Ch20NPE, TH1D* Ch21NPE, TH1D* sCh18AndCh19PerEvent, TH1D* sCh20AndCh21PerEvent, TH1D* sAllChPerEvent, const char* filename)
{
  if (!Ch18NPE || !Ch19NPE || !Ch20NPE || !Ch21NPE || !sCh18AndCh19PerEvent || !sCh20AndCh21PerEvent || !sAllChPerEvent) return;
  Ch18NPE->Reset();
  Ch19NPE->Reset();
  Ch20NPE->Reset();
  Ch21NPE->Reset();
  sCh18AndCh19PerEvent->Reset();
  sCh20AndCh21PerEvent->Reset();
  sAllChPerEvent->Reset();

  TFile* f = TFile::Open(filename, "READ");
  if (!f || f->IsZombie()) {
    return;
  }
  TTree* t = dynamic_cast<TTree*>(f->Get("t"));
  if (!t) {
    f->Close();
    delete f;
    return;
  }

  std::vector<int>* pmt_type = nullptr;
  std::vector<float>* pmt_nPE = nullptr;
  std::vector<int>* pmt_chan = nullptr;
  t->SetBranchAddress("pmt_type", &pmt_type);
  t->SetBranchAddress("pmt_nPE", &pmt_nPE);
  t->SetBranchAddress("pmt_chan", &pmt_chan);
  std::map<Long64_t, float> sCh18AndCh19SumPerEvent;
  std::map<Long64_t, float> sCh20AndCh21SumPerEvent;
  std::map<Long64_t, float> sAllChSumPerEvent;
  std::map<Long64_t, float> Ch18NPEPerEvent;
  std::map<Long64_t, float> Ch19NPEPerEvent;
  std::map<Long64_t, float> Ch20NPEPerEvent;
  std::map<Long64_t, float> Ch21NPEPerEvent;

  Long64_t n = t->GetEntries();
  for (Long64_t i = 0; i < n; i++) {
    t->GetEntry(i);
    if (!pmt_type || !pmt_nPE) continue;
    size_t nHits = std::min(pmt_type->size(), pmt_nPE->size());
    for (size_t j = 0; j < nHits; ++j) {

      if (pmt_chan->at(j) == 18) {
        Ch18NPEPerEvent[i] += pmt_nPE->at(j);
        sCh18AndCh19SumPerEvent[i] += pmt_nPE->at(j);
        sAllChSumPerEvent[i] += pmt_nPE->at(j);
        Ch18NPE->Fill(pmt_nPE->at(j));
      }
      if (pmt_chan->at(j) == 19) {
        Ch19NPEPerEvent[i] += pmt_nPE->at(j);
        sCh18AndCh19SumPerEvent[i] += pmt_nPE->at(j);
        sAllChSumPerEvent[i] += pmt_nPE->at(j);
        Ch19NPE->Fill(pmt_nPE->at(j));
      }
      if (pmt_chan->at(j) == 20) {
        Ch20NPEPerEvent[i] += pmt_nPE->at(j);
        sCh20AndCh21SumPerEvent[i] += pmt_nPE->at(j);
        sAllChSumPerEvent[i] += pmt_nPE->at(j);
        Ch20NPE->Fill(pmt_nPE->at(j));
      }
      if (pmt_chan->at(j) == 21) {
        Ch21NPEPerEvent[i] += pmt_nPE->at(j);
        sCh20AndCh21SumPerEvent[i] += pmt_nPE->at(j);
        sAllChSumPerEvent[i] += pmt_nPE->at(j);
        Ch21NPE->Fill(pmt_nPE->at(j));
      }
    }// end of single event loop
    sCh18AndCh19PerEvent->Fill(sCh18AndCh19SumPerEvent[i]);
    sCh20AndCh21PerEvent->Fill(sCh20AndCh21SumPerEvent[i]);
    sAllChPerEvent->Fill(sAllChSumPerEvent[i]);
    
  }


  f->Close();
  delete f;
}

Double_t FindPeakSeedInRange(TH1* h, Double_t lo, Double_t hi)
{
  TH1* hs = (TH1*)h->Clone(Form("%s_peak_seed_smooth", h->GetName()));
  hs->SetDirectory(nullptr);
  hs->Smooth(2);

  Int_t firstBin = std::max(1, hs->FindBin(lo));
  Int_t lastBin = std::min(hs->GetNbinsX(), hs->FindBin(hi));
  Int_t bestBin = firstBin;
  Double_t bestContent = hs->GetBinContent(firstBin);

  for (Int_t ib = firstBin + 1; ib <= lastBin; ++ib) {
    Double_t content = hs->GetBinContent(ib);
    if (content > bestContent) {
      bestContent = content;
      bestBin = ib;
    }
  }

  Double_t seed = hs->GetBinCenter(bestBin);
  delete hs;
  return seed;
}

TF1* FitPeakSeed(TH1* h, const char* name, Double_t seed, Double_t halfWidth)
{
  Double_t lo = std::max(h->GetXaxis()->GetXmin(), seed - halfWidth);
  Double_t hi = std::min(h->GetXaxis()->GetXmax(), seed + halfWidth);
  TF1* f = new TF1(name, "gaus", lo, hi);
  f->SetParameters(h->GetBinContent(h->FindBin(seed)), seed, 0.5 * halfWidth);
  f->SetParLimits(1, lo, hi);
  f->SetParLimits(2, h->GetBinWidth(1), 2. * halfWidth);
  h->Fit(f, "R Q 0");
  return f;
}

TGraphErrors* MakePeakOnlyGraph(TH1* h,
                                const char* graphName,
                                Double_t mean1, Double_t sigma1,
                                Double_t mean2, Double_t sigma2,
                                Double_t nSigma)
{
  TGraphErrors* g = new TGraphErrors();
  g->SetName(graphName);
  g->SetTitle(Form("%s bins used for double Gaussian fit", h->GetName()));

  sigma1 = std::max(std::abs(sigma1), h->GetBinWidth(1));
  sigma2 = std::max(std::abs(sigma2), h->GetBinWidth(1));

  Int_t ip = 0;
  for (Int_t ib = 1; ib <= h->GetNbinsX(); ++ib) {
    Double_t x = h->GetBinCenter(ib);
    Bool_t inPeak1 = std::abs(x - mean1) <= nSigma * sigma1;
    Bool_t inPeak2 = std::abs(x - mean2) <= nSigma * sigma2;
    if (!inPeak1 && !inPeak2) continue;

    Double_t y = h->GetBinContent(ib);
    if (y <= 0.) continue;

    Double_t ey = h->GetBinError(ib);
    if (ey <= 0.) ey = TMath::Sqrt(std::max(y, 1.0));

    g->SetPoint(ip, x, y);
    g->SetPointError(ip, 0.5 * h->GetBinWidth(ib), ey);
    ++ip;
  }

  return g;
}

TFitResultPtr FitPeakWindowDoubleGaussian(TH1* h,
                                          const char* label,
                                          const char* funcName,
                                          const char* graphName,
                                          Double_t peak1SearchLo,
                                          Double_t peak1SearchHi,
                                          Double_t peak2SearchLo,
                                          Double_t peak2SearchHi,
                                          Double_t seedFitHalfWidth,
                                          Double_t finalFitNSigma,
                                          Double_t maxSigmaForFinalWindow,
                                          TF1*& fitFunc,
                                          TGraphErrors*& fitPoints)
{
  Double_t peak1Seed = FindPeakSeedInRange(h, peak1SearchLo, peak1SearchHi);
  Double_t peak2Seed = FindPeakSeedInRange(h, peak2SearchLo, peak2SearchHi);

  TF1* seedGaus1 = FitPeakSeed(h, Form("%s_seedGaus1", funcName), peak1Seed, seedFitHalfWidth);
  TF1* seedGaus2 = FitPeakSeed(h, Form("%s_seedGaus2", funcName), peak2Seed, seedFitHalfWidth);

  Double_t amp1 = seedGaus1->GetParameter(0);
  Double_t mean1 = seedGaus1->GetParameter(1);
  Double_t sigma1 = std::min(std::abs(seedGaus1->GetParameter(2)), maxSigmaForFinalWindow);
  Double_t amp2 = seedGaus2->GetParameter(0);
  Double_t mean2 = seedGaus2->GetParameter(1);
  Double_t sigma2 = std::min(std::abs(seedGaus2->GetParameter(2)), maxSigmaForFinalWindow);

  if (std::abs(mean1 - peak1Seed) > seedFitHalfWidth) mean1 = peak1Seed;
  if (std::abs(mean2 - peak2Seed) > seedFitHalfWidth) mean2 = peak2Seed;
  if (mean2 < mean1) {
    std::swap(amp1, amp2);
    std::swap(mean1, mean2);
    std::swap(sigma1, sigma2);
  }

  fitPoints = MakePeakOnlyGraph(h, graphName, mean1, sigma1, mean2, sigma2, finalFitNSigma);
  fitPoints->SetMarkerStyle(20);
  fitPoints->SetMarkerSize(0.8);
  fitPoints->SetMarkerColor(kGreen + 2);
  fitPoints->SetLineColor(kGreen + 2);

  fitFunc = new TF1(funcName, "gaus(0)+gaus(3)",
                    mean1 - finalFitNSigma * sigma1,
                    mean2 + finalFitNSigma * sigma2);
  fitFunc->SetParNames("Constant1", "Mean1", "Sigma1",
                       "Constant2", "Mean2", "Sigma2");
  fitFunc->SetParameters(amp1, mean1, sigma1, amp2, mean2, sigma2);
  fitFunc->SetParLimits(0, 0., 1.e9);
  fitFunc->SetParLimits(3, 0., 1.e9);

  Double_t binWidth = h->GetBinWidth(1);
  Double_t split = 0.5 * (mean1 + mean2);
  Double_t mean1Lo = mean1 - finalFitNSigma * sigma1;
  Double_t mean1Hi = std::min(mean1 + finalFitNSigma * sigma1, split - binWidth);
  Double_t mean2Lo = std::max(mean2 - finalFitNSigma * sigma2, split + binWidth);
  Double_t mean2Hi = mean2 + finalFitNSigma * sigma2;
  if (mean1Hi <= mean1Lo) {
    mean1Lo = mean1 - binWidth;
    mean1Hi = mean1 + binWidth;
  }
  if (mean2Hi <= mean2Lo) {
    mean2Lo = mean2 - binWidth;
    mean2Hi = mean2 + binWidth;
  }

  fitFunc->SetParLimits(1, mean1Lo, mean1Hi);
  fitFunc->SetParLimits(4, mean2Lo, mean2Hi);
  fitFunc->SetParLimits(2, binWidth, maxSigmaForFinalWindow);
  fitFunc->SetParLimits(5, binWidth, maxSigmaForFinalWindow);
  fitFunc->SetLineColor(kGreen + 2);
  fitFunc->SetLineWidth(2);
  fitFunc->SetNpx(800);

  TFitResultPtr fitResult = fitPoints->Fit(fitFunc, "S R Q");
  std::printf("\n=== %s peak-window double Gaussian fit ===\n", label);
  std::printf("Peak search seeds: %.4g, %.4g\n", peak1Seed, peak2Seed);
  std::printf("Selected graph points: %d\n", fitPoints->GetN());
  if (fitResult.Get()) {
    Int_t ndf = fitResult->Ndf();
    std::printf("Chi2 / NDF = %.4g / %d = %.4g\n",
                fitResult->Chi2(), ndf,
                ndf > 0 ? fitResult->Chi2() / ndf : 0.);
    std::printf("Fit probability (p-value) = %.4g\n", fitResult->Prob());
  } else {
    std::printf("(TFitResult not available; check ROOT version / fit status)\n");
    std::printf("Chi2 / NDF = %.4g / %d\n", fitFunc->GetChisquare(), fitFunc->GetNDF());
  }
  std::printf("Parameters:\n");
  for (Int_t p = 0; p < fitFunc->GetNpar(); ++p) {
    std::printf("  %s = %.5g +/- %.5g\n",
                fitFunc->GetParName(p),
                fitFunc->GetParameter(p),
                fitFunc->GetParError(p));
  }
  std::printf("\n");

  return fitResult;
}



// Usage: root -l -b -q 'GetSlabFlatSimNpe.C("<flat.root>")'
// Writes all histograms, plus the cSAllCh canvas, to <flat stem>_npe.root.
void GetSlabFlatSimNpe(const char* inputFile)
{
  // Load multiple files (same tree name "t" in each)
  TChain ch("t");
  TChain NoSource("t");

  //const char* fileQEScale0p37 = "/Users/haoliangzheng/2025WorkShop/sourceTest/SourceTestOffline/MilliQan_slabSourceTest_flat_QEScaleChanDep_Random.root";
  //TH1D* hQEScale0p37 = new TH1D("hQEScale0p37", "pmt_nPE ; nPE;Events", 20, 0, 20);
  const char* fileQEScale1CenterSource = inputFile;
  TH1D* sCh18AndCh19PerEvent = new TH1D("sCh18AndCh19PerEvent", "sCh18AndCh19NPE", 20, 0, 20);
  TH1D* sCh20AndCh21PerEvent = new TH1D("sCh20AndCh21PerEvent", "sCh20AndCh21NPE", 20, 0, 20);
  TH1D* sAllChPerEvent = new TH1D("sAllChPerEvent", "sCh18To21NPE", 20, 0, 20);
  TH1D* hCh18NPE = new TH1D("hCh18NPE", "hCh18NPE", 20, 0, 20);
  TH1D* hCh19NPE = new TH1D("hCh19NPE", "hCh19NPE", 20, 0, 20);
  TH1D* hCh20NPE = new TH1D("hCh20NPE", "hCh20NPE", 20, 0, 20);
  TH1D* hCh21NPE = new TH1D("hCh21NPE", "hCh21NPE", 20, 0, 20);
  FillSlabFlatSumNPE(hCh18NPE, hCh19NPE, hCh20NPE, hCh21NPE, sCh18AndCh19PerEvent, sCh20AndCh21PerEvent, sAllChPerEvent, fileQEScale1CenterSource);

  TCanvas* cSCh18AndCh19 = new TCanvas("cSCh18AndCh19", "Ch18+Ch19 sum per event", 800, 600);
  sCh18AndCh19PerEvent->Draw();

  TCanvas* cSCh20AndCh21 = new TCanvas("cSCh20AndCh21", "Ch20+Ch21 sum per event", 800, 600);
  sCh20AndCh21PerEvent->Draw();

  TCanvas* cSAllCh = new TCanvas("cSAllCh", "Ch18+Ch19+Ch20+Ch21 sum per event", 800, 600);
  sAllChPerEvent->Draw();
  // Stats box also shows underflow/overflow ("ourmen"): at QE ~1 the 4-channel sum runs past 20
  cSAllCh->Update();
  if (auto* st = dynamic_cast<TPaveStats*>(sAllChPerEvent->FindObject("stats"))) st->SetOptStat(111111);
  cSAllCh->Modified();

  TCanvas* cCh18 = new TCanvas("cCh18", "Ch18 nPE", 800, 600);
  hCh18NPE->Draw();

  TCanvas* cCh19 = new TCanvas("cCh19", "Ch19 nPE", 800, 600);
  hCh19NPE->Draw();

  TCanvas* cCh20 = new TCanvas("cCh20", "Ch20 nPE", 800, 600);
  hCh20NPE->Draw();

  TCanvas* cCh21 = new TCanvas("cCh21", "Ch21 nPE", 800, 600);
  hCh21NPE->Draw();

  TString stem(inputFile);
  stem.ReplaceAll(".root", "");
  TFile out(Form("%s_npe.root", stem.Data()), "RECREATE");
  for (TH1D* h : {sCh18AndCh19PerEvent, sCh20AndCh21PerEvent, sAllChPerEvent, hCh18NPE, hCh19NPE, hCh20NPE, hCh21NPE}) {
    h->Write();
    std::printf("%-22s entries %8.0f  mean %6.3f  overflow %.0f\n",
                h->GetName(), h->GetEntries(), h->GetMean(), h->GetBinContent(h->GetNbinsX() + 1));
  }
  // The underflow/overflow stats box lives on the canvas, so keep the canvas too
  cSAllCh->Write();
  out.Close();
  std::printf("wrote %s_npe.root\n", stem.Data());
}


  