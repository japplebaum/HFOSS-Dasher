// LanguageModel.h
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2001-2005 David Ward
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __LanguageModelling_LanguageModel_h__
#define __LanguageModelling_LanguageModel_h__

// DJW_TODO - reintegrate PJC's changes
// PJC_TODO - commit LanguageModelParams.h
//#include "LanguageModelParams.h" 

#include "SymbolAlphabet.h"
#include <vector>

/////////////////////////////////////////////////////////////////////////////

namespace Dasher {class CLanguageModel;}

class Dasher::CLanguageModel
{
public:
	
	/////////////////////////////////////////////////////////////////////////////

	CLanguageModel(const CSymbolAlphabet& Alphabet);

//=======/
//	CLanguageModel(const CAlphabet* pcAlphabet, CLanguageModelParams *_params);
//>>>>>>> 1.3

	virtual ~CLanguageModel() {}

	// Handle for a language model context
	// 0 is reserved
	typedef size_t Context;

	/////////////////////////////////////////////////////////////////////////////
	// Context creation/destruction
	////////////////////////////////////////////////////////////////////////////

	// Create a context (empty)
	virtual Context CreateEmptyContext() =0;
	virtual Context CloneContext(Context Context) =0;
	virtual void ReleaseContext(Context Context) =0;

	/////////////////////////////////////////////////////////////////////////////
	// Context modifiers
	////////////////////////////////////////////////////////////////////////////

	// Update context with a character - only modifies context
	virtual void EnterSymbol(Context context, int Symbol) const =0;
	
	// Add character to the language model at the current context and update the context 
	// - modifies both the context and the LanguageModel
	virtual void LearnSymbol(Context context, int Symbol) =0;
	
	/////////////////////////////////////////////////////////////////////////////
	// Prediction
	/////////////////////////////////////////////////////////////////////////////

	// Get symbol probability distribution
	virtual void GetProbs(Context Context, std::vector<unsigned int> &Probs, int iNorm) const =0;

protected:
	int GetSize() const {return m_Alphabet.GetSize();}

	const CSymbolAlphabet& SymbolAlphabet() const
	{
		return m_Alphabet;
	}

	// DJW - non-symbol code should be moved elsewhere - new CLanguageModel knows nothing about strings
	//	virtual const char *defaultContextString() const {
	//	  // Default context string for new documents - replace this if your language model needs something different.
	//	  return ". ";
	//	};
	
	//	CLanguageModelParams *params;
	
private:

	const CSymbolAlphabet m_Alphabet;

};


/////////////////////////////////////////////////////////////////////////////

#endif // ndef __LanguageModelling_LanguageModel_h__