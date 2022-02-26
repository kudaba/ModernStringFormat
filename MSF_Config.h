#pragma once

#include <stdint.h>
#include <stddef.h>

//-------------------------------------------------------------------------------------------------
// Global configuration options. These must be overridden at the project level to make sure
// that all cpp files are using the same state
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Helper to stringify things (not to be confused with MSF_STRING)
//-------------------------------------------------------------------------------------------------
#define MSF_STR2(x) #x
#define MSF_STR(x) MSF_STR2(x)

//-------------------------------------------------------------------------------------------------
// Set the maximum number of arguments that can be provided to any print statement. Mainly affects
// stack space usage
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_MAX_ARGUMENTS)
#define MSF_MAX_ARGUMENTS 32
#endif

//-------------------------------------------------------------------------------------------------
// Control the default parameter for StrFmt helper classes
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_DEFAULT_FMT_SIZE)
#define MSF_DEFAULT_FMT_SIZE 512
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
// Set MSF_FORMAT_LOCAL_PLATFORM to different values to control which configuration to select
// MSF_FORMAT_PLATFORM_MANUAL - Will use any options currently set, otherwise each option defaults to whatever arbitrarily thought was best
// MSF_FORMAT_PLATFORM_AUTO - Adjust config to the current platform (fallback to LINUX if platform is unknown)
// MSF_FORMAT_PLATFORM_* - Choose this platform
//-------------------------------------------------------------------------------------------------
#define MSF_FORMAT_PLATFORM_MANUAL 0
#define MSF_FORMAT_PLATFORM_AUTO 1
#define MSF_FORMAT_PLATFORM_ANDROID 10
#define MSF_FORMAT_PLATFORM_APPLE 20
#define MSF_FORMAT_PLATFORM_LINUX 30
#define MSF_FORMAT_PLATFORM_WINDOWS 40

#if !defined(MSF_FORMAT_LOCAL_PLATFORM)
#define MSF_FORMAT_LOCAL_PLATFORM MSF_FORMAT_PLATFORM_AUTO
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
// Passing in nullptr for a string will cause format to print '(null)' the same as most implementations
// This flag controls whether precision will clip the '(null)' string or omit it entirely.
// Setting to 1 will omit the string if precision is less than 6.
//-------------------------------------------------------------------------------------------------
#if !MSF_STRING_NULL_ALL_OR_NOTHING
#define MSF_STRING_NULL_ALL_OR_NOTHING 1
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
// If enabled then the epsilon modifier to check for rounding will be further decreased
// by the precision of the number being printed.
//-------------------------------------------------------------------------------------------------
#if !MSF_FLOAT_EPSILON_AFFECTED_BY_PRECISION
#define MSF_FLOAT_EPSILON_AFFECTED_BY_PRECISION 0
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

//-------------------------------------------------------------------------------------------------
// c++20 introduces char8_t, we define it for pre-c++20 to keep code easy to read
//-------------------------------------------------------------------------------------------------
#if !defined(__cpp_char8_t)
typedef unsigned char char8_t;
#endif

//-------------------------------------------------------------------------------------------------
// wchar_t is utf16 or utf32 depending on platform and compiler flags
//-------------------------------------------------------------------------------------------------
#if WCHAR_MAX == UINT16_MAX
#define MSF_WCHAR_IS_16 1
typedef char16_t MSF_WChar;
#else
#define MSF_WCHAR_IS_16 0
typedef char32_t MSF_WChar;
#endif

//-------------------------------------------------------------------------------------------------
// When making a capture function for ModernStringFormat, use MSF_STRING(char) in place of char const*
// This will allow you to enable compile time validation on inputs.
//-------------------------------------------------------------------------------------------------
#if defined(MSF_VALIDATION_ENABLED)

#if defined(_MSC_FULL_VER)
#if _MSC_FULL_VER < 193131104
#error "Validation is only availble on MSVC platform toolset 143 or higher"
#endif
#elif defined(__clang_major__)
#if __clang_major__ < 12
#error "Validation is only availble on clang 12 or higher"
#endif
#elif defined(__GNUC__)
#if __GNUC__ < 11
#error "Validation is only availble on gcc 11 or higher"
#endif
#endif

#define MSF_STRING(Char) MSF_Validator<Char, typename MSF_TypeWrapper<Args>::Value...>
#define MSF_VALIDATION_ONLY(...) __VA_ARGS__
#define MSF_LOOKUP_ID(...) static constexpr uint64_t ID = __VA_ARGS__
#else
#define MSF_STRING(Char) Char const*
#define MSF_VALIDATION_ONLY(...)
#define MSF_LOOKUP_ID(...) 
#endif
