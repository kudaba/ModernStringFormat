#pragma once

#include "MSF_Format.h"

//-------------------------------------------------------------------------------------------------
// Flags that affect print output (only available on some functions)
// See standards for more details:
// Microsoft: http://msdn.microsoft.com/en-us/library/56e442dc.aspx
// STD: https://en.cppreference.com/w/c/io/fprintf
//-------------------------------------------------------------------------------------------------
enum MSF_PrintFlags
{
	PRINT_PREFIX = 0x01,		// '#' Only show decimal point if digits follow, truncate trailing zeros
	PRINT_SIGN = 0x02,			// '+' show + sign when positive
	PRINT_BLANK = 0x04,			// ' ' prefix with spaces / show space when positive
	PRINT_LEFTALIGN = 0x08,		// '-' invalidates the zero option
	PRINT_ZERO = 0x10,			// '0' prefix with zeros
	PRINT_PRECISION = 0x20,		// '.' notifies that precision was specified
};

//-------------------------------------------------------------------------------------------------
// Print Data describes all the extra information passed with the type to be printed
//-------------------------------------------------------------------------------------------------
struct MSF_PrintData
{
	MSF_StringFormatType const* myValue; // the format type specified

	void const* myStart; // start of %<type> statement (including the '%')
	void const* myEnd; // end of the %<type> statement, not set in initial func call

	// to be written to in read mode
	size_t myMaxLength; // max length required, can be an estimate but must never be too low.

	// store any data you may need later
	mutable uint64_t myUserData;

	// %<width>.<precision>
	uint16_t myWidth;
	uint16_t myPrecision;

	// see MSF_PrintFlags
	uint8_t myFlags;

	// format to use for printing
	char myPrintChar;

	// Format/Output string size (MSF_StringFormatType::UTF16 or MSF_StringFormatType::UTF32)
	char myStringType;
};

struct MSF_CustomPrinter
{
	//-------------------------------------------------------------------------------------------------
	// Final validation of print info. Make any final adjustments and return the maximum length you might
	// need to print.
	// Do not set the max length directly.
	//-------------------------------------------------------------------------------------------------
	size_t(*ValidateUTF8)(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t(*ValidateUTF16)(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t(*ValidateUTF32)(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);

	//-------------------------------------------------------------------------------------------------
	// Print out your string to the output buffer and return characters printed
	//-------------------------------------------------------------------------------------------------
	size_t(*PrintUTF8)(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aPrintData);
	size_t(*PrintUTF16)(char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aPrintData);
	size_t(*PrintUTF32)(char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aPrintData);

	//-------------------------------------------------------------------------------------------------
	// For any type that is not embedded in the format type (i.e. more than 8 bytes), you can optionally
	// set this helper to get the number of bytes to copy when backing up the format for future printing
	//-------------------------------------------------------------------------------------------------
	size_t(*CopyLength)(MSF_StringFormatType const& aValue);

	template <typename Char>
	size_t Validate(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue) const;
	template <typename Char>
	size_t Print(Char* aBuffer, Char const* aBufferEnd, MSF_PrintData const& aPrintData) const;
};

//-------------------------------------------------------------------------------------------------
// If you define your Validate and Print functions as templates then you can use this helper when registering them
//-------------------------------------------------------------------------------------------------
#define MSF_MakeCustomPrinter(Validate, Print) { Validate<char>, Validate<char16_t>, Validate<char32_t>, Print<char>, Print<char16_t>, Print<char32_t> }

template<> inline size_t MSF_CustomPrinter::Validate<char>(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue) const { return ValidateUTF8(aPrintData, aValue); }
template<> inline size_t MSF_CustomPrinter::Validate<char16_t>(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue) const { return ValidateUTF16(aPrintData, aValue); }
template<> inline size_t MSF_CustomPrinter::Validate<char32_t>(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue) const { return ValidateUTF32(aPrintData, aValue); }

template<> inline size_t MSF_CustomPrinter::Print<char>(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aPrintData) const { return PrintUTF8(aBuffer, aBufferEnd, aPrintData); }
template<> inline size_t MSF_CustomPrinter::Print<char16_t>(char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aPrintData) const { return PrintUTF16(aBuffer, aBufferEnd, aPrintData); }
template<> inline size_t MSF_CustomPrinter::Print<char32_t>(char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aPrintData) const { return PrintUTF32(aBuffer, aBufferEnd, aPrintData); }

namespace MSF_CustomPrint
{
	//-------------------------------------------------------------------------------------------------
	// verify format/type  and return max length in read mode, write mode must not fail and return number of characters written
	// reserved characters:
	// 'c', 'C', 's', 'S', 'd', 'i', 'o', 'u', 'x', 'X', 'p', 'P', 'e', 'E', 'f', 'g', 'G', 'a', 'A', 'n'
	//-------------------------------------------------------------------------------------------------
	void RegisterPrintFunction(char aChar, uint64_t someSupportedTypes, MSF_CustomPrinter aPrinter);

	//-------------------------------------------------------------------------------------------------
	// Register print function and set as the default printing function for the types for c#/python style printing
	//-------------------------------------------------------------------------------------------------
	void RegisterDefaultPrintFunction(char aChar, uint64_t someSupportedTypes, MSF_CustomPrinter aPrinter);

	//-------------------------------------------------------------------------------------------------
	// Set the default print function for a set of types
	//-------------------------------------------------------------------------------------------------
	void RegisterTypesDefaultChar(uint64_t someSupportedTypes, char aChar);

	//-------------------------------------------------------------------------------------------------
	// Force the default print function for a set of types to a new value
	//-------------------------------------------------------------------------------------------------
	void OverrideTypesDefaultChar(uint64_t someSupportedTypes, char aChar);

	//-------------------------------------------------------------------------------------------------
	// Copy length handling for types. This is only required for types that are referenced by pointer.
	//-------------------------------------------------------------------------------------------------
	void RegisterTypeCopyLength(char aChar, size_t(*aCopyLength)(MSF_StringFormatType const& aValue));

	//-------------------------------------------------------------------------------------------------
	// Useful functon for recursive types, returns maximum length needed
	// Validate might affect print data depending on type
	//-------------------------------------------------------------------------------------------------
	size_t ValidateTypeUTF8(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t ValidateTypeUTF16(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t ValidateTypeUTF32(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);

	size_t PrintType(char aChar, char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, MSF_StringFormatType const& aValue);
	size_t PrintType(char aChar, char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aData, MSF_StringFormatType const& aValue);
	size_t PrintType(char aChar, char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aData, MSF_StringFormatType const& aValue);

	template <typename Char>
	size_t ValidateType(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);

	//-------------------------------------------------------------------------------------------------
	// Set how you want string formats to handle errors. Can be set globaller or on a per thread basis
	//-------------------------------------------------------------------------------------------------
	enum ErrorMode
	{
		Silent,			// Format calls will return <0 and print nothing
		WriteString,	// Format calls will write as much error info into string bufer as possible
		Assert,			// Format calls will assert with error information
		UseGlobal,		// For thread local states, will use whatever the global is
	};
	void SetGlobalErrorMode(ErrorMode aMode);
	void SetLocalErrorMode(ErrorMode aMode);
	void ClearLocalErrorMode();
};

template<> inline size_t MSF_CustomPrint::ValidateType<char>(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue) { return MSF_CustomPrint::ValidateTypeUTF8(aChar, aPrintData, aValue); }
template<> inline size_t MSF_CustomPrint::ValidateType<char16_t>(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue) { return MSF_CustomPrint::ValidateTypeUTF8(aChar, aPrintData, aValue); }
template<> inline size_t MSF_CustomPrint::ValidateType<char32_t>(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue) { return MSF_CustomPrint::ValidateTypeUTF8(aChar, aPrintData, aValue); }
