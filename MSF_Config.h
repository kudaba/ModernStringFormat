#pragma once

#include <stdint.h>

//-------------------------------------------------------------------------------------------------
// Global configuration options. These must be overridden at the project level to make sure
// that all cpp files are using the same state
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Set the maximum number of arguments that can be provided to any print statement. Mainly affects
// stack space usage
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_MAX_ARGUMENTS)
#define MSF_MAX_ARGUMENTS 32
#endif

//-------------------------------------------------------------------------------------------------
// Set alignment to use when making a copy of a string format object, must be power of 2
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_COPY_ALIGNMENT)
#define MSF_COPY_ALIGNMENT 4
#endif

//-------------------------------------------------------------------------------------------------
// When pedantic error checking is enable, strings will have additional checks
// i.e. "%++d" will error about the duplicate flags
// If pedantic error checking is disabled then duplicate values will just overwrite previous
// values or simply be redundant
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_ERROR_PEDANTIC)
#define MSF_ERROR_PEDANTIC 0
#endif

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
#if !MSF_STRING_ALLOW_LEADING_ZERO
#define MSF_STRING_ALLOW_LEADING_ZERO 1
#endif

//-------------------------------------------------------------------------------------------------
// Whether to treat the precision limit, when printing strings, to limit the number of characters
// written, or limit the number of bytes written
//-------------------------------------------------------------------------------------------------
#if !MSF_STRING_PRECISION_IS_CHARACTERS
#define MSF_STRING_PRECISION_IS_CHARACTERS 1
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

//-------------------------------------------------------------------------------------------------
// If enabled will print "(nil)" instead of 0
//-------------------------------------------------------------------------------------------------
#if !MSF_POINTER_PRINT_NIL
#define MSF_POINTER_PRINT_NIL 0
#endif

//-------------------------------------------------------------------------------------------------
// If enabled then including the prefix flag '#' when printing floats and double will include
// the whole number when padding to precision.
//    With Flag: #.5f -> 0.0000
// Without Flag: #.5f -> 0.00000
//-------------------------------------------------------------------------------------------------
#if !MSF_FLOAT_PREFIX_INCLUDE_WHOLE_NUMBER
#define MSF_FLOAT_PREFIX_INCLUDE_WHOLE_NUMBER 1
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
