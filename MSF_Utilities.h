#pragma once

#include "MSF_Config.h"

//-------------------------------------------------------------------------------------------------
// Internal utilities to help with string formatting
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Test if a character is ASCII alphanumeric. This is only used when testing for
// print characters so it doesn't not need to be UTF8 compatible.
// Note: Casting to unsigned so I can do sub+cmp instead of cmp+cmp
//-------------------------------------------------------------------------------------------------
constexpr bool MSF_IsAsciiAlpha(char aCharacter) { return (uint32_t(aCharacter - 'A') < 26) || (uint32_t(aCharacter - 'a') < 26); }
constexpr bool MSF_IsAsciiAlpha(char8_t aCharacter) { return (uint32_t(aCharacter - 'A') < 26) || (uint32_t(aCharacter - 'a') < 26); }
constexpr bool MSF_IsAsciiAlpha(char16_t aCharacter) { return (uint32_t(aCharacter - 'A') < 26) || (uint32_t(aCharacter - 'a') < 26); }
constexpr bool MSF_IsAsciiAlpha(char32_t aCharacter) { return (uint32_t(aCharacter - 'A') < 26) || (uint32_t(aCharacter - 'a') < 26); }
constexpr bool MSF_IsAsciiAlpha(wchar_t aCharacter) { return (uint32_t(aCharacter - 'A') < 26) || (uint32_t(aCharacter - 'a') < 26); }

//-------------------------------------------------------------------------------------------------
// Test if a character is a digit.
// Note: Casting to unsigned so I can do sub+cmp instead of cmp+cmp
//-------------------------------------------------------------------------------------------------
constexpr bool MSF_IsDigit(char aCharacter) { return (uint32_t(aCharacter - '0') < 10); }
constexpr bool MSF_IsDigit(char8_t aCharacter) { return (uint32_t(aCharacter - '0') < 10); }
constexpr bool MSF_IsDigit(char16_t aCharacter) { return (uint32_t(aCharacter - '0') < 10); }
constexpr bool MSF_IsDigit(char32_t aCharacter) { return (uint32_t(aCharacter - '0') < 10); }
constexpr bool MSF_IsDigit(wchar_t aCharacter) { return (uint32_t(aCharacter - '0') < 10); }

//-------------------------------------------------------------------------------------------------
// Get the maximum value between two integers. This is done branchless for performance
//-------------------------------------------------------------------------------------------------
template <typename Type>
constexpr Type MSF_IntMax(Type const& anA, Type const& aB)
{
	return anA - ((anA - aB) & -(anA < aB));
}

//-------------------------------------------------------------------------------------------------
// Get the minimum value between two integers. This is done branchless for performance
//-------------------------------------------------------------------------------------------------
template <typename Type>
constexpr Type MSF_IntMin(Type const& anA, Type const& aB)
{
	return aB + ((anA - aB) & -(anA < aB));
}

//-------------------------------------------------------------------------------------------------
// Get the absolute value of an integer. This is done branchless for performance.
//-------------------------------------------------------------------------------------------------
template<typename Type>
constexpr Type MSF_IntAbs_private(Type aValue, Type aMask)
{
	return (aValue ^ aMask) - aMask;
}
template<typename Type>
constexpr Type MSF_IntAbs(Type aValue)
{
	return MSF_IntAbs_private<Type>(aValue, aValue >> (sizeof(aValue) * 8 - 1));
}

//-------------------------------------------------------------------------------------------------
// Simple string length functions. Not using std versions to allow for link time optimizations
//-------------------------------------------------------------------------------------------------
extern size_t MSF_Strlen(char const* aString);
extern size_t MSF_Strlen(char8_t const* aString);
extern size_t MSF_Strlen(char16_t const* aString);
extern size_t MSF_Strlen(char32_t const* aString);
extern size_t MSF_Strlen(wchar_t const* aString);

//-------------------------------------------------------------------------------------------------
// This is equivalent to memset but safe by default.
//-------------------------------------------------------------------------------------------------
extern void MSF_SplatChars(char* aBuffer, char const* aBufferEnd, char aValue, size_t aCount);
extern void MSF_SplatChars(char8_t* aBuffer, char8_t const* aBufferEnd, char8_t aValue, size_t aCount);
extern void MSF_SplatChars(char16_t* aBuffer, char16_t const* aBufferEnd, char16_t aValue, size_t aCount);
extern void MSF_SplatChars(char32_t* aBuffer, char32_t const* aBufferEnd, char32_t aValue, size_t aCount);
extern void MSF_SplatChars(wchar_t* aBuffer, wchar_t const* aBufferEnd, wchar_t aValue, size_t aCount);

//-------------------------------------------------------------------------------------------------
// Basic safe strcpy, includes null terminator
//-------------------------------------------------------------------------------------------------
extern void MSF_CopyChars(char* aBuffer, char const* aBufferEnd, char const* aSource);
extern void MSF_CopyChars(char8_t* aBuffer, char8_t const* aBufferEnd, char8_t const* aSource);
extern void MSF_CopyChars(char16_t* aBuffer, char16_t const* aBufferEnd, char16_t const* aSource);
extern void MSF_CopyChars(char32_t* aBuffer, char32_t const* aBufferEnd, char32_t const* aSource);
extern void MSF_CopyChars(wchar_t* aBuffer, wchar_t const* aBufferEnd, wchar_t const* aSource);

//-------------------------------------------------------------------------------------------------
// This is equivalent to memmove but safe by default, it does NOT add null terminator
//-------------------------------------------------------------------------------------------------
extern void MSF_CopyChars(char* aBuffer, char const* aBufferEnd, char const* aSource, size_t aSourceLength);
extern void MSF_CopyChars(char8_t* aBuffer, char8_t const* aBufferEnd, char8_t const* aSource, size_t aSourceLength);
extern void MSF_CopyChars(char16_t* aBuffer, char16_t const* aBufferEnd, char16_t const* aSource, size_t aSourceLength);
extern void MSF_CopyChars(char32_t* aBuffer, char32_t const* aBufferEnd, char32_t const* aSource, size_t aSourceLength);
extern void MSF_CopyChars(wchar_t* aBuffer, wchar_t const* aBufferEnd, wchar_t const* aSource, size_t aSourceLength);
