#include <vector>
#include <algorithm>
#include <iterator>
#include "CoveredReferenceFeature.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{

int CoveredReferenceState::Compare(const FFState& other) const
{
  const CoveredReferenceState &otherState = static_cast<const CoveredReferenceState&>(other);

  if (m_coveredRef.size() != otherState.m_coveredRef.size()) {
    return (m_coveredRef.size() < otherState.m_coveredRef.size()) ? -1 : +1;  
  } else {
    multiset<string>::const_iterator thisIt, otherIt;
    for (thisIt = m_coveredRef.begin(), otherIt = otherState.m_coveredRef.begin();
        thisIt != m_coveredRef.end(); 
        thisIt++, otherIt++) {
      if (*thisIt != *otherIt) return thisIt->compare(*otherIt);
    }
  }
  return 0;

//  return m_coveredRef == otherState.m_coveredRef;
  
//  if (m_coveredRef == otherState.m_coveredRef)
//    return 0;
//  return (m_coveredRef.size() < otherState.m_coveredRef.size()) ? -1 : +1;
}

void CoveredReferenceFeature::Evaluate(const Phrase &source
                                  , const TargetPhrase &targetPhrase
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection &estimatedFutureScore) const
{}

void CoveredReferenceFeature::Evaluate(const InputType &input
                                  , const InputPath &inputPath
                                  , const TargetPhrase &targetPhrase
                                  , ScoreComponentCollection &scoreBreakdown) const
{}

void CoveredReferenceFeature::Load() {
  InputFileStream refFile(m_path);
  std::string line;
  const StaticData &staticData = StaticData::Instance();
  long sentenceID = staticData.GetStartTranslationId();
  while (getline(refFile, line)) {
    vector<string> words = Tokenize(line, " ");
    multiset<string> wordSet;
    // TODO make Tokenize work with other containers than vector
    copy(words.begin(), words.end(), inserter(wordSet, wordSet.begin())); 
    m_refs.insert(make_pair(sentenceID++, wordSet));
  }
}

void CoveredReferenceFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_path = value;
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* CoveredReferenceFeature::Evaluate(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  const CoveredReferenceState &prev = static_cast<const CoveredReferenceState&>(*prev_state);
  CoveredReferenceState *ret = new CoveredReferenceState(prev);

  const Manager &mgr = cur_hypo.GetManager();
  const InputType &input = mgr.GetSource();
  long id = input.GetTranslationId();

  // which words from the reference remain uncovered
  multiset<string> remaining;
  boost::unordered_map<long, std::multiset<string> >::const_iterator refIt = m_refs.find(id);
  set_difference(refIt->second.begin(), refIt->second.end(),
      ret->m_coveredRef.begin(), ret->m_coveredRef.end(),
      inserter(remaining, remaining.begin()));

  // which of the remaining words are present in the current phrase
  multiset<string> wordsInPhrase = GetWordsInPhrase(cur_hypo.GetCurrTargetPhrase());
  multiset<string> newCovered;
  set_intersection(wordsInPhrase.begin(), wordsInPhrase.end(),
      remaining.begin(), remaining.end(),
      inserter(newCovered, newCovered.begin()));

  // score is just the count of newly-covered words
  vector<float> scores;
  scores.push_back(newCovered.size());
  accumulator->PlusEquals(this, scores);

  // update feature state
  multiset<string>::const_iterator newCoveredIt;
  for (newCoveredIt = newCovered.begin(); newCoveredIt != newCovered.end(); newCoveredIt++) {
    ret->m_coveredRef.insert(*newCoveredIt);
  }
  return ret;
}

FFState* CoveredReferenceFeature::EvaluateChart(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  UTIL_THROW(util::Exception, "Not implemented");
}

}