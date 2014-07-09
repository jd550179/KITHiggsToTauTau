
#include "HiggsAnalysis/KITHiggsToTauTau/interface/Producers/TauSpinnerProducer.h"
#include "Artus/KappaAnalysis/interface/KappaProduct.h"

#include "Artus/Utility/interface/DefaultValues.h"

#define NO_BOSON_FOUND -555
#define NO_HIGGS_FOUND -666
#define WEIGHT_NAN -777

void TauSpinnerProducer::Init(setting_type const& settings)
{
	ProducerBase<HttTypes>::Init(settings);
	// interface to TauSpinner
	//Reading the settings from TauSpinnerSettings.json in following order:
	//name of PDF, CMSENE, Ipp, Ipol, nonSM2, nonSMN (see tau_reweight_lib.cxx),
	//boosted/unboosted to Higgs CMF
//	stringvector tauSpinnerSettings = settings.GetTauSpinnerSettings();
	Tauolapp::Tauola::initialize();
	string name = settings.GetTauSpinnerSettingsPDF();
	LHAPDF::initPDFSetByName(name);
	double CmsEnergy = settings.GetTauSpinnerSettingsCmsEnergy();
	bool ipp = settings.GetTauSpinnerSettingsIpp();
	int ipol = settings.GetTauSpinnerSettingsIpol();
	int nonSM2 = settings.GetTauSpinnerSettingsNonSM2();
	int nonSMN = settings.GetTauSpinnerSettingsNonSMN();
	TauSpinner::initialize_spinner(ipp, ipol, nonSM2, nonSMN, CmsEnergy);
}


void TauSpinnerProducer::Produce(event_type const& event, product_type& product,
								 setting_type const& settings) const
{
	LOG_N_TIMES(20, DEBUG) << "TauSpinnerProducer::Produce ";
	std::vector<MotherDaughterBundle> higgs = product.m_genBoson;
	if (higgs.size() == 0)
	{
		product.m_weights["tauSpinnerWeight"] = NO_HIGGS_FOUND;
		return;
	}

	//Conversion to SimpleParticles
	//select the particles to convert from Output of GenTauDecay Producer, which gives the mother boson, two boson daughters,
	//and the granddaughters.
	KGenParticle* selectedHiggs1 = higgs[0].node;

	MotherDaughterBundle selectedTau1 = higgs[0].Daughters[0];
	MotherDaughterBundle selectedTau2 = higgs[0].Daughters[1];

	std::vector<MotherDaughterBundle> selectedTauDaughters1 = higgs[0].Daughters[0].Daughters;
	std::vector<MotherDaughterBundle> selectedTauDaughters2 = higgs[0].Daughters[1].Daughters;
	LOG_N_TIMES(20, DEBUG) << "Higgs PdgId: " << selectedHiggs1->pdgId();

	//Boosting following vectors to the center of mass system of the Higgs, if nessesary: Higgs, Tau1, Tau2 and TauDaughters
	//Information about boosting is read from TauSpinnerSettings.json, seventh entry.
	RMDataLV vec = selectedHiggs1->p4;
	RMDataLV::BetaVector boostvec = vec.BoostToCM();
	ROOT::Math::Boost boostMat(boostvec);
	bool boosted = (settings.GetTauSpinnerSettingsBoost() == true);
	if (boosted)
	{
		selectedHiggs1->p4 = boostMat * (selectedHiggs1->p4);
		selectedTau1.node->p4 = boostMat * (selectedTau1.node->p4);
		selectedTau2.node->p4 = boostMat * (selectedTau2.node->p4);
		for (unsigned int i = 0; i < selectedTauDaughters1.size(); ++i)
		{
			selectedTauDaughters1[i].node->p4 = boostMat * (selectedTauDaughters1[i].node->p4);
		}
		for (unsigned int i = 0; i < selectedTauDaughters2.size(); ++i)
		{
			selectedTauDaughters2[i].node->p4 = boostMat * (selectedTauDaughters2[i].node->p4);
		}
	}

	if (abs(selectedTau1.node->pdgId()) == DefaultValues::pdgIdTau) //TauSpinner considers only Taus and Tau-Neutrinos as daughters of a Boson (Higgs, W etc.)
	{
		LOG_N_TIMES(20, DEBUG) << "		Tau1 PdgId: " << selectedTau1.node->pdgId();
		LOG_N_TIMES(20, DEBUG) << "		Tau2 PdgId: " << selectedTau2.node->pdgId() << std::endl;

		TauSpinner::SimpleParticle X = getSimpleParticle(selectedHiggs1);
		TauSpinner::SimpleParticle tau1 = getSimpleParticle(selectedTau1.node);
		TauSpinner::SimpleParticle tau2 = getSimpleParticle(selectedTau2.node);



		//choosing considered tau decay products for the TauSpinnerWeight calculaton
		//through the entry ChosenTauDaughters in the TauSpinnerSettings.json
		stringvector chosentaudaughters = settings.GetChosenTauDaughters();
		//bool choose = settings.GetChooseTauDaughter();
		std::vector<int> chosentd;
		for (unsigned int i = 0; i < chosentaudaughters.size(); i++)
		{
			chosentd.push_back(0);
			std::istringstream(chosentaudaughters[i]) >> chosentd[i];
		}

		std::vector<TauSpinner::SimpleParticle> tauFinalStates1;
		GetFinalStates(selectedTau1, &tauFinalStates1);
		std::vector<TauSpinner::SimpleParticle> tauFinalStates2;
		GetFinalStates(selectedTau2, &tauFinalStates2);

		// Mass roundoff check: Compare energy between taus and their decay products

		product.m_genMassRoundOff1 = abs(tau1.e() - GetMass(tauFinalStates1));
		product.m_genMassRoundOff2 = abs(tau2.e() - GetMass(tauFinalStates2));


		bool withoutchoise = true;

		// Debug output for testing
		LOG_N_TIMES(20, DEBUG) << selectedHiggs1->p4.px() << std::endl;
		LOG_N_TIMES(20, DEBUG) << selectedTau1.node->p4.px() << std::endl;
		LOG_N_TIMES(20, DEBUG) << selectedTau2.node->p4.px() << std::endl;
		LOG_N_TIMES(20, DEBUG) << selectedTauDaughters1[1].node->p4.px() << std::endl;
		LOG_N_TIMES(20, DEBUG) << selectedTauDaughters2[1].node->p4.px() << std::endl;

		LOG_N_TIMES(20, DEBUG) << tauFinalStates1.size() << "               " << tauFinalStates2.size() << std::endl;


		if (((tauFinalStates1.size() > 0) && (tauFinalStates2.size()) > 0) || withoutchoise)
		{
			//Decision for a certain weight calculation depending on BosonPdgId
			int bosonPdgId = settings.GetBosonPdgId();
			double weight;
			if (abs(bosonPdgId) == DefaultValues::pdgIdW)
				weight = calculateWeightFromParticlesWorHpn(X, tau1, tau2, tauFinalStates1);
			else if (abs(bosonPdgId) == DefaultValues::pdgIdH)
				weight = calculateWeightFromParticlesH(X, tau1, tau2, tauFinalStates1, tauFinalStates2);
			else
				weight = NO_BOSON_FOUND;

			if (weight == weight)
				product.m_weights["tauSpinnerWeight"] = weight;
			else
			{
				// 'Nan' Debug output
				LOG_N_TIMES(20, DEBUG) << "Found a 'NaN' weight with the following particles: " << std::endl;
				LogSimpleParticle(std::string("Higgs"), X);
				LogSimpleParticle(std::string("Tau1"), tau1);
				LogSimpleParticle(std::string("Tau2"), tau2);
				LogSimpleParticle(std::string("Tau1FinalState"), tauFinalStates1);
				LogSimpleParticle(std::string("Tau2FinalState"), tauFinalStates2);
				product.m_weights["tauSpinnerWeight"] = WEIGHT_NAN;
			} // NaN debug output end
		}
		else product.m_weights["tauSpinnerWeight"] = DefaultValues::UndefinedDouble;
	}// "if 1BosonDaughter is Tau"-end.
	else product.m_weights["tauSpinnerWeight"] = DefaultValues::UndefinedDouble;
}


double TauSpinnerProducer::GetMass(std::vector<TauSpinner::SimpleParticle> in) const
{
	double sumEnergy = 0;
	for (unsigned int i = 0; i < in.size(); ++i)
	{
		sumEnergy += in[i].e();
	}
	return sumEnergy;
}

TauSpinner::SimpleParticle TauSpinnerProducer::getSimpleParticle(KGenParticle*& in) const
{
	return TauSpinner::SimpleParticle(in->p4.px(), in->p4.py(), in->p4.pz(), in->p4.e(), in->pdgId());
}


// recursive function to create a vector of final states particles in the way TauSpinner expects it
std::vector<TauSpinner::SimpleParticle>* TauSpinnerProducer::GetFinalStates(MotherDaughterBundle& mother,
		std::vector<TauSpinner::SimpleParticle>* resultVector) const
{
	for (unsigned int i = 0; i < mother.Daughters.size(); ++i)
	{
		if (abs(mother.Daughters[i].node->pdgId()) == DefaultValues::pdgIdPiZero || mother.Daughters[i].finalState)
		{
			resultVector->push_back(getSimpleParticle(mother.Daughters[i].node));
		}
		else
		{
			GetFinalStates(mother.Daughters[i], resultVector);
		}
	}
	return resultVector;
}

void TauSpinnerProducer::LogSimpleParticle(std::string particleName, TauSpinner::SimpleParticle in) const
{
	LOG_N_TIMES(20, DEBUG) << "\n" << particleName << "Px=" << in.px() << "|"
	                       << particleName << "Py=" << in.py() << "|"
	                       << particleName << "Pz=" << in.pz() << "|"
	                       << particleName << "E=" << in.e() << "|"
	                       << particleName << "PdgId=" << in.pdgid() << "|";
}

void TauSpinnerProducer::LogSimpleParticle(std::string particleName, std::vector<TauSpinner::SimpleParticle> in) const
{
	for (unsigned int i = 0; i < in.size(); i++)
	{
		std::ostringstream index;
		index << i + 1;
		LogSimpleParticle(particleName + index.str(), in[i]);
	}
}


