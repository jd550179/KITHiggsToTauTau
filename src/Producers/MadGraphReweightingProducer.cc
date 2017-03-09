
#include <algorithm>
#include <math.h>

#include <boost/format.hpp>

#include "Artus/Utility/interface/DefaultValues.h"
#include "Artus/Utility/interface/SafeMap.h"
#include "Artus/Utility/interface/Utility.h"
#include "Artus/Consumer/interface/LambdaNtupleConsumer.h"

#include "HiggsAnalysis/KITHiggsToTauTau/interface/Producers/MadGraphReweightingProducer.h"


std::string MadGraphReweightingProducer::GetProducerId() const
{
	return "MadGraphReweightingProducer";
}

void MadGraphReweightingProducer::Init(setting_type const& settings)
{
	ProducerBase<HttTypes>::Init(settings);
	
	for (std::vector<std::string>::const_iterator processDirectory = settings.GetMadGraphProcessDirectories().begin();
	     processDirectory != settings.GetMadGraphProcessDirectories().end(); ++processDirectory)
	{
		m_madGraphTools[*processDirectory] = std::map<int, MadGraphTools*>();
		for (std::vector<float>::const_iterator mixingAngleOverPiHalf = settings.GetMadGraphMixingAnglesOverPiHalf().begin();
		     mixingAngleOverPiHalf != settings.GetMadGraphMixingAnglesOverPiHalf().end(); ++mixingAngleOverPiHalf)
		{
			MadGraphTools* madGraphTools = new MadGraphTools(*mixingAngleOverPiHalf, *processDirectory, settings.GetMadGraphParamCard(), 0.118);
			m_madGraphTools[*processDirectory][GetMixingAngleKey(*mixingAngleOverPiHalf)] = madGraphTools;
		}
	}
	
	// quantities for LambdaNtupleConsumer
	for (std::vector<float>::const_iterator mixingAngleOverPiHalfIt = settings.GetMadGraphMixingAnglesOverPiHalf().begin();
	     mixingAngleOverPiHalfIt != settings.GetMadGraphMixingAnglesOverPiHalf().end();
	     ++mixingAngleOverPiHalfIt)
	{
		float mixingAngleOverPiHalf = *mixingAngleOverPiHalfIt;
		std::string mixingAngleOverPiHalfLabel = GetLabelForWeightsMap(mixingAngleOverPiHalf);
		LambdaNtupleConsumer<HttTypes>::AddFloatQuantity(mixingAngleOverPiHalfLabel, [mixingAngleOverPiHalfLabel](event_type const& event, product_type const& product)
		{
			return SafeMap::GetWithDefault(product.m_optionalWeights, mixingAngleOverPiHalfLabel, 0.0);
		});
	}
	
	if (settings.GetMadGraphMixingAnglesOverPiHalfSample() >= 0.0)
	{
		float mixingAngleOverPiHalfSample = settings.GetMadGraphMixingAnglesOverPiHalfSample();
		
		// if mixing angle for curent sample is defined, it has to be in the list MadGraphMixingAnglesOverPiHalf
		assert(Utility::Contains(settings.GetMadGraphMixingAnglesOverPiHalf(), mixingAngleOverPiHalfSample));
		
		std::string mixingAngleOverPiHalfSampleLabel = GetLabelForWeightsMap(mixingAngleOverPiHalfSample);
		LambdaNtupleConsumer<HttTypes>::AddFloatQuantity("madGraphWeightSample", [mixingAngleOverPiHalfSampleLabel](event_type const& event, product_type const& product)
		{
			return SafeMap::GetWithDefault(product.m_optionalWeights, mixingAngleOverPiHalfSampleLabel, 0.0);
		});
		LambdaNtupleConsumer<HttTypes>::AddFloatQuantity("madGraphWeightInvSample", [mixingAngleOverPiHalfSampleLabel](event_type const& event, product_type const& product)
		{
			double weight = SafeMap::GetWithDefault(product.m_optionalWeights, mixingAngleOverPiHalfSampleLabel, 0.0);
			//return std::min(((weight > 0.0) ? (1.0 / weight) : 0.0), 10.0);   // no physics reason for this
			return ((weight > 0.0) ? (1.0 / weight) : 0.0);
		});
	}
}


void MadGraphReweightingProducer::Produce(event_type const& event, product_type& product,
                                          setting_type const& settings) const
{
	// TODO: should this be an assertion (including a filter to run before this producer)?
	if ((product.m_genBosonParticle != nullptr) &&
	    (product.m_genParticlesProducingBoson.size() > 1) &&
	    (product.m_genParticlesProducingBoson.at(0)->pdgId == DefaultValues::pdgIdGluon) &&
	    (product.m_genParticlesProducingBoson.at(1)->pdgId == DefaultValues::pdgIdGluon))
	{
		std::vector<RMFLV*> particleFourMomenta;
		particleFourMomenta.push_back(&(product.m_genParticlesProducingBoson.at(0)->p4)); // gluon 1
		particleFourMomenta.push_back(&(product.m_genParticlesProducingBoson.at(1)->p4)); // gluon 2
		particleFourMomenta.push_back(&(product.m_genBosonParticle->p4)); // Higgs
		
		// process ggH
		std::string madGraphProcessDirectory = settings.GetMadGraphProcessDirectories()[0]; //dunno if this is right
		std::map<int, MadGraphTools*>* tmpMadGraphToolsMap = const_cast<std::map<int, MadGraphTools*>*>(&(SafeMap::Get(m_madGraphTools, madGraphProcessDirectory)));
		
		// calculate the matrix elements for different mixing angles
		for (std::vector<float>::const_iterator mixingAngleOverPiHalf = settings.GetMadGraphMixingAnglesOverPiHalf().begin();
		     mixingAngleOverPiHalf != settings.GetMadGraphMixingAnglesOverPiHalf().end(); ++mixingAngleOverPiHalf)
		{
			MadGraphTools* tmpMadGraphTools = SafeMap::Get(*tmpMadGraphToolsMap, GetMixingAngleKey(*mixingAngleOverPiHalf));
			product.m_optionalWeights[GetLabelForWeightsMap(*mixingAngleOverPiHalf)] = tmpMadGraphTools->GetMatrixElementSquared(particleFourMomenta);
			//LOG(DEBUG) << *mixingAngleOverPiHalf << " --> " << product.m_optionalWeights[GetLabelForWeightsMap(*mixingAngleOverPiHalf)];
		}
	}
}

int MadGraphReweightingProducer::GetMixingAngleKey(float mixingAngleOverPiHalf) const
{
	return int(mixingAngleOverPiHalf * 100.0);
}

std::string MadGraphReweightingProducer::GetLabelForWeightsMap(float mixingAngleOverPiHalf) const
{
	return ("madGraphWeight" + str(boost::format("%03d") % (mixingAngleOverPiHalf * 100.0)));
}
