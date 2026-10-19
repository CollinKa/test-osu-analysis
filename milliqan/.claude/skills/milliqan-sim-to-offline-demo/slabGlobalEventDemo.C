// Stage 2 of milliqan-sim-to-offline-demo: vectorized slab waveform file -> MilliDAQ
// digitizer-format file (GlobalEvent tree).
//
// Based on milliqanOffline SlabPulseInjectionSim @ 2fb617a,
// Run3Detector/analysis/simConversion/utils/slabGlobalEventDemo.C, with these fixes:
//  - the input is waveform[6][16][1024]/F from waveinject_slab.C (6 digitizers, 96
//    summing-amp channels); the upstream macro read only 5 digitizers into a 5-digitizer
//    buffer (overflow, channels 80-95 lost) and had an undefined name `waveformS`
//  - the branch shape is checked before reading
//  - no hardcoded user paths: the skill replaces @MILLIDAQ_DIR@ in a run copy
// Like upstream, every sample is sign-flipped: sim pulses are positive, slab data negative.
// waveinject_slab.C (>= 2dfd12a) subtracts the raw zero bias, so after this flip the
// output holds (sim x -1) + raw zero bias.
//
// Usage: root -l -b -q 'slabGlobalEventDemo.C("vector.root", "digitizer.root")'

#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TString.h"
#include "@MILLIDAQ_DIR@/interface/GlobalEvent.h"
#include "@MILLIDAQ_DIR@/interface/V1743Event.h"
#include <iostream>
#include <vector>

R__LOAD_LIBRARY(@MILLIDAQ_DIR@/libMilliDAQ.so)

int slabGlobalEventDemo(TString InputVecfileName, TString DigiOutputFileName) {
    const int N_DIG = 6, N_CH = 16, N_SAMP = 1024;  // slab: 6 x 16 = 96 channels

    if ((int)mdaq::GlobalEvent().digitizers.size() < N_DIG) {
        std::cerr << "libMilliDAQ.so has fewer than " << N_DIG << " digitizers; build MilliDAQ "
                  << "from branch SlabPulseInjectionSim (nDigitizers 7)" << std::endl;
        return 1;
    }

    TFile *inputFile = TFile::Open(InputVecfileName);
    if (!inputFile || inputFile->IsZombie()) {
        std::cerr << "Cannot open input file " << InputVecfileName << std::endl;
        return 1;
    }
    TTree *inputTree = (TTree*)inputFile->Get("Events");
    if (!inputTree) {
        std::cerr << "No Events tree in " << InputVecfileName << std::endl;
        return 1;
    }
    // Disable TTree read cache to avoid double-free in TTreeCache destructor during Close
    inputTree->SetCacheSize(0);

    TLeaf *leaf = inputTree->GetLeaf("waveform");
    if (!leaf || leaf->GetLen() != N_DIG * N_CH * N_SAMP) {
        std::cerr << "Input 'waveform' branch must be float[" << N_DIG << "][" << N_CH << "]["
                  << N_SAMP << "] (from waveinject_slab.C)" << std::endl;
        return 1;
    }
    std::vector<float> waveform(N_DIG * N_CH * N_SAMP, 0.f);
    inputTree->SetBranchAddress("waveform", waveform.data());

    TFile *outputFile = new TFile(DigiOutputFileName, "RECREATE");
    TTree *outputTree = new TTree("Events", "GlobalEvents TTree");
    TTree *metadata = new TTree("Metadata", "Configuration");

    float secondsPerSample = 2.5e-09;
    int numChan = N_DIG * N_CH;
    metadata->Branch("secondsPerSample", &secondsPerSample);
    metadata->Branch("numChan", &numChan);
    metadata->Fill();

    // GlobalEvent by value (not pointer) to avoid ROOT branch ownership/double-free issues
    mdaq::GlobalEvent globalEvent;
    outputTree->Branch("event", &globalEvent);

    Long64_t nEntries = inputTree->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) {
        inputTree->GetEntry(i);
        globalEvent.Reset();
        globalEvent.SetEventNumber(i);
        // Digitizers 0-5 get the sim waveform; digitizer 6 (nDigitizers 7) stays zero.
        for (int d = 0; d < N_DIG; ++d) {
            mdaq::V1743Event &digitizerEvent = globalEvent.digitizers[d];
            digitizerEvent.nanosecondsPerSample = 2.5;
            for (int ch = 0; ch < N_CH; ++ch)
                for (int sample = 0; sample < N_SAMP; ++sample)
                    digitizerEvent.waveform[ch][sample] = -waveform[(d * N_CH + ch) * N_SAMP + sample];
        }
        outputTree->Fill();
    }

    outputFile->cd();
    outputTree->Write();
    metadata->Write();

    // Unset branch address BEFORE closing to avoid double-free during TTree cleanup
    inputTree->SetBranchAddress("waveform", nullptr);
    outputFile->Close();
    inputFile->Close();

    std::cout << "Converted " << nEntries << " events: " << DigiOutputFileName << std::endl;
    return 0;
}
