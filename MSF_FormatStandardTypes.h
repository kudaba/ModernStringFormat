#pragma once

#include "MSF_FormatPrint.h"

//-------------------------------------------------------------------------------------------------
// Print functions for standard printf/format types: char, string, ints and floats
//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatChar
{
	static const uint32_t ValidTypes = MSF_StringFormatType::Type8 | MSF_StringFormatType::Type16 | MSF_StringFormatType::Type32 | MSF_StringFormatType::Type64;

	size_t Validate(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData);
}

//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatString
{
	static const uint32_t ValidTypes = MSF_StringFormatType::TypeString;

	size_t Validate(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t ValidateStringLength(MSF_PrintData& aPrintData, char const* aString);
	size_t ValidateStringLength(MSF_PrintData& aPrintData, char16_t const* aString);
	size_t ValidateStringLength(MSF_PrintData& aPrintData, char32_t const* aString);
	size_t ValidateStringLength(MSF_PrintData& aPrintData, wchar_t const* aString);

	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData);
	size_t PrintString(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, char const* aString);
	size_t PrintString(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, char16_t const* aString);
	size_t PrintString(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, char32_t const* aString);
	size_t PrintString(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, wchar_t const* aString);
}

//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatInt
{
	static const uint32_t ValidTypes = MSF_StringFormatType::Type8 | MSF_StringFormatType::Type16 | MSF_StringFormatType::Type32 | MSF_StringFormatType::Type64;

	size_t Validate(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t ValidateOctal(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t ValidateHex(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t ValidatePointer(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData);
}

//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatFloat
{
	static const uint32_t ValidTypes = MSF_StringFormatType::Typefloat | MSF_StringFormatType::Typedouble;

	size_t Validate(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData);
}
