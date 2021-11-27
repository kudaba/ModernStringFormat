#pragma once

#include <stdint.h>
#include <stddef.h>

//-------------------------------------------------------------------------------------------------
// Helper for reading code points from UTF strings
//-------------------------------------------------------------------------------------------------
struct MSF_CodeRead
{
	uint32_t CodePoint; // Code point read
	uint32_t CharsRead; // Number of characters read from the input string
};
//-------------------------------------------------------------------------------------------------
// When converting between types return both the number of array elements written to as well
// as the number of unicode characters processed
//-------------------------------------------------------------------------------------------------
struct MSF_CharactersWritten
{
	size_t Characters; // Number of visible characters written
	size_t Elements; // Number of array elements written to
};

//-------------------------------------------------------------------------------------------------
// Read one character from a UTF8 or UTF16 string
//-------------------------------------------------------------------------------------------------
MSF_CodeRead MSF_ReadCodePoint(char const* aString);
MSF_CodeRead MSF_ReadCodePoint(char16_t const* aString);
MSF_CodeRead MSF_ReadCodePoint(char32_t const* aString);
MSF_CodeRead MSF_ReadCodePoint(wchar_t const* aString);

//-------------------------------------------------------------------------------------------------
// Write a code point to a UTF8 or UTF16 string
//-------------------------------------------------------------------------------------------------
uint32_t MSF_WriteCodePoint(uint32_t aCodePoint, char aString[4/sizeof(char)]);
uint32_t MSF_WriteCodePoint(uint32_t aCodePoint, char16_t aString[4 / sizeof(char16_t)]);
uint32_t MSF_WriteCodePoint(uint32_t aCodePoint, char32_t aString[4 / sizeof(char32_t)]);
uint32_t MSF_WriteCodePoint(uint32_t aCodePoint, wchar_t aString[4 / sizeof(wchar_t)]);

//-------------------------------------------------------------------------------------------------
// Copy a UTF string to another UTF string.
// If there is not enough space in the buffer for a character the entire character is omitted
// You can optinally limit the copy by number of characters as well as number of elements in the buffer
//-------------------------------------------------------------------------------------------------
MSF_CharactersWritten MSF_UTFCopy(char* aStringOut, size_t aBufferLength, char const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(char* aStringOut, size_t aBufferLength, char16_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(char* aStringOut, size_t aBufferLength, char32_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(char * aStringOut, size_t aBufferLength, wchar_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);

MSF_CharactersWritten MSF_UTFCopy(char16_t* aStringOut, size_t aBufferLength, char const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(char16_t* aStringOut, size_t aBufferLength, char16_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(char16_t* aStringOut, size_t aBufferLength, char32_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(char16_t* aStringOut, size_t aBufferLength, wchar_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);

MSF_CharactersWritten MSF_UTFCopy(char32_t* aStringOut, size_t aBufferLength, char const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(char32_t* aStringOut, size_t aBufferLength, char16_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(char32_t* aStringOut, size_t aBufferLength, char32_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(char32_t* aStringOut, size_t aBufferLength, wchar_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);

MSF_CharactersWritten MSF_UTFCopy(wchar_t* aStringOut, size_t aBufferLength, char const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(wchar_t* aStringOut, size_t aBufferLength, char16_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(wchar_t* aStringOut, size_t aBufferLength, char32_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);
MSF_CharactersWritten MSF_UTFCopy(wchar_t* aStringOut, size_t aBufferLength, wchar_t const* aStringIn, size_t aCharacterLimit = SIZE_MAX);

template <typename CharOut, size_t Size, typename CharIn>
MSF_CharactersWritten MSF_UTFCopy(CharOut(&aStringOut)[Size], CharIn const* aStringIn, size_t aCharacterLimit = SIZE_MAX)
{
	return MSF_UTFCopy(aStringOut, Size, aStringIn, aCharacterLimit);
}

//-------------------------------------------------------------------------------------------------
// Used hidden functionality in conversion functions to count the required characters
//-------------------------------------------------------------------------------------------------
template <typename CharTo, typename CharFrom>
inline MSF_CharactersWritten MSF_UTFCopyLength(CharFrom const* aString, size_t aCharacterLimit = SIZE_MAX) { return MSF_UTFCopy((CharTo*)nullptr, SIZE_MAX, aString, aCharacterLimit); }
