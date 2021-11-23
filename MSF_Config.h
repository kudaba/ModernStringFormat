#pragma once

#include <stdint.h>

//-------------------------------------------------------------------------------------------------
// Global configuration options. These must be overridden at the project level to make sure
// that all cpp files are using the same state
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Formatting options:
// When MSF_FORMAT_LOCAL_PLATFORM is enabled, then some formatting options are adjusted to adhere
// to local platforms versions of printf. When disabled then users can override the specific options
// to have a consistent experience across all platforms.
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_FORMAT_LOCAL_PLATFORM)
#define MSF_FORMAT_LOCAL_PLATFORM 1
#endif

#if !MSF_FORMAT_LOCAL_PLATFORM

//-------------------------------------------------------------------------------------------------
// Allow leading character to be zeroes, when requested, for string and character printing.
//-------------------------------------------------------------------------------------------------
#if !MSF_ALLOW_LEADING_ZERO
#define MSF_ALLOW_LEADING_ZERO 1
#endif

//-------------------------------------------------------------------------------------------------
// Override precision to always be size of pointer address
//-------------------------------------------------------------------------------------------------
#if !MSF_POINTER_FORCE_PRECISION
#define MSF_POINTER_FORCE_PRECISION 1
#endif

//-------------------------------------------------------------------------------------------------
// Always include prefix '0x' when printing out pointer addresses
//-------------------------------------------------------------------------------------------------
#if !MSF_POINTER_ADD_PREFIX
#define MSF_POINTER_ADD_PREFIX 1
#endif

//-------------------------------------------------------------------------------------------------
// Allow inclusion of leading '+' or ' ' character, upon request, when printing out pointer addresses
//-------------------------------------------------------------------------------------------------
#if !MSF_POINTER_ADD_SIGN_OR_BLANK
#define MSF_POINTER_ADD_SIGN_OR_BLANK 0
#endif

//-------------------------------------------------------------------------------------------------
// Always print pointer addresses in capital letters. (does not affect '%P')
//-------------------------------------------------------------------------------------------------
#if !MSF_POINTER_PRINT_CAPS
#define MSF_POINTER_PRINT_CAPS 0
#endif

#endif // !MSF_FORMAT_LOCAL_PLATFORM

//-------------------------------------------------------------------------------------------------
// Determine if we should enable asserts in this build or not if it is not already defined by user
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_ASSERTS_ENABLED)
#if defined(NDEBUG)
#define MSF_ASSERTS_ENABLED 0
#else
#define MSF_ASSERTS_ENABLED 1
#endif
#endif // !defined(MSF_ASSERTS_ENABLED)