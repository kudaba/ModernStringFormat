#pragma once

#include <stdint.h>
#include <stddef.h>

//-------------------------------------------------------------------------------------------------
// The returned value is the start of the string content, not the buffer. This is an optimization
// to prevent having to reverse the characters in the string.
//-------------------------------------------------------------------------------------------------
template <typename Type, typename Char = char>
struct MSF_UnsignedToString
{
	enum
	{
		MaxLength = sizeof(Type) * 8, // accounts for binary, base 1
		BufferLength = MaxLength + 1
	};

	MSF_UnsignedToString(Type aValue, uint32_t aRadix = 10, char aHexStart = 'a');

	uint32_t Length() const { return myLength; }
	operator Char const* () const { return myBuffer + MaxLength - myLength; }
	Char const* GetString() const { return myBuffer + MaxLength - myLength; }

protected:
	uint32_t myLength;
	Char myBuffer[BufferLength];
};

//-------------------------------------------------------------------------------------------------
// signed intergers only supports base 10, any other base will be the same as unsigned
//-------------------------------------------------------------------------------------------------
template<typename SignedType, typename UnsignedType, typename Char = char>
struct MSF_SignedToString : public MSF_UnsignedToString<UnsignedType, Char>
{
	MSF_SignedToString(SignedType aValue, uint32_t aRadix = 10, char aHexStart = 'a');
};

extern template struct MSF_UnsignedToString<uint8_t, char>;
extern template struct MSF_UnsignedToString<uint16_t, char>;
extern template struct MSF_UnsignedToString<uint32_t, char>;
extern template struct MSF_UnsignedToString<uint64_t, char>;
extern template struct MSF_SignedToString<int8_t, uint8_t, char>;
extern template struct MSF_SignedToString<int16_t, uint16_t, char>;
extern template struct MSF_SignedToString<int32_t, uint32_t, char>;
extern template struct MSF_SignedToString<int64_t, uint64_t, char>;

extern template struct MSF_UnsignedToString<uint8_t, char16_t>;
extern template struct MSF_UnsignedToString<uint16_t, char16_t>;
extern template struct MSF_UnsignedToString<uint32_t, char16_t>;
extern template struct MSF_UnsignedToString<uint64_t, char16_t>;
extern template struct MSF_SignedToString<int8_t, uint8_t, char16_t>;
extern template struct MSF_SignedToString<int16_t, uint16_t, char16_t>;
extern template struct MSF_SignedToString<int32_t, uint32_t, char16_t>;
extern template struct MSF_SignedToString<int64_t, uint64_t, char16_t>;

extern template struct MSF_UnsignedToString<uint8_t, char32_t>;
extern template struct MSF_UnsignedToString<uint16_t, char32_t>;
extern template struct MSF_UnsignedToString<uint32_t, char32_t>;
extern template struct MSF_UnsignedToString<uint64_t, char32_t>;
extern template struct MSF_SignedToString<int8_t, uint8_t, char32_t>;
extern template struct MSF_SignedToString<int16_t, uint16_t, char32_t>;
extern template struct MSF_SignedToString<int32_t, uint32_t, char32_t>;
extern template struct MSF_SignedToString<int64_t, uint64_t, char32_t>;

//-------------------------------------------------------------------------------------------------
// Explicitly implementing supported types only to help reduce compiler errors when using incorrect types
//-------------------------------------------------------------------------------------------------
inline MSF_UnsignedToString<uint8_t> MSF_IntegerToString(uint8_t aValue, uint32_t aRadix = 10, char aHexStart = 'a') { return MSF_UnsignedToString<uint8_t>(aValue, aRadix, aHexStart); }
inline MSF_UnsignedToString<uint16_t> MSF_IntegerToString(uint16_t aValue, uint32_t aRadix = 10, char aHexStart = 'a') { return MSF_UnsignedToString<uint16_t>(aValue, aRadix, aHexStart); }
inline MSF_UnsignedToString<uint32_t> MSF_IntegerToString(uint32_t aValue, uint32_t aRadix = 10, char aHexStart = 'a') { return MSF_UnsignedToString<uint32_t>(aValue, aRadix, aHexStart); }
inline MSF_UnsignedToString<uint64_t> MSF_IntegerToString(uint64_t aValue, uint32_t aRadix = 10, char aHexStart = 'a') { return MSF_UnsignedToString<uint64_t>(aValue, aRadix, aHexStart); }

inline MSF_SignedToString<int8_t, uint8_t> MSF_IntegerToString(int8_t aValue, uint32_t aRadix = 10, char aHexStart = 'a') { return MSF_SignedToString<int8_t, uint8_t>(aValue, aRadix, aHexStart); }
inline MSF_SignedToString<int16_t, uint16_t> MSF_IntegerToString(int16_t aValue, uint32_t aRadix = 10, char aHexStart = 'a') { return MSF_SignedToString<int16_t, uint16_t>(aValue, aRadix, aHexStart); }
inline MSF_SignedToString<int32_t, uint32_t> MSF_IntegerToString(int32_t aValue, uint32_t aRadix = 10, char aHexStart = 'a') { return MSF_SignedToString<int32_t, uint32_t>(aValue, aRadix, aHexStart); }
inline MSF_SignedToString<int64_t, uint64_t> MSF_IntegerToString(int64_t aValue, uint32_t aRadix = 10, char aHexStart = 'a') { return MSF_SignedToString<int64_t, uint64_t>(aValue, aRadix, aHexStart); }

//-------------------------------------------------------------------------------------------------
// Use DoubleToString to print float/double float values.
// Unlike UnsignedToString/SignedToString you have more control over the format.
//
// Runtime checks are performed on the buffer size.  If there's not enough
// room to print then nothing at all is printed and -1 is returned.
// Otherwise the returned value is the number of characters printed.
// If nullptr is passed in as the output buffer then the number of required characters are returned. (NOT including null terminator)
//
// aValue - The value to be printed
// anOutput - The buffer to be printed to 
// aLength - The number of characters available to write to in anOutput
// aFormat - Equivalent to printf formats (f,F,e,E,g,G)
// aWidth - the total number of characters to print
// aPrecision - Truncate the number of character printed
// aFlags - See MSF_PrintFlags.  These are equivalent to the printf flags "-+ 0#"
//-------------------------------------------------------------------------------------------------
int MSF_DoubleToString(double aValue, char* anOutput, size_t aLength, char aFormat = 'f', uint32_t aWidth = 0, uint32_t aPrecision = 6, uint32_t someFlags = 0);
int MSF_DoubleToString(double aValue, char16_t* anOutput, size_t aLength, char aFormat = 'f', uint32_t aWidth = 0, uint32_t aPrecision = 6, uint32_t someFlags = 0);
int MSF_DoubleToString(double aValue, char32_t* anOutput, size_t aLength, char aFormat = 'f', uint32_t aWidth = 0, uint32_t aPrecision = 6, uint32_t someFlags = 0);

template<uint32_t Size>
int MSF_DoubleToString(double aValue, char(&anOutput)[Size], char aFormat = 'f', uint32_t aWidth = 0, uint32_t aPrecision = 6, uint32_t someFlags = 0)
{
	return MSF_DoubleToString(aValue, anOutput, Size, aFormat, aWidth, aPrecision, someFlags);
}

template<uint32_t Size>
int MSF_DoubleToString(double aValue, char16_t(&anOutput)[Size], char aFormat = 'f', uint32_t aWidth = 0, uint32_t aPrecision = 6, uint32_t someFlags = 0)
{
	return MSF_DoubleToString(aValue, anOutput, Size, aFormat, aWidth, aPrecision, someFlags);
}

template<uint32_t Size>
int MSF_DoubleToString(double aValue, char32_t(&anOutput)[Size], char aFormat = 'f', uint32_t aWidth = 0, uint32_t aPrecision = 6, uint32_t someFlags = 0)
{
	return MSF_DoubleToString(aValue, anOutput, Size, aFormat, aWidth, aPrecision, someFlags);
}

// Since there are no special characters, all string types will report the same length requirements
inline int MSF_DoubleToStringLength(double aValue, char aFormat = 'f', uint32_t aWidth = 0, uint32_t aPrecision = 6, uint32_t someFlags = 0)
{
	return MSF_DoubleToString(aValue, (char*)nullptr, 0, aFormat, aWidth, aPrecision, someFlags);
}

#define MSF_FloatToString MSF_DoubleToString
#define MSF_FloatToStringLength MSF_DoubleToStringLength
