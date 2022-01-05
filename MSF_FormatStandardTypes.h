#pragma once

#include "MSF_FormatPrint.h"

//-------------------------------------------------------------------------------------------------
// Print functions for standard printf/format types: char, string, ints and floats
//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatChar
{
	static const uint32_t ValidTypes = MSF_StringFormatType::Type8 | MSF_StringFormatType::Type16 | MSF_StringFormatType::Type32 | MSF_StringFormatType::Type64;

	size_t ValidateUTF8(MSF_PrintData& aData, MSF_StringFormatType const& aValue);
	size_t ValidateUTF16(MSF_PrintData& aData, MSF_StringFormatType const& aValue);
	size_t ValidateUTF32(MSF_PrintData& aData, MSF_StringFormatType const& aValue);

	size_t PrintUTF8(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData);
	size_t PrintUTF16(char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aData);
	size_t PrintUTF32(char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aData);
}

//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatString
{
	static const uint32_t ValidTypes = MSF_StringFormatType::TypeString;

	size_t ValidateUTF8(MSF_PrintData& aData, MSF_StringFormatType const& aValue);
	size_t ValidateUTF16(MSF_PrintData& aData, MSF_StringFormatType const& aValue);
	size_t ValidateUTF32(MSF_PrintData& aData, MSF_StringFormatType const& aValue);

	size_t ValidateUTF8(MSF_PrintData& aData, MSF_StringFormatType const& aValue, size_t aLength);
	size_t ValidateUTF16(MSF_PrintData& aData, MSF_StringFormatType const& aValue, size_t aLength);
	size_t ValidateUTF32(MSF_PrintData& aData, MSF_StringFormatType const& aValue, size_t aLength);

	size_t PrintUTF8(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData);
	size_t PrintUTF16(char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aData);
	size_t PrintUTF32(char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aData);

	size_t CopyLength(MSF_StringFormatType const& aValue);
}

//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatInt
{
	static const uint32_t ValidTypes = MSF_StringFormatType::Type8 | MSF_StringFormatType::Type16 | MSF_StringFormatType::Type32 | MSF_StringFormatType::Type64;

	size_t Validate(MSF_PrintData& aData, MSF_StringFormatType const& aValue);
	size_t ValidateOctal(MSF_PrintData& aData, MSF_StringFormatType const& aValue);
	size_t ValidateHex(MSF_PrintData& aData, MSF_StringFormatType const& aValue);
	size_t ValidatePointer(MSF_PrintData& aData, MSF_StringFormatType const& aValue);

	size_t PrintUTF8(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData);
	size_t PrintUTF16(char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aData);
	size_t PrintUTF32(char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aData);
}

//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatFloat
{
	static const uint32_t ValidTypes = MSF_StringFormatType::Typefloat | MSF_StringFormatType::Typedouble;

	size_t Validate(MSF_PrintData& aData, MSF_StringFormatType const& aValue);
	size_t PrintUTF8(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData);
	size_t PrintUTF16(char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aData);
	size_t PrintUTF32(char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aData);
}
