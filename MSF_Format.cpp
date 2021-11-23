#include "MSF_FormatStandardTypes.h"
#include "MSF_Assert.h"
#include "MSF_Utilities.h"

//-------------------------------------------------------------------------------------------------
// Types of errors that can occur during print validation
//-------------------------------------------------------------------------------------------------
enum MSF_PrintResultType
{
	ER_NotEnoughSpace,
	ER_AllocationFailed,
	ER_TooManyInputs,
	ER_TooManyPrints,
	ER_InconsistentPrintType,
	ER_UnexpectedEnd,
	ER_InvalidPrintCharacter,
	ER_TypeMismatch,
	ER_UnregisteredChar,

	// csharp errors
	ER_UnexpectedBrace,
	ER_ExpectedIndex,
	ER_IndexOutOfRange,
	ER_ExpectedWidth,
	ER_ExpectedClosingBrace,
	ER_UnsupportedType,

	ER_Count
};

static char const* thePrintErrors[] =
{
	"ER_NotEnoughSpace: {2} chars was not enough space",
	"ER_AllocationFailed: Failed to allocation additional memory for {2} chars",
	"ER_TooManyInputs: Too many inputs {0} to printf system. Can only support 32",
	"ER_TooManyPrints: Too many print statements in string {0}",
	"ER_InconsistentPrintType: Inconsistent print type '{0}' at {1}. 1=Auto(%% or {{}}), 2=Specific({{X}}). Strings shouldn't mix modes.",
	"ER_UnexpectedEnd: Unexpected end of format string",
	"ER_InvalidPrintCharacter: Invalid print character '{0:c}' at {1}. Only lower and upper case ascii letters supported",
	"ER_TypeMismatch: Type mismatch for print character '{0:c}' at {1}",
	"ER_UnregisteredChar: Unregistered print character  '{0:c}' at {1}",

	"ER_UnexpectedBrace: Unexpected '}}' at {1}. Did you forget to double up your braces?",
	"ER_ExpectedIndex: Expected an index at {1}",
	"ER_IndexOutOfRange: Requested print index is {1} is out of range",
	"ER_ExpectedWidth: Expected a width value at {1}",
	"ER_ExpectedClosingBrace: Expected a closing brace '}}' at {1}",
	"ER_UnsupportedType: Unsupported type {0:x} at {1}",
};

//-------------------------------------------------------------------------------------------------
// Structure holding error information or length on success
//-------------------------------------------------------------------------------------------------
struct MSF_PrintResult
{
	explicit MSF_PrintResult(size_t aNumChars) : MaxBufferLength(aNumChars) {}
	explicit MSF_PrintResult(MSF_PrintResultType anError, uint64_t anErrorInfo = 0, uint64_t anErrorLocation = 0)
		: MaxBufferLength(-(anError + 1))
		, Error(anError)
		, Info(anErrorInfo)
		, Location(anErrorLocation)
	{
	}

	intptr_t ToString(char(& anErrorBuffer)[256], size_t aBufferLength) const
	{
		static_assert((sizeof(thePrintErrors) / sizeof(thePrintErrors[0])) == ER_Count, "Number of error messages does not match number of error codes");
		MSF_ASSERT(MaxBufferLength < 0);
		return MSF_Format(anErrorBuffer, thePrintErrors[Error], Info, Location, aBufferLength);
	}

	intptr_t MaxBufferLength;
	MSF_PrintResultType Error;
	uint64_t Info;
	uint64_t Location;
};
//-------------------------------------------------------------------------------------------------
// Customized print interface, holds registered information about types that can be printed
//-------------------------------------------------------------------------------------------------
namespace MSF_CustomPrint
{
	//-------------------------------------------------------------------------------------------------
	// For each printf character registered we have a set of types that can be used as well
	// as a validate and print functions.
	//-------------------------------------------------------------------------------------------------
	struct RegisteredChar
	{
		ValidateFunc Validate;
		PrintFunc Print;
		uint64_t SupportedTypes;
	};

	// a-z + A-Z
	RegisteredChar theRegisteredChars[52];

	// Currently we have a 32 limit based on bit field of registered types
	char theDefaultPrintCharacters[64];

	// helpers to decode character to a 0-52 index
	int GetCharIndex(char aChar)
	{
		MSF_ASSERT(MSF_IsAsciiAlphaNumeric(aChar), "Invalid print character '%c'. Must be a-z or A-Z.", aChar);
		int offset = aChar < 'a' ? 'A' - 26 : 'a';
		return aChar - offset;
	}

	// helper to iterate bits in integer
	int GetFirstSetBit(uint64_t aValue)
	{
#if _MSC_VER
#if INTPTR_MAX == INT32_MAX
		extern unsigned char ::_BitScanForward(unsigned long* _Index, unsigned long _Mask);

		unsigned long result;
		if (_BitScanForward(&result, (unsigned long)aValue))
			return result;
		if (_BitScanForward(&result, (unsigned long)(aValue >> 32)))
			return result + 32;
		return -1;
#else
		extern unsigned char ::_BitScanForward64(unsigned long* _Index, unsigned __int64 _Mask);

		unsigned long result;
		if (_BitScanForward64(&result, aValue))
			return result;
		return -1;
#endif
#else
		return aValue ? __builtin_ctzll(aValue) : -1;
#endif
	}

	//-------------------------------------------------------------------------------------------------
	void RegisterPrintFunction(char aChar, uint64_t someSupportedTypes, ValidateFunc aValidateFunc, PrintFunc aPrintFunc)
	{
		MSF_ASSERT(someSupportedTypes != 0 && aValidateFunc != nullptr && aPrintFunc != nullptr, "Invalid arguments");

		int index = GetCharIndex(aChar);
		RegisteredChar& registered = theRegisteredChars[index];

		MSF_ASSERT(registered.Validate == nullptr || registered.Validate == aValidateFunc,
			"Custom print function for '%c' already registered", aChar);
		MSF_ASSERT(registered.Print == nullptr || registered.Print == aPrintFunc,
			"Custom print function for '%c' already registered", aChar);
		MSF_ASSERT(registered.SupportedTypes == 0 || registered.SupportedTypes == someSupportedTypes,
			"Custom print function for '%c' already registered", aChar);

		registered.Validate = aValidateFunc;
		registered.Print = aPrintFunc;
		registered.SupportedTypes = someSupportedTypes;
	}
	//-------------------------------------------------------------------------------------------------
	void RegisterDefaultPrintFunction(char aChar, uint64_t someSupportedTypes, ValidateFunc aValidateFunc, PrintFunc aPrintFunc)
	{
		RegisterPrintFunction(aChar, someSupportedTypes, aValidateFunc, aPrintFunc);
		RegisterTypesDefaultChar(someSupportedTypes, aChar);
	}
	//-------------------------------------------------------------------------------------------------
	void RegisterTypesDefaultChar(uint64_t someSupportedTypes, char aChar)
	{
		int index = GetFirstSetBit(someSupportedTypes);
		MSF_ASSERT(index >= 0);
		while (index >= 0)
		{
			theDefaultPrintCharacters[index] = aChar;

			someSupportedTypes &= ~(1 << index);
			index = GetFirstSetBit(someSupportedTypes);
		}
	}

	//-------------------------------------------------------------------------------------------------
	// For a given type, get the character used to print it by default.
	// Special case for signed integers and char type.
	//-------------------------------------------------------------------------------------------------
	char GetTypeDefaultPrintCharacters(MSF_StringFormatType const& aValue)
	{
		if (aValue.myType < MSF_StringFormatType::TypeUser)
		{
			// special hacks
			if (aValue.myUserData & MSF_StringFormatType::Char)
				return 'c';
			if (aValue.myUserData & MSF_StringFormatType::Pointer)
				return 'p';
		}

		return theDefaultPrintCharacters[GetFirstSetBit(aValue.myType)];
	}
	//-------------------------------------------------------------------------------------------------
	// Initialize standard printf types
	//-------------------------------------------------------------------------------------------------
	bool InitializeCustomPrint()
	{
		RegisterDefaultPrintFunction('c', MSF_StringFormatChar::ValidTypes, MSF_StringFormatChar::Validate, MSF_StringFormatChar::Print);
		RegisterPrintFunction('C', MSF_StringFormatChar::ValidTypes, MSF_StringFormatChar::Validate, MSF_StringFormatChar::Print);

		RegisterDefaultPrintFunction('s', MSF_StringFormatString::ValidTypes, MSF_StringFormatString::Validate, MSF_StringFormatString::Print);
		RegisterPrintFunction('S', MSF_StringFormatString::ValidTypes, MSF_StringFormatString::Validate, MSF_StringFormatString::Print);

		RegisterDefaultPrintFunction('d', MSF_StringFormatInt::ValidTypes, MSF_StringFormatInt::Validate, MSF_StringFormatInt::Print);
		RegisterPrintFunction('i', MSF_StringFormatInt::ValidTypes, MSF_StringFormatInt::Validate, MSF_StringFormatInt::Print);
		RegisterPrintFunction('u', MSF_StringFormatInt::ValidTypes, MSF_StringFormatInt::Validate, MSF_StringFormatInt::Print);
		RegisterPrintFunction('o', MSF_StringFormatInt::ValidTypes, MSF_StringFormatInt::ValidateOctal, MSF_StringFormatInt::Print);
		RegisterPrintFunction('x', MSF_StringFormatInt::ValidTypes, MSF_StringFormatInt::ValidateHex, MSF_StringFormatInt::Print);
		RegisterPrintFunction('X', MSF_StringFormatInt::ValidTypes, MSF_StringFormatInt::ValidateHex, MSF_StringFormatInt::Print);
		RegisterPrintFunction('p', MSF_StringFormatInt::ValidTypes, MSF_StringFormatInt::ValidatePointer, MSF_StringFormatInt::Print);
		RegisterPrintFunction('P', MSF_StringFormatInt::ValidTypes, MSF_StringFormatInt::ValidatePointer, MSF_StringFormatInt::Print);

		RegisterPrintFunction('e', MSF_StringFormatFloat::ValidTypes, MSF_StringFormatFloat::Validate, MSF_StringFormatFloat::Print);
		RegisterPrintFunction('E', MSF_StringFormatFloat::ValidTypes, MSF_StringFormatFloat::Validate, MSF_StringFormatFloat::Print);
		RegisterPrintFunction('f', MSF_StringFormatFloat::ValidTypes, MSF_StringFormatFloat::Validate, MSF_StringFormatFloat::Print);
		RegisterPrintFunction('F', MSF_StringFormatFloat::ValidTypes, MSF_StringFormatFloat::Validate, MSF_StringFormatFloat::Print);
		RegisterDefaultPrintFunction('g', MSF_StringFormatFloat::ValidTypes, MSF_StringFormatFloat::Validate, MSF_StringFormatFloat::Print);
		RegisterPrintFunction('G', MSF_StringFormatFloat::ValidTypes, MSF_StringFormatFloat::Validate, MSF_StringFormatFloat::Print);

		return true;
	}

	bool theCustomPrintInitialized = InitializeCustomPrint();

	//-------------------------------------------------------------------------------------------------
	// Validate external assumed users are calling the correct types and will crash if invalid
	//-------------------------------------------------------------------------------------------------
	size_t ValidateType(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		RegisteredChar const& registered = theRegisteredChars[GetCharIndex(aChar)];
		return registered.Validate(aPrintData, aValue);
	}

	//-------------------------------------------------------------------------------------------------
	// Validate internal is safer than the external one since it depends on string format
	//-------------------------------------------------------------------------------------------------
	MSF_PrintResult ValidateType(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		RegisteredChar const& registered = theRegisteredChars[GetCharIndex(aPrintData.myPrintChar)];
		if (!registered.Validate)
			return MSF_PrintResult(ER_UnregisteredChar, aPrintData.myPrintChar);

		if (!(registered.SupportedTypes & aValue.myType))
			return MSF_PrintResult(ER_TypeMismatch, aPrintData.myPrintChar);

		size_t maxLength = registered.Validate(aPrintData, aValue);
		MSF_ASSERT(maxLength >= 0, "Validates can't fail");

		aPrintData.myMaxLength = maxLength;
		return MSF_PrintResult(maxLength);
	}
	//-------------------------------------------------------------------------------------------------
	// Print a character type, useful for chaining custom types into standard types
	//-------------------------------------------------------------------------------------------------
	size_t PrintType(char aChar, char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, MSF_StringFormatType const& aValue)
	{
		RegisteredChar const& registered = theRegisteredChars[GetCharIndex(aChar)];

		MSF_PrintData tmp = aData;
		tmp.myPrintChar = aChar;
		tmp.myValue = &aValue;
		return registered.Print(aBuffer, aBufferEnd, tmp);
	}
	//-------------------------------------------------------------------------------------------------
	// Error Handling
	//-------------------------------------------------------------------------------------------------
	ErrorMode theGlobalErrorMode = Silent;
	ErrorMode thread_local theLocalErrorMode = UseGlobal;

	void SetGlobalErrorMode(ErrorMode aMode)
	{
		theGlobalErrorMode = aMode;
	}
	void SetLocalErrorMode(ErrorMode aMode)
	{
		MSF_ASSERT(theLocalErrorMode == UseGlobal, "Local error mode already in use");
		theLocalErrorMode = aMode;
	}
	void ClearLocalErrorMode()
	{
		theLocalErrorMode = UseGlobal;
	}

	ErrorMode GetErrorMode()
	{
		ErrorMode localMode = theLocalErrorMode;
		return localMode == UseGlobal ? theGlobalErrorMode : localMode;
	}

	//-------------------------------------------------------------------------------------------------
	// Normal printf
	//-------------------------------------------------------------------------------------------------
	MSF_PrintResult SetupPrintfInfo(MSF_PrintData& aPrintData, char const*& anInput)
	{
		// gather the parts
		// [flags] [width] [.precision] type
		// using the similar standards as Microsoft's printf
		// we don't need additional size specifiers since the size
		// is known in the format type.  this also frees up
		// more room for custom types

		bool leadingZero = true; // to know if we found a 0 before or after other numbers
		uint32_t width = 0;
		uint32_t precision = 0;
		for (char character = *anInput++; character; character = *anInput++)
		{
			// read until non-flag is found
			switch (character)
			{
			case '-': aPrintData.myFlags |= PRINT_LEFTALIGN; aPrintData.myFlags &= ~PRINT_ZERO; break;
			case '+': aPrintData.myFlags |= PRINT_SIGN; break;
			case ' ': aPrintData.myFlags |= PRINT_BLANK; break;
			case '#': aPrintData.myFlags |= PRINT_PREFIX; break;
			case '.': aPrintData.myFlags |= PRINT_PRECISION; break;
			case '0':
				if (leadingZero)
				{
					if (!(aPrintData.myFlags & PRINT_LEFTALIGN))
					{
						aPrintData.myFlags |= PRINT_ZERO;
					}
					break;
				}
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				leadingZero = false;
				if (!(aPrintData.myFlags & PRINT_PRECISION))
				{
					width = (width * 10) + (character - '0');
				}
				else
				{
					precision = (precision * 10) + (character - '0');
				}
				break;
			case 'I': // look for I[(32|64)](d|i|o|u|x|X)
				{
					int advance = 0;
					if ((anInput[0] == '3' && anInput[1] == '2') ||
						(anInput[0] == '6' && anInput[1] == '4'))
					{
						advance = 2;
					}

					char const next = anInput[advance];
					if (next == 'd' || next == 'i' || next == 'o' || next == 'u' || next == 'x' || next == 'X')
					{
						character = next;
						anInput += advance + 1;
					}
				}
				goto do_print;

			case 'h': // look for (h|hh)(d|i|o|u|x|X|n) or h(s|c)
				if (anInput[0] == 'h')
				{
					char const next = anInput[1];
					if (next == 'd' || next == 'i' || next == 'o' || next == 'u' || next == 'x' || next == 'X' || next == 'n')
					{
						// skip hh
						character = next;
						anInput += 2;
					}
				}
				else
				{
					char const next = anInput[0];
					if (next == 'd' || next == 'i' || next == 'o' || next == 'u' || next == 'x' || next == 'X' || next == 'n' ||
						next == 's' || next == 'c')
					{
						// skip h
						character = next;
						anInput += 1;
					}
				}
				goto do_print;

			case 'l': // look for (l|ll)(d|i|o|u|x|X|n) or l(s|c|f|F|e|E|a|A|g|G)
				if (anInput[0] == 'l')
				{
					char const next = anInput[1];
					if (next == 'd' || next == 'i' || next == 'o' || next == 'u' || next == 'x' || next == 'X' || next == 'n')
					{
						// skip ll
						character = next;
						anInput += 2;
					}
				}
				else
				{
					char const next = anInput[0];
					if (next == 'd' || next == 'i' || next == 'o' || next == 'u' || next == 'x' || next == 'X' || next == 'n' ||
						next == 's' || next == 'c' ||
						next == 'f' || next == 'F' || next == 'e' || next == 'E' || next == 'a' || next == 'A' || next == 'g' || next == 'G')
					{
						// skip l
						character = next;
						anInput += 1;
					}
				}
				goto do_print;

			case 'j':
			case 't':
			case 'z': // look for (j|t|z)(d|i|o|u|x|X)
			{
				char const next = anInput[0];
				if (next == 'd' || next == 'i' || next == 'o' || next == 'u' || next == 'x' || next == 'X')
				{
					// skip l
					character = next;
					anInput += 1;
				}
			}
				goto do_print;

			case 'L': // look for L(f|F|e|E|a|A|g|G)
			{
				char const next = anInput[0];
				if (next == 'f' || next == 'F' || next == 'e' || next == 'E' || next == 'a' || next == 'A' || next == 'g' || next == 'G')
				{
					// skip l
					character = next;
					anInput += 1;
				}
			}
				goto do_print;

			case 'w': // look for w(s|c)
			{
				char const next = anInput[0];
				if (next == 's' || next == 'c')
				{
					// skip l
					character = next;
					anInput += 1;
				}
			}
				goto do_print;

			default:
				if (!MSF_IsAsciiAlphaNumeric(character))
				{
					return MSF_PrintResult(ER_InvalidPrintCharacter, character);
				}
				else
				{
					do_print:

					aPrintData.myPrintChar = character;

					static_assert(sizeof(aPrintData.myWidth) == 2, "sizeof myWidth changed, update code");
					MSF_ASSERT(width < UINT16_MAX);
					aPrintData.myWidth = (uint16_t)width;

					static_assert(sizeof(aPrintData.myPrecision) == 2, "sizeof myPrecision changed, update code");
					MSF_ASSERT(precision < UINT16_MAX);
					aPrintData.myPrecision = (uint16_t)precision;

					return ValidateType(aPrintData, *aPrintData.myValue);
				}
			}
		}

		return MSF_PrintResult(ER_UnexpectedEnd);
	}

	//-------------------------------------------------------------------------------------------------
	// CSharp/hybrid style
	//-------------------------------------------------------------------------------------------------
	MSF_PrintResult SetupFormatInfo(MSF_PrintData& aPrintData, char const*& anInput)
	{
		// gather the parts
		// https://docs.microsoft.com/en-us/dotnet/api/system.string.format?view=netframework-4.8#controlling-formatting
		// {[index][,alignment][:formatString]} 

		// Read alignment / width
		if (*anInput == ',')
		{
			++anInput;
			if (*anInput == '-')
			{
				aPrintData.myFlags |= PRINT_LEFTALIGN;
				++anInput;
			}

			if (!MSF_IsDigit(*anInput))
				return MSF_PrintResult(ER_ExpectedWidth, 0);

			uint32_t width = *(anInput++) - '0';
			while (MSF_IsDigit(*anInput))
				width = width * 10 + *(anInput++) - '0';

			static_assert(sizeof(aPrintData.myWidth) == 2, "sizeof myWidth changed, update code");
			MSF_ASSERT(width < UINT16_MAX);
			aPrintData.myWidth = (uint16_t)width;
		}

		// Read formatting
		if (*anInput == ':')
		{
			++anInput;
			if (!MSF_IsAsciiAlphaNumeric(*anInput))
				return MSF_PrintResult(ER_InvalidPrintCharacter, aPrintData.myValue->myType, 0);

			aPrintData.myPrintChar = *(anInput++);

			uint32_t precision = 0;
			while (MSF_IsDigit(*anInput))
				precision = precision * 10 + *(anInput++) - '0';

			static_assert(sizeof(aPrintData.myPrecision) == 2, "sizeof myPrecision changed, update code");
			MSF_ASSERT(precision < UINT16_MAX);
			aPrintData.myPrecision = (uint16_t)precision;
		}
		else
		{
			aPrintData.myPrintChar = GetTypeDefaultPrintCharacters(*aPrintData.myValue);
			if (!aPrintData.myPrintChar)
				return MSF_PrintResult(ER_UnsupportedType, aPrintData.myValue->myType, 0);
		}

		if (*anInput != '}')
			return MSF_PrintResult(ER_ExpectedClosingBrace, 0);

		++anInput;

		return ValidateType(aPrintData, *aPrintData.myValue);
	}
}

//-------------------------------------------------------------------------------------------------
// Helper class for constructing a formatted string
//-------------------------------------------------------------------------------------------------
class MSF_StringFormatter
{
public:
	enum
	{
		MaxArguments = 32,
	};

	MSF_StringFormatter() : myPrintedCharacters(0) {}

	MSF_PrintResult PrepareFormatter(MSF_StringFormat const& aStringFormat)
	{
		uint32_t const inputCount = aStringFormat.NumArgs();
		if (inputCount > MaxArguments)
			return MSF_PrintResult(ER_TooManyInputs, inputCount, 0);

		uint32_t inputIndex = 0;
		MSF_StringFormatType const* aData = aStringFormat.GetArgs();
		
		myPrintString = aStringFormat.GetString();
		char const* str = myPrintString;

		enum Mode { None, Auto, Specific };
		Mode printMode = None;
		char character;

		for ((character = *str++); character; (character = *str++))
		{
			switch (character)
			{
			case '%':
			case '{':
			{
				switch (*str)
				{
				case '\0':
					return MSF_PrintResult(ER_UnexpectedEnd, 0, 0);
				case '%':
				case '{':
					if (character != *str)
						return MSF_PrintResult(ER_InvalidPrintCharacter, *str, int(str - myPrintString));
					++str;
					break;
				default:
					// prepare print information
					if (myPrintedCharacters == MaxArguments)
						return MSF_PrintResult(ER_TooManyPrints, myPrintedCharacters, int(str - myPrintString));

					MSF_PrintData& printData = myPrintData[myPrintedCharacters++];
					printData.myFlags = 0; // initialize the whole first block
					printData.myPrecision = 0;
					printData.myWidth = 0;
					printData.myStart = str - 1;// account for '%' or '{'

					// look for position argument and print mode
					Mode thisMode = Auto;
					if (character == '{')
					{
						// Read Index
						if (MSF_IsDigit(*str))
						{
							thisMode = Specific;

							inputIndex = *(str++) - '0';
							while (MSF_IsDigit(*str))
								inputIndex = inputIndex * 10 + *(str++);
						}
					}

					// Check for inconsistent print modes
					if (thisMode != printMode)
					{
						if (printMode == None)
							printMode = thisMode;
						else 
							return MSF_PrintResult(ER_InconsistentPrintType, printMode, int(printData.myStart - myPrintString));
					}

					if (inputIndex >= inputCount)
						return MSF_PrintResult(ER_IndexOutOfRange, inputIndex);

					printData.myValue = aData + inputIndex;

					// Post increment in auto mode
					if (printMode == Auto)
					{
						++inputIndex;
					}
					
					// Process any format specifiers
					MSF_PrintResult result = character == '%' ? MSF_CustomPrint::SetupPrintfInfo(printData, str) : MSF_CustomPrint::SetupFormatInfo(printData, str);

					if (result.MaxBufferLength < 0)
					{
						result.Location = int(str - myPrintString);
						return result;
					}

					printData.myEnd = str;
					break;
				}
				break;
			}
			case '}':
				if (*str != '}')
					return MSF_PrintResult(ER_UnexpectedBrace, 0, int(str - myPrintString));
				++str;
				break;
			}
		}

		// we actually end up 1 past the end of the string which is actually ok count wise
		MSF_HEAVY_ASSERT(!*(str-1));

		// get the max size required
		size_t requiredLength = str - myPrintString;
		for (uint32_t i = 0; i < myPrintedCharacters; ++i)
		{
			requiredLength += myPrintData[i].myMaxLength - (myPrintData[i].myEnd - myPrintData[i].myStart);
		}

		return MSF_PrintResult(requiredLength);
	}

	size_t FormatString(char* aBuffer, size_t aBufferLength) const
	{
		char const* start = aBuffer;
		char const* end = aBuffer + aBufferLength;
		char const* read = myPrintString;
		for (uint32_t i = 0; i < myPrintedCharacters; ++i)
		{
			// copy segment between print markers
			if (myPrintData[i].myStart > read)
			{
				while (read != myPrintData[i].myStart)
				{
					// if a print character is found, that's because there's two, skip one
					if (*read == '%' || *read == '{' || *read == '}')
					{
						++read;
					}

					MSF_HEAVY_ASSERT(aBuffer < end);
					*aBuffer++ = *read++;
				}
			}

			int index = MSF_CustomPrint::GetCharIndex(myPrintData[i].myPrintChar);
			aBuffer += MSF_CustomPrint::theRegisteredChars[index].Print(aBuffer, end, myPrintData[i]);
			MSF_ASSERT(aBuffer < end);

			read = myPrintData[i].myEnd;
		}

		while (*read)
		{
			// if a '%' is found, that's because there's two, skip one
			if (*read == '%' || *read == '{' || *read == '}')
			{
				++read;
			}

			MSF_HEAVY_ASSERT(aBuffer < end);
			*aBuffer++ = *read++;
		}

		MSF_HEAVY_ASSERT(aBuffer < end);
		*aBuffer = '\0';

		// Don't include the null terminator since we want character printed, not bytes used
		return aBuffer - start;
	}

	void ProcessError(MSF_PrintResult anError, char* aBuffer, size_t aBufferLength, size_t anOffset, char* (*aReallocFunction)(char*, size_t, void*), void* aUserData)
	{
		MSF_CustomPrint::ErrorMode errorMode = MSF_CustomPrint::GetErrorMode();
		char errorMessage[256];
		size_t errorLength = anError.ToString(errorMessage, aBufferLength);

		// Null terminate buffer if possible
		if (aBuffer && aBufferLength)
		{
			aBuffer[0] = 0;
		}

		if (errorMode == MSF_CustomPrint::Assert)
		{
#if MSF_ASSERTS_ENABLED
			if (!MSF_IsAsserting())
			{
				MSF_ASSERT(false, "Error formatting string: %s", errorMessage);
			}
#endif
		}
		else if (errorMode == MSF_CustomPrint::WriteString)
		{
			if (errorLength + anOffset > aBufferLength)
			{
				if (aReallocFunction)
				{
					aBuffer = aReallocFunction(aBuffer, errorLength + anOffset, aUserData);
					aBufferLength = errorLength + anOffset;

					if (aBuffer == nullptr)
					{
						return;
					}
				}
				else
				{
					errorLength = aBufferLength - anOffset;
				}
			}
			MSF_CopyChars(aBuffer + anOffset, aBuffer + aBufferLength, errorMessage, errorLength);
		}
		
		// else do nothing
	}

private:

	MSF_PrintData myPrintData[MaxArguments];
	char const* myPrintString;
	size_t myPrintedCharacters;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
intptr_t MSF_FormatString(MSF_StringFormatTemplate<char> const& aStringFormat, char* aBuffer, size_t aBufferLength, size_t anOffset, char* (*aReallocFunction)(char*, size_t, void*), void* aUserData)
{
	MSF_StringFormatter formatter;
	MSF_PrintResult result = formatter.PrepareFormatter(aStringFormat);

	if (result.MaxBufferLength < 0)
	{
		formatter.ProcessError(result, aBuffer, aBufferLength, anOffset, aReallocFunction, aUserData);
		return -1;
	}

	if (result.MaxBufferLength + anOffset > aBufferLength || aBuffer == nullptr)
	{
		if (aReallocFunction == nullptr)
		{
			formatter.ProcessError(MSF_PrintResult(ER_NotEnoughSpace), aBuffer, aBufferLength, anOffset, aReallocFunction, aUserData);
			return -1;
		}

		aBuffer = aReallocFunction(aBuffer, result.MaxBufferLength + anOffset, aUserData);
		if (aBuffer == nullptr)
		{
			formatter.ProcessError(MSF_PrintResult(ER_AllocationFailed), aBuffer, aBufferLength, anOffset, aReallocFunction, aUserData);
			return -1;
		}

		aBufferLength = result.MaxBufferLength + anOffset;
	}

	// process string
	size_t printed = formatter.FormatString(aBuffer + anOffset, aBufferLength - anOffset);
	MSF_ASSERT(printed <= (size_t)result.MaxBufferLength - 1);
	return printed;
}

template class MSF_StringFormatTemplate<char>;
template class MSF_StringFormatTemplate<wchar_t>;
