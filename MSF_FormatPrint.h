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
	PRINT_SIGN = 0x02,		// '+' show + sign when positive
	PRINT_BLANK = 0x04,		// ' ' prefix with spaces / show space when positive
	PRINT_LEFTALIGN = 0x08,		// '-' invalidates the zero option
	PRINT_ZERO = 0x10,		// '0' prefix with zeros
	PRINT_PRECISION = 0x20,		// '.' notifies that precision was specified
};

//-------------------------------------------------------------------------------------------------
// Print Data describes all the extra information passed with the type to be printed
//-------------------------------------------------------------------------------------------------
struct MSF_PrintData
{
	MSF_StringFormatType const* myValue; // the format type specified

	char const* myStart; // start of %<type> statement (including the '%')
	char const* myEnd; // end of the %<type> statement, not set in initial func call

	// to be written to in read mode
	size_t myMaxLength; // max length required, can be an estimate but must never be too low.

	// store any data you may need later
	mutable size_t myUserData;

	// %<width>.<precision>
	uint16_t myWidth;
	uint16_t myPrecision;

	// see MSF_PrintFlags
	uint8_t myFlags;

	// format to use for printing
	char myPrintChar;
};

namespace MSF_CustomPrint
{
	//-------------------------------------------------------------------------------------------------
	// Final validation of print info. Make any final adjustments and return the maximum length you might
	// need to print.
	// Do not set the max length directly.
	//-------------------------------------------------------------------------------------------------
	typedef size_t(*ValidateFunc)(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	//-------------------------------------------------------------------------------------------------
	// Print out your string to the output buffer and return characters printed
	//-------------------------------------------------------------------------------------------------
	typedef size_t(*PrintFunc)(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aPrintData);
	//-------------------------------------------------------------------------------------------------
	// verify format/type  and return max length in read mode, write mode must not fail and return number of characters written
	// reserved characters:
	// 'c', 'C', 's', 'S', 'd', 'i', 'o', 'u', 'x', 'X', 'p', 'P', 'e', 'E', 'f', 'g', 'G'
	//-------------------------------------------------------------------------------------------------
	void RegisterPrintFunction(char aChar, uint64_t someSupportedTypes, ValidateFunc aValidateFunc, PrintFunc aPrintFunc);

	//-------------------------------------------------------------------------------------------------
	// Register print function and set as the default printing function for the types for c#/python style printing
	//-------------------------------------------------------------------------------------------------
	void RegisterDefaultPrintFunction(char aChar, uint64_t someSupportedTypes, ValidateFunc aValidateFunc, PrintFunc aPrintFunc);

	//-------------------------------------------------------------------------------------------------
	// Override the default print function for a set of types
	//-------------------------------------------------------------------------------------------------
	void RegisterTypesDefaultChar(uint64_t someSupportedTypes, char aChar);

	//-------------------------------------------------------------------------------------------------
	// Useful functon for recursive types, returns maximum length needed
	// Validate might affect print data depending on type
	//-------------------------------------------------------------------------------------------------
	size_t ValidateType(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue);
	size_t PrintType(char aChar, char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, MSF_StringFormatType const& aValue);

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
