
#include "HHKinFit2/HHKinFit2/interface/HHKinFitMasterSingleHiggs.h"

#include "Artus/Consumer/interface/LambdaNtupleConsumer.h"
#include "Artus/Utility/interface/DefaultValues.h"

#include "HiggsAnalysis/KITHiggsToTauTau/interface/Producers/HHKinFitProducer.h"


TLorentzVector HHKinFitProducer::GetTauLorentzVector(RMFLV const& tauFourMomentum)
{
	TLorentzVector tauLorentzVector;
	tauLorentzVector.SetPtEtaPhiM(
			tauFourMomentum.Pt(),
			tauFourMomentum.Eta(),
			tauFourMomentum.Phi(),
			tauFourMomentum.M()
	);
	tauLorentzVector.Print();
	return tauLorentzVector;
}


TVector2 HHKinFitProducer::GetMetComponents(RMFLV const& metFourMomentum)
{
	TVector2(metFourMomentum.Px(), metFourMomentum.Py()).Print();
	return TVector2(metFourMomentum.Px(), metFourMomentum.Py());
}

TMatrixD HHKinFitProducer::GetMetCovarianceMatrix(ROOT::Math::SMatrix<double, 2, 2, ROOT::Math::MatRepSym<double, 2> > const& metSignificance)
{
    TMatrixD metCovMatrix(2, 2);
    metCovMatrix[0][0] = metSignificance.At(0, 0);
    metCovMatrix[1][0] = metSignificance.At(1, 0);
    metCovMatrix[0][1] = metSignificance.At(0, 1);
    metCovMatrix[1][1] = metSignificance.At(1, 1);
	metCovMatrix.Print();
    return metCovMatrix;
}

void HHKinFitProducer::Init(setting_type const& settings)
{
	ProducerBase<HttTypes>::Init(settings);
	
	// add possible quantities for the lambda ntuples consumers
	//LambdaNtupleConsumer<HttTypes>::AddFloatQuantity("svfitMass", [](event_type const& event, product_type const& product) {
	//	return (product.m_svfitResults.momentum ? product.m_svfitResults.momentum->mass() : DefaultValues::UndefinedFloat);
	//});
}

void HHKinFitProducer::Produce(event_type const& event, product_type& product,
                            setting_type const& settings) const
{
	assert(product.m_met);
	LOG(INFO) << "--------------------------------------------------------------";

	// consider only the first two leptons
	assert(product.m_flavourOrderedLeptons.size() >= 2);
	
	HHKinFit2::HHKinFitMasterSingleHiggs hhKinFit(
			HHKinFitProducer::GetTauLorentzVector(product.m_flavourOrderedLeptons[0]->p4),
			HHKinFitProducer::GetTauLorentzVector(product.m_flavourOrderedLeptons[1]->p4),
			HHKinFitProducer::GetMetComponents(product.m_met->p4),
			HHKinFitProducer::GetMetCovarianceMatrix(product.m_met->significance)
	);
	
	hhKinFit.addHypo(90);
	
	hhKinFit.fit();
	LOG(INFO) << hhKinFit.getBestHypothesis();
	LOG(INFO) << "==============================================================";
	
	/*
	// construct decay types
	svFitStandalone::kDecayType decayType1 = svFitStandalone::kTauToHadDecay;
	if (product.m_decayChannel == HttEnumTypes::DecayChannel::MT || product.m_decayChannel == HttEnumTypes::DecayChannel::MM)
	{
		decayType1 = svFitStandalone::kTauToMuDecay;
	}
	else if (product.m_decayChannel == HttEnumTypes::DecayChannel::ET || product.m_decayChannel == HttEnumTypes::DecayChannel::EE)
	{
		decayType1 = svFitStandalone::kTauToElecDecay;
	}
	
	svFitStandalone::kDecayType decayType2 = svFitStandalone::kTauToHadDecay;
	if (product.m_decayChannel == HttEnumTypes::DecayChannel::MM)
	{
		decayType2 = svFitStandalone::kTauToMuDecay;
	}
	else if (product.m_decayChannel == HttEnumTypes::DecayChannel::EE)
	{
		decayType2 = svFitStandalone::kTauToElecDecay;
	}
	
	// construct event key
	product.m_svfitEventKey.Set(event.m_eventInfo->nRun, event.m_eventInfo->nLumi, event.m_eventInfo->nEvent,
	                            decayType1, decayType2,
	                            product.m_systematicShift, product.m_systematicShiftSigma, integrationMethod);
	
	// construct inputs
	product.m_svfitInputs.Set(product.m_flavourOrderedLeptons[0]->p4, product.m_flavourOrderedLeptons[1]->p4,
	                          product.m_met->p4.Vect(), product.m_met->significance);
	
	// calculate results
	product.m_svfitResults = HHKinFitProducer::svfitTools.GetResults(product.m_svfitEventKey,
	                                                              product.m_svfitInputs,
	                                                              product.m_svfitCalculated,
	                                                              settings.GetSvfitCheckInputs());
	
	// apply systematic shifts
	product.m_svfitResults.momentum->SetM(product.m_svfitResults.momentum->M() * settings.GetSvfitMassShift());
	*/
}

