
#pragma once

#include <boost/regex.hpp>

#include "Kappa/DataFormats/interface/Kappa.h"

#include "Artus/Core/interface/ProducerBase.h"
#include "Artus/Consumer/interface/LambdaNtupleConsumer.h"
#include "Artus/Utility/interface/Utility.h"

#include "HiggsAnalysis/KITHiggsToTauTau/interface/HttTypes.h"

/**
   \brief Producers for candidates of di-tau pairs
*/


template<class TLepton1, class TLepton2>
class ValidDiTauPairCandidatesProducerBase: public ProducerBase<HttTypes>
{
public:

	typedef typename HttTypes::event_type event_type;
	typedef typename HttTypes::product_type product_type;
	typedef typename HttTypes::setting_type setting_type;
	
	ValidDiTauPairCandidatesProducerBase(std::vector<TLepton1*> product_type::*validLeptonsMember1,
	                                     std::vector<TLepton2*> product_type::*validLeptonsMember2) :
		ProducerBase<HttTypes>(),
		m_validLeptonsMember1(validLeptonsMember1),
		m_validLeptonsMember2(validLeptonsMember2)
	{
	}

	virtual void Init(setting_type const& settings) override
	{
		ProducerBase<HttTypes>::Init(settings);
		
		// configurations possible:
		// "cut" --> applied to all pairs
		// "<index in setting HltPaths>:<cut>" --> applied to pairs that fired and matched ONLY the indexed HLT path
		// "<HLT path regex>:<cut>" --> applied to pairs that fired and matched ONLY the given HLT path
		m_lepton1LowerPtCutsByIndex = Utility::ParseMapTypes<size_t, float>(
				Utility::ParseVectorToMap(settings.GetDiTauPairLepton1LowerPtCuts()), m_lepton1LowerPtCutsByHltName
		);
		m_lepton2LowerPtCutsByIndex = Utility::ParseMapTypes<size_t, float>(
				Utility::ParseVectorToMap(settings.GetDiTauPairLepton2LowerPtCuts()), m_lepton2LowerPtCutsByHltName
		);
		m_hltFiredBranchNames = Utility::ParseVectorToMap(settings.GetHLTBranchNames());
		std::map<std::string, std::vector<std::string> > hltUsingOnlyFirstLeptonPerBranchNames = Utility::ParseVectorToMap(settings.GetUsingOnlyFirstLeptonPerHLTBranchNames());
		
		// add possible quantities for the lambda ntuples consumers
		LambdaNtupleConsumer<HttTypes>::AddIntQuantity("nDiTauPairCandidates", [](event_type const& event, product_type const& product)
		{
			return static_cast<int>(product.m_validDiTauPairCandidates.size());
		});
		LambdaNtupleConsumer<HttTypes>::AddIntQuantity("nAllDiTauPairCandidates", [](event_type const& event, product_type const& product)
		{
			return static_cast<int>(product.m_validDiTauPairCandidates.size()+product.m_invalidDiTauPairCandidates.size());
		});
		std::vector<std::string> hltPathsWithoutCommonMatch = settings.GetDiTauPairHltPathsWithoutCommonMatchRequired();
		// debug output in initialization step
		/*
		for(unsigned int i = 0; i < hltPathsWithoutCommonMatch.size(); i++)
		{
			LOG(DEBUG) << "Trigger without common match: "  << hltPathsWithoutCommonMatch.at(i) << std::endl;
		}
		LOG(DEBUG) << "Settings for Lepton 1 Pt Cuts: " << std::endl;
		for(unsigned int i = 0; i < settings.GetDiTauPairLepton1LowerPtCuts().size(); i++)
		{
			LOG(DEBUG) << settings.GetDiTauPairLepton1LowerPtCuts().at(i) << std::endl;
		}
		LOG(DEBUG) << "Settings for Lepton 2 Pt Cuts: " << std::endl;
		for(unsigned int i = 0; i < settings.GetDiTauPairLepton2LowerPtCuts().size(); i++)
		{
			LOG(DEBUG) << settings.GetDiTauPairLepton2LowerPtCuts().at(i) << std::endl;
		}
		LOG(DEBUG) << "Amount of lepton 1 Pt Cuts by Hlt Name: " << m_lepton1LowerPtCutsByHltName.size() << std::endl;
		LOG(DEBUG) << "Amount of lepton 2 Pt Cuts by Hlt Name: " << m_lepton2LowerPtCutsByHltName.size() << std::endl;
		*/
		for(auto hltNames: m_hltFiredBranchNames)
		{
			std::map<std::string, std::vector<float>> lepton1LowerPtCutsByHltName = m_lepton1LowerPtCutsByHltName;
			std::map<std::string, std::vector<float>> lepton2LowerPtCutsByHltName = m_lepton2LowerPtCutsByHltName;
			LambdaNtupleConsumer<HttTypes>::AddBoolQuantity(hltNames.first, [hltUsingOnlyFirstLeptonPerBranchNames , hltNames, hltPathsWithoutCommonMatch, lepton1LowerPtCutsByHltName, lepton2LowerPtCutsByHltName](event_type const& event, product_type const& product)
			{
				bool diTauPairFiredTrigger = false;
				//LOG(DEBUG) << "Beginning of lambda function for " << hltNames.first << std::endl;
				for (auto hltName: hltNames.second)
				{
					if(std::find(hltPathsWithoutCommonMatch.begin(), hltPathsWithoutCommonMatch.end(), hltName) == hltPathsWithoutCommonMatch.end())
					{
						//LOG(DEBUG) << "Common match required for " << hltName << std::endl;
						// we do require a common match, check, whether both leptons are matched to trigger objects.
						if (product.m_detailedTriggerMatchedLeptons.find(static_cast<KLepton*>(product.m_validDiTauPairCandidates.at(0).first)) != product.m_detailedTriggerMatchedLeptons.end() &&
						    product.m_detailedTriggerMatchedLeptons.find(static_cast<KLepton*>(product.m_validDiTauPairCandidates.at(0).second)) != product.m_detailedTriggerMatchedLeptons.end())
						{
							auto trigger1 = product.m_detailedTriggerMatchedLeptons.at(static_cast<KLepton*>(product.m_validDiTauPairCandidates.at(0).first));
							auto trigger2 = product.m_detailedTriggerMatchedLeptons.at(static_cast<KLepton*>(product.m_validDiTauPairCandidates.at(0).second));
							bool hltFired1 = false;
							bool hltFired2 = false;
							for (auto hlts: (*trigger1))
							{
								if (boost::regex_search(hlts.first, boost::regex(hltName, boost::regex::icase | boost::regex::extended)))
								{
									for (auto matchedObjects: hlts.second)
									{
										if (matchedObjects.second.size() > 0) hltFired1 = true;
									}
								}
							}
							//LOG(DEBUG) << "Found trigger for the lepton 1? " << hltFired1 << std::endl;
							for (auto hlts: (*trigger2))
							{
								if (boost::regex_search(hlts.first, boost::regex(hltName, boost::regex::icase | boost::regex::extended)))
								{
									for (auto matchedObjects: hlts.second)
									{
										if (matchedObjects.second.size() > 0) hltFired2 = true;
									}
								}
							}
							//LOG(DEBUG) << "Found trigger for the lepton 2? " << hltFired1 << std::endl;
							bool hltFired = hltFired1 && hltFired2;
							// passing kinematic cuts for trigger 
							if (lepton1LowerPtCutsByHltName.find(hltName) != lepton1LowerPtCutsByHltName.end())
							{
								hltFired = hltFired &&
										(product.m_validDiTauPairCandidates.at(0).first->p4.Pt() > *std::max_element(lepton1LowerPtCutsByHltName.at(hltName).begin(), lepton1LowerPtCutsByHltName.at(hltName).end()));
								//LOG(DEBUG) << "lepton 1 Pt: " << product.m_validDiTauPairCandidates.at(0).first->p4.Pt() << " threshold: " << *std::max_element(lepton1LowerPtCutsByHltName.at(hltName).begin(), lepton1LowerPtCutsByHltName.at(hltName).end()) <<  std::endl;
							}
							if (lepton2LowerPtCutsByHltName.find(hltName) != lepton2LowerPtCutsByHltName.end())
							{
								hltFired = hltFired &&
									(product.m_validDiTauPairCandidates.at(0).second->p4.Pt() > *std::max_element(lepton2LowerPtCutsByHltName.at(hltName).begin(), lepton2LowerPtCutsByHltName.at(hltName).end()));
								//LOG(DEBUG) << "lepton 2 Pt: " << product.m_validDiTauPairCandidates.at(0).second->p4.Pt() << " threshold: " << *std::max_element(lepton2LowerPtCutsByHltName.at(hltName).begin(), lepton2LowerPtCutsByHltName.at(hltName).end()) <<  std::endl;
							}
							//LOG(DEBUG) << "Both leptons passed kinematic cuts? " << hltFired  << std::endl;
							diTauPairFiredTrigger = diTauPairFiredTrigger || hltFired;
						}
					}
					else
					{
						//LOG(DEBUG) << "Common match NOT required for " << hltName << std::endl;
						// we do not require a common match, check the matching only for the one of the leptons.
						bool useOnlyFirstLepton = true;
                                                if (hltUsingOnlyFirstLeptonPerBranchNames.find(hltNames.first) != hltUsingOnlyFirstLeptonPerBranchNames.end())
                                                {
                                                    useOnlyFirstLepton = bool(std::stoi(hltUsingOnlyFirstLeptonPerBranchNames.at(hltNames.first).at(0)));
                                                }
						// check only first lepton
						if (useOnlyFirstLepton)
                                                {
                                                        if (product.m_detailedTriggerMatchedLeptons.find(static_cast<KLepton*>(product.m_validDiTauPairCandidates.at(0).first)) != product.m_detailedTriggerMatchedLeptons.end())
                                                        {
                                                                auto trigger = product.m_detailedTriggerMatchedLeptons.at(static_cast<KLepton*>(product.m_validDiTauPairCandidates.at(0).first));
                                                                bool hltFired = false;
                                                                for (auto hlts: (*trigger))
                                                                {
                                                                        if (boost::regex_search(hlts.first, boost::regex(hltName, boost::regex::icase | boost::regex::extended)))
                                                                        {
                                                                                for (auto matchedObjects: hlts.second)
                                                                                {
                                                                                        if (matchedObjects.second.size() > 0) hltFired = true;
                                                                                }
                                                                        }
                                                                }
                                                                //LOG(DEBUG) << "Found trigger for the lepton 1? " << hltFired << std::endl;
                                                                // passing kinematic cuts for trigger
                                                                if (lepton1LowerPtCutsByHltName.find(hltName) != lepton1LowerPtCutsByHltName.end())
                                                                {
                                                                        hltFired = hltFired &&
                                                                                        (product.m_validDiTauPairCandidates.at(0).first->p4.Pt() > *std::max_element(lepton1LowerPtCutsByHltName.at(hltName).begin(), lepton1LowerPtCutsByHltName.at(hltName).end()));
                                                                        //LOG(DEBUG) << "lepton 1 Pt: " << product.m_validDiTauPairCandidates.at(0).first->p4.Pt() << " threshold: " << *std::max_element(lepton1LowerPtCutsByHltName.at(hltName).begin(), lepton1LowerPtCutsByHltName.at(hltName).end()) <<  std::endl;
                                                                }
                                                                if (lepton2LowerPtCutsByHltName.find(hltName) != lepton2LowerPtCutsByHltName.end())
                                                                {
                                                                        hltFired = hltFired &&
                                                                                (product.m_validDiTauPairCandidates.at(0).second->p4.Pt() > *std::max_element(lepton2LowerPtCutsByHltName.at(hltName).begin(), lepton2LowerPtCutsByHltName.at(hltName).end()));
                                                                        //LOG(DEBUG) << "lepton 2 Pt: " << product.m_validDiTauPairCandidates.at(0).second->p4.Pt() << " threshold: " << *std::max_element(lepton2LowerPtCutsByHltName.at(hltName).begin(), lepton2LowerPtCutsByHltName.at(hltName).end()) <<  std::endl;
                                                                }
                                                                //LOG(DEBUG) << "Both leptons passed kinematic cuts? " << hltFired << std::endl;
                                                                diTauPairFiredTrigger = diTauPairFiredTrigger || hltFired;
                                                        }
                                                }
						// check only second lepton
                                                else
                                                {
                                                        if (product.m_detailedTriggerMatchedLeptons.find(static_cast<KLepton*>(product.m_validDiTauPairCandidates.at(0).second)) != product.m_detailedTriggerMatchedLeptons.end())
                                                        {
                                                                auto trigger = product.m_detailedTriggerMatchedLeptons.at(static_cast<KLepton*>(product.m_validDiTauPairCandidates.at(0).second));
                                                                bool hltFired = false;
                                                                for (auto hlts: (*trigger))
                                                                {
                                                                        if (boost::regex_search(hlts.first, boost::regex(hltName, boost::regex::icase | boost::regex::extended)))
                                                                        {
                                                                                for (auto matchedObjects: hlts.second)
                                                                                {
                                                                                        if (matchedObjects.second.size() > 0) hltFired = true;
                                                                                }
                                                                        }
                                                                }
                                                                //LOG(DEBUG) << "Found trigger for the lepton 2? " << hltFired << std::endl;
                                                                // passing kinematic cuts for trigger
                                                                if (lepton1LowerPtCutsByHltName.find(hltName) != lepton1LowerPtCutsByHltName.end())
                                                                {
                                                                        hltFired = hltFired &&
                                                                                        (product.m_validDiTauPairCandidates.at(0).first->p4.Pt() > *std::max_element(lepton1LowerPtCutsByHltName.at(hltName).begin(), lepton1LowerPtCutsByHltName.at(hltName).end()));
                                                                        //LOG(DEBUG) << "lepton 1 Pt: " << product.m_validDiTauPairCandidates.at(0).first->p4.Pt() << " threshold: " << *std::max_element(lepton1LowerPtCutsByHltName.at(hltName).begin(), lepton1LowerPtCutsByHltName.at(hltName).end()) <<  std::endl;
                                                                }
                                                                if (lepton2LowerPtCutsByHltName.find(hltName) != lepton2LowerPtCutsByHltName.end())
                                                                {
                                                                        hltFired = hltFired &&
                                                                                (product.m_validDiTauPairCandidates.at(0).second->p4.Pt() > *std::max_element(lepton2LowerPtCutsByHltName.at(hltName).begin(), lepton2LowerPtCutsByHltName.at(hltName).end()));
                                                                        //LOG(DEBUG) << "lepton 2 Pt: " << product.m_validDiTauPairCandidates.at(0).second->p4.Pt() << " threshold: " << *std::max_element(lepton2LowerPtCutsByHltName.at(hltName).begin(), lepton2LowerPtCutsByHltName.at(hltName).end()) <<  std::endl;
                                                                }
                                                                //LOG(DEBUG) << "Both leptons passed kinematic cuts? " << hltFired << std::endl;
                                                                diTauPairFiredTrigger = diTauPairFiredTrigger || hltFired;
                                                        }
                                                }
					}
				}
				//LOG(DEBUG) << "Tau pair with valid trigger match? " << diTauPairFiredTrigger << std::endl << std::endl;
				return diTauPairFiredTrigger;
			});
		}
	}
	
	virtual void Produce(event_type const& event, product_type & product, 
	                     setting_type const& settings) const override
	{
		product.m_validDiTauPairCandidates.clear();
		//LOG(DEBUG) << "ValidDiTauPairCandidatesProducer processing run:lumi:event " << event.m_eventInfo->nRun << ":" << event.m_eventInfo->nLumi << ":" << event.m_eventInfo->nEvent << std::endl; 
		
		// build pairs for all combinations
		for (typename std::vector<TLepton1*>::iterator lepton1 = (product.*m_validLeptonsMember1).begin();
		     lepton1 != (product.*m_validLeptonsMember1).end(); ++lepton1)
		{
			for (typename std::vector<TLepton2*>::iterator lepton2 = (product.*m_validLeptonsMember2).begin();
			     lepton2 != (product.*m_validLeptonsMember2).end(); ++lepton2)
			{
				DiTauPair diTauPair(*lepton1, *lepton2);
				
				// pair selections
				bool validDiTauPair = true;

				// OS charge requirement
				//validDiTauPair = validDiTauPair && diTauPair.IsOppositelyCharged();

				// delta R cut
				validDiTauPair = validDiTauPair && ((settings.GetDiTauPairMinDeltaRCut() < 0.0) || (diTauPair.GetDeltaR() > static_cast<double>(settings.GetDiTauPairMinDeltaRCut())));
				//LOG(DEBUG) << "Pair passed the delta R cut of " << settings.GetDiTauPairMinDeltaRCut() << "? " << validDiTauPair <<  ", because computed value is " << diTauPair.GetDeltaR() << std::endl;

				// require matchings with the same triggers
				if ((! settings.GetDiTauPairNoHLT()) && (! settings.GetDiTauPairHLTLast()))
				{
				//LOG(DEBUG) << "HLT required, but applied afterwards" << std::endl;
					std::vector<std::string> commonHltPaths = diTauPair.GetCommonHltPaths(product.m_detailedTriggerMatchedLeptons, settings.GetDiTauPairHltPathsWithoutCommonMatchRequired());
					validDiTauPair = validDiTauPair && (commonHltPaths.size() > 0);

					// pt cuts in case one or more HLT paths are matched
					if (validDiTauPair)
					{
						//vector to hold the results for the individual HLTPaths, all results default to true
						std::vector<bool> hltValidDiTauPair(commonHltPaths.size(), true);
						for (std::vector<std::string>::size_type hltPathNumber = 0; hltPathNumber != commonHltPaths.size(); ++hltPathNumber)
						{
							// lepton 1
							for (std::map<size_t, std::vector<float> >::const_iterator lowerPtCutByIndex = m_lepton1LowerPtCutsByIndex.begin();
								lowerPtCutByIndex != m_lepton1LowerPtCutsByIndex.end() && hltValidDiTauPair.at(hltPathNumber); ++lowerPtCutByIndex)
							{
								if ((diTauPair.first->p4.Pt() <= *std::max_element(lowerPtCutByIndex->second.begin(), lowerPtCutByIndex->second.end())) &&
									boost::regex_search(commonHltPaths.at(hltPathNumber), boost::regex(settings.GetHltPaths().at(lowerPtCutByIndex->first), boost::regex::icase | boost::regex::extended)))
								{
									hltValidDiTauPair.at(hltPathNumber) = false;
								}
							}

							// lepton 1
							for (std::map<std::string, std::vector<float> >::const_iterator lowerPtCutByHltName = m_lepton1LowerPtCutsByHltName.begin();
								lowerPtCutByHltName != m_lepton1LowerPtCutsByHltName.end() && hltValidDiTauPair.at(hltPathNumber); ++lowerPtCutByHltName)
							{
								if ((diTauPair.first->p4.Pt() <= *std::max_element(lowerPtCutByHltName->second.begin(), lowerPtCutByHltName->second.end())) &&
									boost::regex_search(commonHltPaths.at(hltPathNumber), boost::regex(lowerPtCutByHltName->first, boost::regex::icase | boost::regex::extended)))
								{
									hltValidDiTauPair.at(hltPathNumber) = false;
								}
							}

							// lepton 2
							for (std::map<size_t, std::vector<float> >::const_iterator lowerPtCutByIndex = m_lepton2LowerPtCutsByIndex.begin();
								lowerPtCutByIndex != m_lepton2LowerPtCutsByIndex.end() && hltValidDiTauPair.at(hltPathNumber); ++lowerPtCutByIndex)
							{
								if ((diTauPair.second->p4.Pt() <= *std::max_element(lowerPtCutByIndex->second.begin(), lowerPtCutByIndex->second.end())) &&
									boost::regex_search(commonHltPaths.at(hltPathNumber), boost::regex(settings.GetHltPaths().at(lowerPtCutByIndex->first), boost::regex::icase | boost::regex::extended)))
								{
									hltValidDiTauPair.at(hltPathNumber) = false;
								}
							}

							// lepton 2
							for (std::map<std::string, std::vector<float> >::const_iterator lowerPtCutByHltName = m_lepton2LowerPtCutsByHltName.begin();
							lowerPtCutByHltName != m_lepton2LowerPtCutsByHltName.end() && hltValidDiTauPair.at(hltPathNumber); ++lowerPtCutByHltName)
							{
								if ((diTauPair.second->p4.Pt() <= *std::max_element(lowerPtCutByHltName->second.begin(), lowerPtCutByHltName->second.end())) &&
									boost::regex_search(commonHltPaths.at(hltPathNumber), boost::regex(lowerPtCutByHltName->first, boost::regex::icase | boost::regex::extended)))
								{
									hltValidDiTauPair.at(hltPathNumber) = false;
								}
							}
						}
						//default validity of ditaupair to false and set it to true in case it is a valid pair for at least one HLTPath
						validDiTauPair = false;
						for (std::vector<bool>::const_iterator hltPathResult = hltValidDiTauPair.begin(); hltPathResult != hltValidDiTauPair.end(); ++hltPathResult)
						{
							if(*hltPathResult)
							{
								validDiTauPair = true;
								break;
							}
						}
						if (settings.GetRequireFirstTriggering())
						{
							bool hltFired = false;
							auto trigger = product.m_detailedTriggerMatchedLeptons[static_cast<KLepton*>(diTauPair.first)];
							for (auto hltName : settings.GetHltPaths())
							{
								for (auto hlts: (*trigger))
								{
									if (boost::regex_search(hlts.first, boost::regex(hltName, boost::regex::icase | boost::regex::extended)))
									{
										for (auto matchedObjects: hlts.second)
										{
											if (matchedObjects.second.size() > 0) hltFired = true;
										}
									}
								}
							}
							validDiTauPair = validDiTauPair && hltFired;
							validDiTauPair = validDiTauPair && (diTauPair.first->p4.Pt() >= diTauPair.second->p4.Pt());
						}
					}
				}
				else if(!settings.GetDiTauPairHLTLast())// will hopefully become obsolete towards the end of 2016 when the trigger is included in simulation
				{
				//LOG(DEBUG) << "HLT NOT required" << std::endl;
					if (validDiTauPair)
					{
						// lepton 1
						for (std::map<size_t, std::vector<float> >::const_iterator lowerPtCutByIndex = m_lepton1LowerPtCutsByIndex.begin();
							lowerPtCutByIndex != m_lepton1LowerPtCutsByIndex.end(); ++lowerPtCutByIndex)
						{
							if (diTauPair.first->p4.Pt() <= *std::max_element(lowerPtCutByIndex->second.begin(), lowerPtCutByIndex->second.end()))
							{
								validDiTauPair = false;
							}
						}

						// lepton 1
						for (std::map<std::string, std::vector<float> >::const_iterator lowerPtCutByHltName = m_lepton1LowerPtCutsByHltName.begin();
							lowerPtCutByHltName != m_lepton1LowerPtCutsByHltName.end(); ++lowerPtCutByHltName)
						{
							if (diTauPair.first->p4.Pt() <= *std::max_element(lowerPtCutByHltName->second.begin(), lowerPtCutByHltName->second.end()))
							{
								validDiTauPair = false;
							}
						}

						// lepton 2
						for (std::map<size_t, std::vector<float> >::const_iterator lowerPtCutByIndex = m_lepton2LowerPtCutsByIndex.begin();
							lowerPtCutByIndex != m_lepton2LowerPtCutsByIndex.end(); ++lowerPtCutByIndex)
						{
							if (diTauPair.second->p4.Pt() <= *std::max_element(lowerPtCutByIndex->second.begin(), lowerPtCutByIndex->second.end()))
							{
								validDiTauPair = false;
							}
						}

						// lepton 2
						for (std::map<std::string, std::vector<float> >::const_iterator lowerPtCutByHltName = m_lepton2LowerPtCutsByHltName.begin();
						lowerPtCutByHltName != m_lepton2LowerPtCutsByHltName.end(); ++lowerPtCutByHltName)
						{
							if (diTauPair.second->p4.Pt() <= *std::max_element(lowerPtCutByHltName->second.begin(), lowerPtCutByHltName->second.end()))
							{
								validDiTauPair = false;
							}
						}
					}
					// require at least one of the leptons to pass a higher pt threshold. this is needed for double-lepton or cross triggers
					validDiTauPair = validDiTauPair && (diTauPair.first->p4.Pt() > settings.GetLowerCutHardLepPt() || diTauPair.second->p4.Pt() > settings.GetLowerCutHardLepPt());
				}
				// check possible additional criteria from subclasses
				validDiTauPair = validDiTauPair && AdditionalCriteria(diTauPair, event, product, settings);
				//LOG(DEBUG) << "Pair passed additional Criteria? " << validDiTauPair << std::endl;
				if (validDiTauPair)
				{
					product.m_validDiTauPairCandidates.push_back(diTauPair);
				}
				else
				{
					product.m_invalidDiTauPairCandidates.push_back(diTauPair);
				}
			}
		}
		// sort pairs
		std::sort(product.m_validDiTauPairCandidates.begin(), product.m_validDiTauPairCandidates.end(),
		          DiTauPairIsoPtComparator(&(product.m_leptonIsolationOverPt), settings.GetDiTauPairIsTauIsoMVA()));
		std::sort(product.m_invalidDiTauPairCandidates.begin(), product.m_invalidDiTauPairCandidates.end(),
		          DiTauPairIsoPtComparator(&(product.m_leptonIsolationOverPt), settings.GetDiTauPairIsTauIsoMVA()));
		// another debug output at the end: which pair is selected?
		/*
		// Loop over valid diTauPairs with deltaR:
		for(unsigned int i=0;i<product.m_validDiTauPairCandidates.size();i++)
		{
				//LOG(DEBUG) << "Delta R separation within valid Pair:" << product.m_validDiTauPairCandidates.at(i).GetDeltaR() << std::endl;
		}
		if(product.m_validDiTauPairCandidates.size()>0)
		{
			//LOG(DEBUG) << "Pt of leading Tau Candidate in first pair: " << product.m_validDiTauPairCandidates.at(0).first->p4.Pt() << std::endl;
			//LOG(DEBUG) << "Pt of traling Tau Candidate in first pair: " << product.m_validDiTauPairCandidates.at(0).second->p4.Pt() << std::endl;
		}
		*/
	}


protected:
	// Can be overwritten for special use cases
	virtual bool AdditionalCriteria(DiTauPair const& diTauPair, event_type const& event,
	                                product_type& product, setting_type const& settings) const
	{
		bool validDiTauPair = true;
		return validDiTauPair;
	}


private:
	std::vector<TLepton1*> product_type::*m_validLeptonsMember1;
	std::vector<TLepton2*> product_type::*m_validLeptonsMember2;

	std::map<size_t, std::vector<float> > m_lepton1LowerPtCutsByIndex;
	std::map<std::string, std::vector<float> > m_lepton1LowerPtCutsByHltName;
	std::map<size_t, std::vector<float> > m_lepton2LowerPtCutsByIndex;
	std::map<std::string, std::vector<float> > m_lepton2LowerPtCutsByHltName;
	std::map<std::string, std::vector<std::string> > m_hltFiredBranchNames;

};


class ValidTTPairCandidatesProducer: public ValidDiTauPairCandidatesProducerBase<KTau, KTau>
{
public:
	ValidTTPairCandidatesProducer();
	virtual std::string GetProducerId() const override;
	virtual bool AdditionalCriteria(DiTauPair const&, event_type const&,
	                                product_type&, setting_type const&) const override;
};

class ValidMTPairCandidatesProducer: public ValidDiTauPairCandidatesProducerBase<KMuon, KTau>
{
public:
	ValidMTPairCandidatesProducer();
	virtual std::string GetProducerId() const override;
};

class ValidETPairCandidatesProducer: public ValidDiTauPairCandidatesProducerBase<KElectron, KTau>
{
public:
	ValidETPairCandidatesProducer();
	virtual std::string GetProducerId() const override;
};

class ValidEMPairCandidatesProducer: public ValidDiTauPairCandidatesProducerBase<KMuon, KElectron>
{
public:
	ValidEMPairCandidatesProducer();
	virtual std::string GetProducerId() const override;
};

class ValidMMPairCandidatesProducer: public ValidDiTauPairCandidatesProducerBase<KMuon, KMuon>
{
public:
	ValidMMPairCandidatesProducer();
	virtual std::string GetProducerId() const override;
	virtual bool AdditionalCriteria(DiTauPair const&, event_type const&,
	                                product_type&, setting_type const&) const override;
};

class ValidEEPairCandidatesProducer: public ValidDiTauPairCandidatesProducerBase<KElectron, KElectron>
{
public:
	ValidEEPairCandidatesProducer();
	virtual std::string GetProducerId() const override;
	virtual bool AdditionalCriteria(DiTauPair const&, event_type const&,
	                                product_type&, setting_type const&) const override;
};

