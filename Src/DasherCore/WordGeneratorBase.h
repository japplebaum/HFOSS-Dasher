#include <string>
using namespace std;

namespace Dasher {
/**
 * The base class for all word generators. Word generators encapsulate
 * logic for simply generating words based on implementation-specific
 * conditions. Currently, this is being used to wrap GameMode's simple
 * text-file reading mechanism, but another generator can easily be
 * written that selects words based on a function of the current value
 * of the Sri Lankan rupee and the amount of twitter feeds regarding
 * the winter olympics, for example.
 */ 
class CWordGeneratorBase {
public:
	CWordGeneratorBase(int regen) : m_iRegenTime(regen), m_bWordsReady(false) {}

	virtual ~CWordGeneratorBase() { } 


    /**
     * Gets a single word based on the generation rules.
	 * @pre { Generate must have been called at least once. }
     * @return The next string from this generator
     */
	virtual std::string	GetNextWord() = 0;

	/**
     * Gets the previous word in the generated string based on generation rules.
	 * @pre { Generate must have been called at least once. }
	 * @return The previous string from this generator
     */
	//virtual std::string GetPrevWord() = 0;

    /**
     * Generate the words based on the generation rules
	 * @return True if the generation succeeded, false if not.
	 * @warning The return value of this function should always be checked in case it's using files.
     */
	virtual bool Generate() = 0;
protected:
	/**
     * The time in seconds after which a generator should regenerate.
     */
	int m_iRegenTime;

    /** 
     * A boolean representing whether the words have been generated yet.
     */
	bool m_bWordsReady;
};
}