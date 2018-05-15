#!/usr/bin/env python
# -*- coding: utf-8 -*-

import logging
import Artus.Utility.logger as logger
log = logging.getLogger(__name__)

import re
import json
import Artus.Utility.jsonTools as jsonTools
import Kappa.Skimming.datasetsHelperTwopz as datasetsHelperTwopz
import importlib
import os

def build_config(nickname):
  config = jsonTools.JsonDict()
  datasetsHelper = datasetsHelperTwopz.datasetsHelperTwopz(os.path.expandvars("$CMSSW_BASE/src/Kappa/Skimming/data/datasets.json"))
  
  
  # define frequently used conditions
  isEmbedded = datasetsHelper.isEmbedded(nickname)
  isData = datasetsHelper.isData(nickname) and (not isEmbedded)
  isTTbar = re.search("TT(To|_|Jets)", nickname)
  isDY = re.search("DY.?JetsToLLM(10to50|50)", nickname)
  isWjets = re.search("W.?JetsToLNu", nickname)
  isSUSYggH = re.search("SUSYGluGluToHToTauTau", nickname)
  isSignal = re.search("HToTauTau",nickname)
  
  
  ## fill config:
  # includes
  includes = [
    "HiggsAnalysis.KITHiggsToTauTau.data.ArtusConfigs.Includes.settingsKappa",
    "HiggsAnalysis.KITHiggsToTauTau.data.ArtusConfigs.Includes.lheWeightAssignment",
    "HiggsAnalysis.KITHiggsToTauTau.data.ArtusConfigs.Includes.settingsSampleStitchingWeights"
    ]
  for include_file in includes:
    analysis_config_module = importlib.import_module(include_file)
    config += analysis_config_module.build_config(nickname)
  
  # explicit configuration
  config["SkipEvents"] = 0
  config["EventCount"] = -1
  config["Year"] = 2017
  config["InputIsData"] = isData
  
  if isSUSYggH:
    config["HiggsBosonMass"] = re.search("SUSYGluGluToHToTauTauM(\d+)_", nickname).groups()[0] #extracts generator mass from nickname
    config["NLOweightsRooWorkspace"] = "$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/NLOWeights/higgs_pt_v2_mssm_mode.root"

  BosonPdgIds = {
      "DY.?JetsToLL|EWKZ2Jets|Embedding(2016|MC)" : [
        23
      ],
      "^(GluGlu|GluGluTo|VBF|Wminus|Wplus|Z)(HToTauTau|H2JetsToTauTau)" : [
        25
      ],
      "W.?JetsToLN|EWKW" : [
        24
      ],
      "SUSY(BB|GluGlu|GluGluTo)(BB)?HToTauTau" : [
        25,
        35,
        36
        ]
  }
  config["BosonPdgIds"] = [0]
  for key, pdgids in BosonPdgIds.items():
    if re.search(key, nickname): config["BosonPdgIds"] = pdgids
  
  config["BosonStatuses"] = [62]
  config["ChooseMvaMet"] = False
  config["DeltaRMatchingRecoElectronGenParticle"] = 0.2
  config["DeltaRMatchingRecoElectronGenTau"] = 0.2
  config["DeltaRMatchingRecoMuonGenParticle"] = 0.2
  config["DeltaRMatchingRecoMuonGenTau"] = 0.2
  config["DeltaRMatchingRecoTauGenParticle"] = 0.2
  config["DeltaRMatchingRecoTauGenTau"] = 0.2
  config["RecoElectronMatchingGenParticlePdgIds"] = [11,13]
  config["RecoMuonMatchingGenParticlePdgIds"] = [11,13]
  config["RecoTauMatchingGenParticlePdgIds"] = [11,13]
  config["RecoElectronMatchingGenParticleMatchAllElectrons"] = "true"
  config["RecoMuonMatchingGenParticleMatchAllMuons"] = "true"
  config["RecoTauMatchingGenParticleMatchAllTaus"] = "true"
  config["MatchAllElectronsGenTau"] = "true"
  config["MatchAllMuonsGenTau"] = "true"
  config["MatchAllTausGenTau"] = "true"
  config["UpdateMetWithCorrectedLeptons"] = "true"
  
  config["MetFilter"] = [
        "Flag_HBHENoiseFilter",
        "Flag_HBHENoiseIsoFilter",
        "Flag_EcalDeadCellTriggerPrimitiveFilter",
        "Flag_goodVertices",
        "Flag_BadPFMuonFilter",
        "Flag_BadChargedCandidateFilter"
  ]
  if isData:
    config["MetFilter"].extend((
        "Flag_eeBadScFilter",
    ))
  else:
    config["MetFilter"].extend((
    ))
  if re.search(".*Summer17.*",nickname):
    config["MetFilter"].extend((
        "Flag_globalSuperTightHalo2016Filter",
    ))
  else:
    config["MetFilter"].extend((
        "Flag_ecalBadCalibFilter",
        "Flag_globalTightHalo2016Filter",
    ))
  
  config["OutputPath"] = "output.root"
  
  config["Processors"] = []
  #config["Processors"].append("filter:RunLumiEventFilter")
  if isData or isEmbedded:             config["Processors"].append( "filter:JsonFilter")
  if not isEmbedded:                   config["Processors"].append( "filter:MetFilter")
  #if isDY or isTTbar:                  config["Processors"].append( "producer:ScaleVariationProducer")
  config["Processors"].append(                                      "producer:NicknameProducer")
  if not isData:
    config["Processors"].extend((                                   "producer:CrossSectionWeightProducer",
                                                                    "producer:NumberGeneratedEventsWeightProducer"))
    if not isEmbedded:                 config["Processors"].append( "producer:PUWeightProducer")
    if isWjets or isDY or isSignal:    config["Processors"].append( "producer:GenBosonFromGenParticlesProducer")
    if isDY or isEmbedded:             config["Processors"].append( "producer:GenDiLeptonDecayModeProducer")
    config["Processors"].extend((                                   "producer:GenParticleProducer",
                                                                    "producer:GenPartonCounterProducer"))
    if isSUSYggH:                      config["Processors"].append( "producer:NLOreweightingWeightsProducer")
    if isWjets or isDY or isEmbedded:  config["Processors"].extend(("producer:GenTauDecayProducer",
                                                                    "producer:GenBosonDiLeptonDecayModeProducer"))
    config["Processors"].extend((                                   "producer:GeneratorWeightProducer",
                                                                    "producer:RecoMuonGenParticleMatchingProducer",
                                                                    "producer:RecoMuonGenTauMatchingProducer",
                                                                    "producer:RecoElectronGenParticleMatchingProducer",
                                                                    "producer:RecoElectronGenTauMatchingProducer",
                                                                    "producer:RecoTauGenParticleMatchingProducer",
                                                                    "producer:RecoTauGenTauMatchingProducer",
                                                                    "producer:MatchedLeptonsProducer"))
    #if isTTbar:                        config["Processors"].append( "producer:TTbarGenDecayModeProducer")

  if isData or isEmbedded:                config["PileupWeightFile"] = "not needed"
  elif re.search(".*Fall17", nickname): config["PileupWeightFile"] = "$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/pileup/Data_Pileup_2017_294927-306462_13TeVFall17_17Nov2017ReReco_69p2mbMinBiasXS/%s.root"%nickname
  elif re.search(".*Summer17", nickname): config["PileupWeightFile"] = "$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/pileup/Data_Pileup_2017_294927-306462_13TeVSummer17_PromptReco_69p2mbMinBiasXS.root"
  #elif re.search(".*Summer17", nickname): config["PileupWeightFile"] = "$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/pileup/Data_Pileup_2017_294927-306462_13TeVSummer17_PromptReco_80p0mbMinBiasXS.root"
  elif re.search(".*Summer16", nickname): config["PileupWeightFile"] = "$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/pileup/Data_Pileup_2016_271036-284044_13TeVMoriond17_23Sep2016ReReco_69p2mbMinBiasXS.root"
  else:                                   config["PileupWeightFile"] = "$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/pileup/Data_Pileup_2015_246908-260627_13TeVFall15MiniAODv2_PromptReco_69mbMinBiasXS.root"

  config["ZptRooWorkspace"] = "$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/scaleFactorWeights/htt_scalefactors_v16_5.root"
  config["DoZptUncertainties"] = True
  config["MetRecoilCorrectorFile"] = "$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/recoilMet/TypeI-PFMet_Run2016BtoH.root"
  config["MetShiftCorrectorFile"] = "$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/recoilMet/PFMEtSys_2016.root"
  config["MvaMetRecoilCorrectorFile"] = "$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/recoilMet/MvaMET_2016BCD.root"
  config["MetCorrectionMethod"] = "meanResolution"
  
  if isData or isEmbedded:
    if   re.search("Run2017|Embedding2017", nickname):      config["JsonFiles"] = ["$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/json/Cert_294927-306462_13TeV_EOY2017ReReco_Collisions17_JSON.txt"]
    elif re.search("Run2016|Embedding2016", nickname):      config["JsonFiles"] = ["$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/json/Cert_271036-284044_13TeV_23Sep2016ReReco_Collisions16_JSON.txt"]
    elif re.search("Run2015(C|D)|Embedding2015", nickname): config["JsonFiles"] = ["$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/json/Cert_13TeV_16Dec2015ReReco_Collisions15_25ns_JSON_v2.txt"]
    elif re.search("Run2015B", nickname):                   config["JsonFiles"] = ["$CMSSW_BASE/src/HiggsAnalysis/KITHiggsToTauTau/data/root/json/Cert_13TeV_16Dec2015ReReco_Collisions15_50ns_JSON_v2.txt"]
    
  config["SimpleMuTauFakeRateWeightLoose"] = [1.06, 1.02, 1.10, 1.03, 1.94]
  config["SimpleMuTauFakeRateWeightTight"] = [1.17, 1.29, 1.14, 0.93, 1.61]
  config["SimpleEleTauFakeRateWeightVLoose"] = [1.09, 1.19]
  config["SimpleEleTauFakeRateWeightTight"] = [1.80, 1.53]

  
  # pipelines - channels including systematic shifts
  config["Pipelines"] = jsonTools.JsonDict()
  #config["Pipelines"] += importlib.import_module("HiggsAnalysis.KITHiggsToTauTau.data.ArtusConfigs.Run2MSSM2017.ee").build_config(nickname)
  config["Pipelines"] += importlib.import_module("HiggsAnalysis.KITHiggsToTauTau.data.ArtusConfigs.Run2MSSM2017.em").build_config(nickname)
  config["Pipelines"] += importlib.import_module("HiggsAnalysis.KITHiggsToTauTau.data.ArtusConfigs.Run2MSSM2017.et").build_config(nickname)
  #config["Pipelines"] += importlib.import_module("HiggsAnalysis.KITHiggsToTauTau.data.ArtusConfigs.Run2MSSM2017.mm").build_config(nickname)
  config["Pipelines"] += importlib.import_module("HiggsAnalysis.KITHiggsToTauTau.data.ArtusConfigs.Run2MSSM2017.mt").build_config(nickname)
  config["Pipelines"] += importlib.import_module("HiggsAnalysis.KITHiggsToTauTau.data.ArtusConfigs.Run2MSSM2017.tt").build_config(nickname)


  return config
