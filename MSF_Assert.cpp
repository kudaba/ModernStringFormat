#include "MSF_Assert.h"

#if MSF_ASSERTS_ENABLED
#include <stdio.h>

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void (*MSF_LogAssertExternal)(const char* aFile, int aLine, char const* aCondition, MSF_StringFormat const& aStringFormat);
bool (*MSF_DoAssert)();

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
volatile bool theIsAsserting;
bool MSF_IsAsserting()
{
    return theIsAsserting;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void MSF_LogAssertInternal(const char* aFile, int aLine, char const* aCondition, MSF_StringFormat const& aStringFormat)
{
	// Don't allow asserts while asserting
    if (theIsAsserting)
        return;

    theIsAsserting = true;

	if (MSF_LogAssertExternal != nullptr)
	{
		MSF_LogAssertExternal(aFile, aLine, aCondition, aStringFormat);
	}
	else
	{
        char temporary[4096];
        MSF_FormatString(aStringFormat, temporary, sizeof(temporary));
		printf("%s(%d): %s failed: %s", aFile, aLine, aCondition, temporary);
	}

    theIsAsserting = false;
}
#endif
