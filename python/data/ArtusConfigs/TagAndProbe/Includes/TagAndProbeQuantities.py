#!/usr/bin/env python
# -*- coding: utf-8 -*-

import logging
import Artus.Utility.logger as logger
log = logging.getLogger(__name__)

import re
import json
import Artus.Utility.jsonTools as jsonTools
import Kappa.Skimming.datasetsHelperTwopz as datasetsHelperTwopz

def build_list():
  quantities_list = [
    "wt",
    "n_vtx",
    "run",
    "lumi",
    "evt",
    "usedMuonIDshortTerm",
    "pt_t",
    "eta_t",
    "phi_t",
    "id_t",
    "iso_t",
    "muon_p",
    "trk_p",
    "pt_p",
    "eta_p",
    "phi_p",
    "id_p",
    "iso_p",
    "dxy_p",
    "dz_p",
    "gen_p",
    "genZ_p",
    "m_ll",
    "trg_t_IsoMu22",
    "trg_t_IsoMu22_eta2p1",
    "trg_t_IsoMu24",
    "trg_t_IsoMu19Tau",
    "trg_p_IsoMu22",
    "trg_p_IsoTkMu22",
    "trg_p_IsoMu22_eta2p1",
    "trg_p_IsoTkMu22_eta2p1",
    "trg_p_IsoMu24",
    "trg_p_IsoTkMu24",
    "trg_p_PFTau120",
    "trg_p_IsoMu19TauL1",
    "trg_p_IsoMu19Tau"
    ]

  return quantities_list