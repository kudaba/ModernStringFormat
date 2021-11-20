#pragma once

#include "MSF_Config.h"
#include "MSF_Format.h"

//-------------------------------------------------------------------------------------------------
// Define basic break mechanism for supported platforms
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_BREAK)
#if _MSC_VER
#define MSF_BREAK __debugbreak()
#elif defined(__i386__) || defined(__x86_64__)
#define MSF_BREAK __asm__ volatile("int $0x03");
#else
#error 
#endif
#endif // !defined(MSF_BREAK)

//-------------------------------------------------------------------------------------------------
// Custom assert handler used to validate internal operation
//-------------------------------------------------------------------------------------------------
#if MSF_ASSERTS_ENABLED

//-------------------------------------------------------------------------------------------------
// Helper to stringify assert conditions
//-------------------------------------------------------------------------------------------------
#define MSF_STR2(x) #x
#define MSF_STR(x) MSF_STR2(x)

//-------------------------------------------------------------------------------------------------
// MSF_IsAsserting is set to true so we can detect if a format is occuring during assert processing
// This can be useful to prevent firing recursive asserts, like when assert error handling is enabled
//-------------------------------------------------------------------------------------------------
extern bool MSF_IsAsserting();

//-------------------------------------------------------------------------------------------------
// Internal assert processing will send info to printf if no custom handler is set
//-------------------------------------------------------------------------------------------------
extern void MSF_LogAssertInternal(const char* aFile, int aLine, char const* aCondition, MSF_StringFormat const& aStringFormat);

//-------------------------------------------------------------------------------------------------
// Set this function pointer to override where asserts get printed to
//-------------------------------------------------------------------------------------------------
extern void (*MSF_LogAssertExternal)(const char* aFile, int aLine, char const* aCondition, MSF_StringFormat const& aStringFormat);

//-------------------------------------------------------------------------------------------------
// Set this function if you want control of when to enable asserts will cause a program to halt
//-------------------------------------------------------------------------------------------------
extern bool (*MSF_LogAssertBreak)();

//-------------------------------------------------------------------------------------------------
// Assert macro
//-------------------------------------------------------------------------------------------------
#define MSF_ASSERT(condition, ...) do {\
		if (!(condition)) { \
			MSF_LogAssertInternal(__FILE__, __LINE__, MSF_STR(condition), MSF_MakeStringFormat("" __VA_ARGS__));\
			if (MSF_LogAssertBreak == nullptr || MSF_LogAssertBreak()) MSF_BREAK;\
		} } while(0)\

#else

#define MSF_ASSERT(...) (void)0

#endif // MSF_ASSERTS_ENABLED

//-------------------------------------------------------------------------------------------------
// Optional heavy asserts. When enabled these do additional checks that may have significant effect on performance
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_HEAVY_ASSERTS_ENABLED)
#define MSF_HEAVY_ASSERTS_ENABLED 0
#endif

#if MSF_HEAVY_ASSERTS_ENABLED
#define MSF_HEAVY_ASSERT MSF_ASSERT
#else
#define MSF_HEAVY_ASSERT(...) (void)0
#endif
