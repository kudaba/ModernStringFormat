#include "MSF_FormatStandardTypes.h"
#include "MSF_Assert.h"
#include "MSF_UTF.h"
#include "MSF_Utilities.h"
#include <new>

#if _MSC_VER
#define STRICT
#define WIN32_LEANER_AND_MEANER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

template class MSF_StringFormatTemplate<char>;
template class MSF_StringFormatTemplate<char8_t>;
template class MSF_StringFormatTemplate<char16_t>;
template class MSF_StringFormatTemplate<char32_t>;
template class MSF_StringFormatTemplate<wchar_t>;

//-------------------------------------------------------------------------------------------------
// Types of errors that can occur during print validation
//-------------------------------------------------------------------------------------------------
enum MSF_PrintResultType
{
	ER_NotEnoughSpace,
	ER_TooManyInputs,
	ER_InconsistentPrintType,
	ER_DuplicateFlag,
	ER_InvalidPrintCharacter,
	ER_TypeMismatch,
	ER_UnregisteredChar,
	ER_WildcardType,

	// csharp errors
	ER_UnexpectedBrace,
	ER_IndexOutOfRange,
	ER_ExpectedWidth,
	ER_ExpectedClosingBrace,
	ER_UnsupportedType,

	ER_Count
};

static char const* thePrintErrors[] =
{
	"ER_NotEnoughSpace: {2} chars was not enough space",
	"ER_TooManyInputs: Too many inputs {0} to printf system. Can only support " MSF_STR(MSF_MAX_ARGUMENTS),
	"ER_InconsistentPrintType: Inconsistent print type '{0}' at {1}. 1=Auto(%% or {{}}), 2=Specific({{X}}). Strings shouldn't mix modes.",
	"ER_DuplicateFlag: Duplicate Flag '{0:c}' found at {1}.",
	"ER_InvalidPrintCharacter: Invalid print character '{0:c}' at {1}. Only lower and upper case ascii letters supported",
	"ER_TypeMismatch: Type mismatch for print character '{0:c}' at {1}",
	"ER_UnregisteredChar: Unregistered print character  '{0:c}' at {1}",
	"ER_WildcardType: Incorrect type specified for wildcard flag at {1}",

	"ER_UnexpectedBrace: Unexpected '}}' at {1}. Did you forget to double up your braces?",
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

	bool HasError() const { return MaxBufferLength < 0; }

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
		MSF_CustomPrinter Printer;
		uint64_t SupportedTypes;
	};

	// a-z + A-Z
	RegisteredChar theRegisteredChars[52];

	// Currently we have a 32 limit based on bit field of registered types
	char theDefaultPrintCharacters[64];

	// helpers to decode character to a 0-52 index
	int GetCharIndex(char aChar)
	{
		MSF_ASSERT(MSF_IsAsciiAlpha(aChar), "Invalid print character '%c'. Must be a-z or A-Z.", aChar);
		int offset = aChar < 'a' ? 'A' - 26 : 'a';
		return aChar - offset;
	}

	// helper to iterate bits in integer
	int GetFirstSetBit(uint64_t aValue)
	{
#if _MSC_VER
#if INTPTR_MAX == INT32_MAX
		unsigned long result;
		if (_BitScanForward(&result, (unsigned long)aValue))
			return result;
		if (_BitScanForward(&result, (unsigned long)(aValue >> 32)))
			return result + 32;
		return -1;
#else
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
	void RegisterPrintFunction(char aChar, uint64_t someSupportedTypes, MSF_CustomPrinter aPrinter)
	{
		MSF_ASSERT(someSupportedTypes != 0, "Invalid arguments");
		MSF_ASSERT(
			aPrinter.ValidateUTF8 && aPrinter.ValidateUTF16 && aPrinter.ValidateUTF32 &&
			aPrinter.PrintUTF8 && aPrinter.PrintUTF16 && aPrinter.PrintUTF32, "Incomplete Printer");

		int index = GetCharIndex(aChar);
		RegisteredChar& registered = theRegisteredChars[index];

		MSF_ASSERT(registered.Printer.ValidateUTF8 == nullptr || registered.Printer.ValidateUTF8 == aPrinter.ValidateUTF8,
			"Custom print function for '%c' already registered", aChar);
		MSF_ASSERT(registered.Printer.ValidateUTF16 == nullptr || registered.Printer.ValidateUTF16 == aPrinter.ValidateUTF16,
			"Custom print function for '%c' already registered", aChar);
		MSF_ASSERT(registered.Printer.ValidateUTF32 == nullptr || registered.Printer.ValidateUTF32 == aPrinter.ValidateUTF32,
			"Custom print function for '%c' already registered", aChar);
		MSF_ASSERT(registered.Printer.PrintUTF8 == nullptr || registered.Printer.PrintUTF8 == aPrinter.PrintUTF8,
			"Custom print function for '%c' already registered", aChar);
		MSF_ASSERT(registered.Printer.PrintUTF16 == nullptr || registered.Printer.PrintUTF16 == aPrinter.PrintUTF16,
			"Custom print function for '%c' already registered", aChar);
		MSF_ASSERT(registered.Printer.PrintUTF32 == nullptr || registered.Printer.PrintUTF32 == aPrinter.PrintUTF32,
			"Custom print function for '%c' already registered", aChar);
		MSF_ASSERT(registered.SupportedTypes == 0 || registered.SupportedTypes == someSupportedTypes,
			"Custom print function for '%c' already registered", aChar);

		registered.Printer = aPrinter;
		registered.SupportedTypes = someSupportedTypes;
	}
	//-------------------------------------------------------------------------------------------------
	void RegisterDefaultPrintFunction(char aChar, uint64_t someSupportedTypes, MSF_CustomPrinter aPrinter)
	{
		RegisterPrintFunction(aChar, someSupportedTypes, aPrinter);
		RegisterTypesDefaultChar(someSupportedTypes, aChar);
	}
	//-------------------------------------------------------------------------------------------------
	void RegisterTypesDefaultChar(uint64_t someSupportedTypes, char aChar)
	{
		int index = GetFirstSetBit(someSupportedTypes);
		MSF_ASSERT(index >= 0);
		while (index >= 0)
		{
			MSF_ASSERT(theDefaultPrintCharacters[index] == 0 || theDefaultPrintCharacters[index] == aChar, "Type %d(%x) is already set to %c", index, 1 << index, aChar);
			if (theDefaultPrintCharacters[index] == 0)
				theDefaultPrintCharacters[index] = aChar;

			someSupportedTypes &= ~(1 << index);
			index = GetFirstSetBit(someSupportedTypes);
		}
	}
	//-------------------------------------------------------------------------------------------------
	void OverrideTypesDefaultChar(uint64_t someSupportedTypes, char aChar)
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
	//-------------------------------------------------------------------------------------------------
	void RegisterTypeCopyLength(char aChar, size_t(*aCopyLength)(MSF_StringFormatType const& aValue))
	{
		RegisteredChar& registered = theRegisteredChars[GetCharIndex(aChar)];
		MSF_ASSERT(registered.Printer.PrintUTF16, "Copy Length can only apply to already registered type");
		MSF_ASSERT(registered.Printer.CopyLength == nullptr || registered.Printer.CopyLength == aCopyLength);
		registered.Printer.CopyLength = aCopyLength;
	}
	//-------------------------------------------------------------------------------------------------
	size_t GetTypeCopyLength(MSF_StringFormatType const& aValue)
	{
		char const printChar = theDefaultPrintCharacters[GetFirstSetBit(aValue.myType)];
		RegisteredChar const& registered = theRegisteredChars[GetCharIndex(printChar)];
		return registered.Printer.CopyLength ? registered.Printer.CopyLength(aValue) : 0;
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
	bool InitializeCustomPrint();
	bool theCustomPrintInitialized = InitializeCustomPrint();

	bool InitializeCustomPrint()
	{
		if (theCustomPrintInitialized)
			return true;

		{
			MSF_CustomPrinter printer{
				MSF_StringFormatChar::ValidateUTF8, MSF_StringFormatChar::ValidateUTF16, MSF_StringFormatChar::ValidateUTF32,
				MSF_StringFormatChar::PrintUTF8, MSF_StringFormatChar::PrintUTF16, MSF_StringFormatChar::PrintUTF32
			};
			RegisterPrintFunction('c', MSF_StringFormatChar::ValidTypes, printer);
			RegisterPrintFunction('C', MSF_StringFormatChar::ValidTypes, printer);
		}

		{
			MSF_CustomPrinter printer{
				MSF_StringFormatString::ValidateUTF8, MSF_StringFormatString::ValidateUTF16, MSF_StringFormatString::ValidateUTF32,
				MSF_StringFormatString::PrintUTF8, MSF_StringFormatString::PrintUTF16, MSF_StringFormatString::PrintUTF32
			};
			RegisterDefaultPrintFunction('s', MSF_StringFormatString::ValidTypes, printer);
			RegisterPrintFunction('S', MSF_StringFormatString::ValidTypes, printer);
			RegisterTypeCopyLength('s', MSF_StringFormatString::CopyLength);
		}

		{
			MSF_CustomPrinter printer{
				MSF_StringFormatInt::Validate, MSF_StringFormatInt::Validate, MSF_StringFormatInt::Validate,
				MSF_StringFormatInt::PrintUTF8, MSF_StringFormatInt::PrintUTF16, MSF_StringFormatInt::PrintUTF32
			};
			RegisterDefaultPrintFunction('d', MSF_StringFormatInt::ValidTypes, printer);
			RegisterPrintFunction('i', MSF_StringFormatInt::ValidTypes, printer);
			RegisterPrintFunction('u', MSF_StringFormatInt::ValidTypes, printer);
		}

		{
			MSF_CustomPrinter printer{
				MSF_StringFormatInt::ValidateOctal, MSF_StringFormatInt::ValidateOctal, MSF_StringFormatInt::ValidateOctal,
				MSF_StringFormatInt::PrintUTF8, MSF_StringFormatInt::PrintUTF16, MSF_StringFormatInt::PrintUTF32
			};
			RegisterPrintFunction('o', MSF_StringFormatInt::ValidTypes, printer);
		}

		{
			MSF_CustomPrinter printer{
				MSF_StringFormatInt::ValidateHex, MSF_StringFormatInt::ValidateHex, MSF_StringFormatInt::ValidateHex,
				MSF_StringFormatInt::PrintUTF8, MSF_StringFormatInt::PrintUTF16, MSF_StringFormatInt::PrintUTF32
			};
			RegisterPrintFunction('x', MSF_StringFormatInt::ValidTypes, printer);
			RegisterPrintFunction('X', MSF_StringFormatInt::ValidTypes, printer);
		}

		{
			MSF_CustomPrinter printer{
				MSF_StringFormatInt::ValidatePointer, MSF_StringFormatInt::ValidatePointer, MSF_StringFormatInt::ValidatePointer,
				MSF_StringFormatInt::PrintUTF8, MSF_StringFormatInt::PrintUTF16, MSF_StringFormatInt::PrintUTF32
			};
			RegisterPrintFunction('p', MSF_StringFormatInt::ValidTypes, printer);
			RegisterPrintFunction('P', MSF_StringFormatInt::ValidTypes, printer);
		}

		{
			MSF_CustomPrinter printer{
				MSF_StringFormatFloat::Validate, MSF_StringFormatFloat::Validate, MSF_StringFormatFloat::Validate,
				MSF_StringFormatFloat::PrintUTF8, MSF_StringFormatFloat::PrintUTF16, MSF_StringFormatFloat::PrintUTF32
			};
			RegisterPrintFunction('e', MSF_StringFormatFloat::ValidTypes, printer);
			RegisterPrintFunction('E', MSF_StringFormatFloat::ValidTypes, printer);
			RegisterPrintFunction('f', MSF_StringFormatFloat::ValidTypes, printer);
			RegisterPrintFunction('F', MSF_StringFormatFloat::ValidTypes, printer);
			RegisterDefaultPrintFunction('g', MSF_StringFormatFloat::ValidTypes, printer);
			RegisterPrintFunction('G', MSF_StringFormatFloat::ValidTypes, printer);
		}

		return true;
	}

	//-------------------------------------------------------------------------------------------------
	// Validate external assumed users are calling the correct types and will crash if invalid
	//-------------------------------------------------------------------------------------------------
	size_t ValidateTypeUTF8(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		RegisteredChar const& registered = theRegisteredChars[GetCharIndex(aChar)];
		return registered.Printer.ValidateUTF8(aPrintData, aValue);
	}
	size_t ValidateTypeUTF16(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		RegisteredChar const& registered = theRegisteredChars[GetCharIndex(aChar)];
		return registered.Printer.ValidateUTF16(aPrintData, aValue);
	}
	size_t ValidateTypeUTF32(char aChar, MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		RegisteredChar const& registered = theRegisteredChars[GetCharIndex(aChar)];
		return registered.Printer.ValidateUTF32(aPrintData, aValue);
	}

	//-------------------------------------------------------------------------------------------------
	// Validate internal is safer than the external one since it depends on string format
	//-------------------------------------------------------------------------------------------------
	template <typename Char>
	MSF_PrintResult ValidateTypeShared(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		RegisteredChar const& registered = theRegisteredChars[GetCharIndex(aPrintData.myPrintChar)];
		if (!registered.Printer.ValidateUTF8)
		{
			// If called pre-main then we might be be initialized yet, so just do that now!
			if (!theRegisteredChars[GetCharIndex('c')].Printer.ValidateUTF8)
				theCustomPrintInitialized = InitializeCustomPrint();

			if (!registered.Printer.ValidateUTF8)
				return MSF_PrintResult(ER_UnregisteredChar, aPrintData.myPrintChar);
		}
		if (!(registered.SupportedTypes & aValue.myType))
			return MSF_PrintResult(ER_TypeMismatch, aPrintData.myPrintChar);

		size_t maxLength = registered.Printer.Validate<Char>(aPrintData, aValue);
		MSF_ASSERT(maxLength >= 0, "Validates can't fail");

		aPrintData.myMaxLength = maxLength;
		return MSF_PrintResult(maxLength);
	}
	//-------------------------------------------------------------------------------------------------
	// Print a character type, useful for chaining custom types into standard types
	//-------------------------------------------------------------------------------------------------
	template <typename Char>
	size_t PrintTypeShared(char aChar, Char* aBuffer, Char const* aBufferEnd, MSF_PrintData const& aData, MSF_StringFormatType const& aValue)
	{
		RegisteredChar const& registered = theRegisteredChars[GetCharIndex(aChar)];

		MSF_PrintData tmp = aData;
		tmp.myPrintChar = aChar;
		tmp.myValue = &aValue;
		return registered.Printer.Print(aBuffer, aBufferEnd, tmp);
	}
	size_t PrintType(char aChar, char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, MSF_StringFormatType const& aValue)
	{
		return PrintTypeShared(aChar, aBuffer, aBufferEnd, aData, aValue);
	}
	size_t PrintType(char aChar, char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aData, MSF_StringFormatType const& aValue)
	{
		return PrintTypeShared(aChar, aBuffer, aBufferEnd, aData, aValue);
	}
	size_t PrintType(char aChar, char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aData, MSF_StringFormatType const& aValue)
	{
		return PrintTypeShared(aChar, aBuffer, aBufferEnd, aData, aValue);
	}
	//-------------------------------------------------------------------------------------------------
	// Error Handling
	//-------------------------------------------------------------------------------------------------
	MSF_ErrorMode theGlobalErrorMode = MSF_ErrorMode::Silent;
	MSF_ErrorMode thread_local theLocalErrorMode = MSF_ErrorMode::UseGlobal;

	MSF_ErrorFlags theGlobalErrorFlags;
	MSF_ErrorFlags thread_local theLocalErrorFlags = MSF_ErrorFlags::UseGlobal;

	void SetGlobalErrorMode(MSF_ErrorMode aMode)
	{
		theGlobalErrorMode = aMode;
	}
	void SetLocalErrorMode(MSF_ErrorMode aMode)
	{
		MSF_ASSERT(theLocalErrorMode == MSF_ErrorMode::UseGlobal, "Local error mode already in use");
		theLocalErrorMode = aMode;
	}
	void ClearLocalErrorMode()
	{
		theLocalErrorMode = MSF_ErrorMode::UseGlobal;
	}

	MSF_ErrorMode GetErrorMode()
	{
		MSF_ErrorMode const localMode = theLocalErrorMode;
		return localMode == MSF_ErrorMode::UseGlobal ? theGlobalErrorMode : localMode;
	}

	void SetGlobalErrorFlags(MSF_ErrorFlags someFlags)
	{
		theGlobalErrorFlags = someFlags;
	}
	void SetLocalErrorFlags(MSF_ErrorFlags someFlags)
	{
		MSF_ASSERT(theLocalErrorFlags == MSF_ErrorFlags::UseGlobal, "Local error mode already in use");
		theLocalErrorFlags = someFlags;
	}
	void ClearLocalErrorFlags()
	{
		theLocalErrorFlags = MSF_ErrorFlags::UseGlobal;
	}

	MSF_ErrorFlags GetErrorFlags()
	{
		MSF_ErrorFlags const localFlags = theLocalErrorFlags;
		return (localFlags & MSF_ErrorFlags::UseGlobal) ? theGlobalErrorFlags : localFlags;
	}

	bool IsRelaxedCSharpFormat()
	{
		return (GetErrorFlags() & MSF_ErrorFlags::RelaxedCSharpFormat) != 0;
	}

	//-------------------------------------------------------------------------------------------------
	// Normal printf
	//-------------------------------------------------------------------------------------------------
	template <typename Char>
	MSF_PrintResult SetupPrintfInfo(MSF_PrintData& aPrintData, Char const*& anInput, uint32_t& anInputIndex, uint32_t anInputCount)
	{
		// gather the parts
		// [flags] [width] [.precision] type
		// using the similar standards as Microsoft's printf
		// we don't need additional size specifiers since the size
		// is known in the format type.  this also frees up
		// more room for custom types

#if MSF_ERROR_PEDANTIC
#define SET_FLAG(flag) { if (aPrintData.myFlags & flag) return MSF_PrintResult(ER_DuplicateFlag, character); aPrintData.myFlags |= flag; }
#else
#define SET_FLAG(flag) aPrintData.myFlags |= flag;
#endif

		// Read Flags
		for (Char character = *anInput; character; character = *++anInput)
		{
			// read until non-flag is found
			switch (character)
			{
			case '-':
				SET_FLAG(PRINT_LEFTALIGN);
				aPrintData.myFlags &= ~PRINT_ZERO;
				continue;
			case '+':
				SET_FLAG(PRINT_SIGN);
				continue;
			case ' ':
				SET_FLAG(PRINT_BLANK);
				continue;
			case '#':
				SET_FLAG(PRINT_PREFIX);
				continue;
			case '0':
				if (!(aPrintData.myFlags & PRINT_LEFTALIGN))
					SET_FLAG(PRINT_ZERO);
				continue;
			}
			// no more flags
			break;
		}

		// Width and precision
		uint32_t widthPrecision[2] = {};
		for (uint32_t i = 0; i < 2; ++i)
		{
			uint32_t& value = widthPrecision[i];

			Char character = *anInput;

			if (character == '*')
			{
				++anInput;

				if ((aPrintData.myValue->myType & MSF_StringFormatInt::ValidTypes) == 0)
					return MSF_PrintResult(ER_WildcardType);

				if (anInputIndex == anInputCount)
					return MSF_PrintResult(ER_IndexOutOfRange, anInputIndex);

				switch (aPrintData.myValue->myType)
				{
				case MSF_StringFormatType::Type8: value = aPrintData.myValue->myValue8; break;
				case MSF_StringFormatType::Type16: value = aPrintData.myValue->myValue16; break;
				case MSF_StringFormatType::Type32: value = aPrintData.myValue->myValue32; break;
				case MSF_StringFormatType::Type64: value = (uint32_t)aPrintData.myValue->myValue32; break; // sure hope someone isn't trying 2gb+ width or precision O.o
				}

				++aPrintData.myValue;
				++anInputIndex;
			}
			else
			{
				for (; MSF_IsDigit(character); character = *++anInput)
					value = value * 10 + (character - '0');
			}

			if (!i && *anInput == '.')
			{
				aPrintData.myFlags |= PRINT_PRECISION;
				++anInput;
			}
			else
				break;
		}

		// Read print character
		switch (Char character = *anInput++)
		{
		case 'I': // look for I[(32|64)](d|i|o|u|x|X)
			{
				int advance = 0;
				if ((anInput[0] == '3' && anInput[1] == '2') ||
					(anInput[0] == '6' && anInput[1] == '4'))
				{
					advance = 2;
				}

				Char const next = anInput[advance];
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
				Char const next = anInput[1];
				if (next == 'd' || next == 'i' || next == 'o' || next == 'u' || next == 'x' || next == 'X' || next == 'n')
				{
					// skip hh
					character = next;
					anInput += 2;
				}
			}
			else
			{
				Char const next = anInput[0];
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
				Char const next = anInput[1];
				if (next == 'd' || next == 'i' || next == 'o' || next == 'u' || next == 'x' || next == 'X' || next == 'n')
				{
					// skip ll
					character = next;
					anInput += 2;
				}
			}
			else
			{
				Char const next = anInput[0];
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
			Char const next = anInput[0];
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
			Char const next = anInput[0];
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
			Char const next = anInput[0];
			if (next == 's' || next == 'c')
			{
				// skip l
				character = next;
				anInput += 1;
			}
		}
			goto do_print;

		default:
			if (!MSF_IsAsciiAlpha(character))
			{
				return MSF_PrintResult(ER_InvalidPrintCharacter, character ? character : '?');
			}
			else
			{
				do_print:

				aPrintData.myPrintChar = (char)character;

				static_assert(sizeof(aPrintData.myWidth) == 2, "sizeof myWidth changed, update code");
				MSF_ASSERT(widthPrecision[0] < UINT16_MAX);
				aPrintData.myWidth = (uint16_t)widthPrecision[0];

				static_assert(sizeof(aPrintData.myPrecision) == 2, "sizeof myPrecision changed, update code");
				MSF_ASSERT(widthPrecision[1] < UINT16_MAX);
				aPrintData.myPrecision = (uint16_t)widthPrecision[1];

				return ValidateTypeShared<Char>(aPrintData, *aPrintData.myValue);
			}
		}
	}

	//-------------------------------------------------------------------------------------------------
	// CSharp/hybrid style
	//-------------------------------------------------------------------------------------------------
	template <typename Char>
	MSF_PrintResult SetupFormatInfo(MSF_PrintData& aPrintData, Char const*& anInput)
	{
		// gather the parts
		// https://docs.microsoft.com/en-us/dotnet/api/system.string.format?view=netframework-4.8#controlling-formatting
		// {[index][,[-]width][:formatString[precision]]} 

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
			if (!MSF_IsAsciiAlpha(*anInput))
				return MSF_PrintResult(ER_InvalidPrintCharacter, *anInput ? *anInput : '?');

			aPrintData.myPrintChar = (char)*(anInput++);

			uint32_t precision = 0;
			while (MSF_IsDigit(*anInput))
				precision = precision * 10 + *(anInput++) - '0';

			static_assert(sizeof(aPrintData.myPrecision) == 2, "sizeof myPrecision changed, update code");
			MSF_ASSERT(precision < UINT16_MAX);
			aPrintData.myPrecision = (uint16_t)precision;
		}
		else if (aPrintData.myValue)
		{
			aPrintData.myPrintChar = GetTypeDefaultPrintCharacters(*aPrintData.myValue);
			if (!aPrintData.myPrintChar)
				return MSF_PrintResult(ER_UnsupportedType, aPrintData.myValue->myType, 0);
		}

		if (*anInput != '}')
			return MSF_PrintResult(ER_ExpectedClosingBrace, 0);

		++anInput;

		return aPrintData.myValue ? ValidateTypeShared<Char>(aPrintData, *aPrintData.myValue) : MSF_PrintResult(0);
	}
}

//-------------------------------------------------------------------------------------------------
// Helper class for constructing a formatted string
//-------------------------------------------------------------------------------------------------
template <typename Char>
class MSF_StringFormatter
{
public:
	MSF_StringFormatter() : myPrintedCharacters(0) {}

	MSF_PrintResult PrepareFormatter(MSF_StringFormatTemplate<Char> const& aStringFormat)
	{
		uint32_t const inputCount = aStringFormat.NumArgs();
		if (inputCount > MSF_MAX_ARGUMENTS)
			return MSF_PrintResult(ER_TooManyInputs, inputCount, 0);

		uint32_t inputIndex = 0;
		MSF_StringFormatType const* aData = aStringFormat.GetArgs();
		
		myPrintString = aStringFormat.GetString();
		Char const* str = myPrintString;

		enum Mode { None, Auto, Specific };
		Mode printMode = None;
		Char character;

		for ((character = *str++); character; (character = *str++))
		{
			switch (character)
			{
			case '%':
			case '{':
			{
				switch (*str)
				{
				case '%':
				case '{':
					if (character != *str)
						return MSF_PrintResult(ER_InvalidPrintCharacter, *str, int(str - myPrintString));
					++str;
					break;
				default:
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

							uint32_t nextInputIndex = *(str++) - '0';
							while (MSF_IsDigit(*str))
							{
								nextInputIndex = nextInputIndex * 10 + (*(str++) - '0');

								// This is not a perf issue in the loop since 99.9% of the time it's going to be a single character
								// we detect the moment it goes out of range to support relaxed errors in the case someone wrote "{12345667}".
								if (nextInputIndex >= MSF_MAX_ARGUMENTS*2)
									break;
							}

							if (nextInputIndex >= inputCount)
							{
								// Only relax the error if it's at least 5 past the last input. This is to try to disambiguate
								// between a user error and and non-print text. i.e. {123} is clearly out of range where {7} might just be a mistaken print
								// This does mean there might be some small edge case errors like {12} with 7 real inputs will trigger an error vs a relax
								if (nextInputIndex - inputCount > 5 && MSF_CustomPrint::IsRelaxedCSharpFormat())
								{
									--myPrintedCharacters;
									break;
								}

								return MSF_PrintResult(ER_IndexOutOfRange, nextInputIndex);
							}

							inputIndex = nextInputIndex;
						}
					}

					// Check for inconsistent print modes
					if (thisMode != printMode)
					{
						if (printMode == None)
							printMode = thisMode;
						else 
							return MSF_PrintResult(ER_InconsistentPrintType, printMode, int((Char const*)printData.myStart - myPrintString));
					}

					if (printMode == Auto && inputIndex == inputCount)
					{
						// Before considering out of range make sure this is a valid print statement and not an csharp looking one
						if (character == '{' && MSF_CustomPrint::IsRelaxedCSharpFormat())
						{
							printData.myValue = nullptr;
							if (MSF_CustomPrint::SetupFormatInfo(printData, str).HasError())
							{
								--myPrintedCharacters;
								break;
							}
						}

						return MSF_PrintResult(ER_IndexOutOfRange, inputIndex);
					}

					printData.myValue = aData + inputIndex;

					// Post increment in auto mode
					if (printMode == Auto)
					{
						++inputIndex;
					}
					
					// Process any format specifiers
					MSF_PrintResult result = character == '%' ? MSF_CustomPrint::SetupPrintfInfo(printData, str, inputIndex, inputCount) : MSF_CustomPrint::SetupFormatInfo(printData, str);

					if (result.HasError())
					{
						if (character == '{' && MSF_CustomPrint::IsRelaxedCSharpFormat())
						{
							if (result.Error == ER_ExpectedWidth ||
								result.Error == ER_InvalidPrintCharacter ||
								result.Error == ER_ExpectedClosingBrace)
							{
								--myPrintedCharacters;
								break;
							}
						}

						result.Location = int(str - myPrintString);
						return result;
					}

					printData.myEnd = str;
					break;
				}
				break;
			}
			case '}':
				if (*str == '}')
					++str;
				else if (!MSF_CustomPrint::IsRelaxedCSharpFormat())
					return MSF_PrintResult(ER_UnexpectedBrace, 0, int(str - myPrintString));

				break;
			}
		}

		// we actually end up 1 past the end of the string which is actually ok count wise
		MSF_HEAVY_ASSERT(!*(str-1));

		// get the max size required
		size_t requiredLength = str - myPrintString;
		for (uint32_t i = 0; i < myPrintedCharacters; ++i)
		{
			requiredLength += myPrintData[i].myMaxLength - ((Char const*)myPrintData[i].myEnd - (Char const*)myPrintData[i].myStart);
		}

		return MSF_PrintResult(requiredLength);
	}

	size_t FormatString(Char* aBuffer, size_t aBufferLength) const
	{
		Char const* start = aBuffer;
		Char const* end = aBuffer + aBufferLength;
		Char const* read = myPrintString;
		for (uint32_t i = 0; i < myPrintedCharacters; ++i)
		{
			// copy segment between print markers
			if (myPrintData[i].myStart > read)
			{
				while (read != myPrintData[i].myStart)
				{
					// if double control characters are found, skip one
					if ((*read == '%' || *read == '{' || *read == '}') && read[0] == read[1])
					{
						++read;
					}

					MSF_HEAVY_ASSERT(aBuffer < end);
					*aBuffer++ = *read++;
				}
			}

			int index = MSF_CustomPrint::GetCharIndex(myPrintData[i].myPrintChar);
			aBuffer += MSF_CustomPrint::theRegisteredChars[index].Printer.Print(aBuffer, (Char const*)end, myPrintData[i]);
			MSF_ASSERT(aBuffer < end);

			read = (Char const*)myPrintData[i].myEnd;
		}

		while (*read)
		{
			// if double control characters are found, skip one
			if ((*read == '%' || *read == '{' || *read == '}') && read[0] == read[1])
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

	void ProcessError(MSF_PrintResult anError, Char* aBuffer, size_t aBufferLength, size_t anOffset, Char* (*aReallocFunction)(Char*, size_t, void*), void* aUserData)
	{
		MSF_ErrorMode errorMode = MSF_CustomPrint::GetErrorMode();
		char errorMessage[256];
		size_t errorLength = anError.ToString(errorMessage, aBufferLength);

		// Null terminate buffer if possible
		if (aBuffer && aBufferLength)
		{
			aBuffer[MSF_IntMin(anOffset, aBufferLength-1)] = 0;
		}

		if (errorMode == MSF_ErrorMode::Assert)
		{
#if MSF_ASSERTS_ENABLED
			if (!MSF_IsAsserting())
			{
				MSF_ASSERT(false, "Error formatting string: %s", errorMessage);
			}
#endif
		}
		else if (errorMode == MSF_ErrorMode::WriteString)
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
			MSF_UTFCopy(aBuffer + anOffset, aBufferLength - anOffset, errorMessage, errorLength);
		}
		
		// else do nothing
	}

private:

	MSF_PrintData myPrintData[MSF_MAX_ARGUMENTS];
	Char const* myPrintString;
	size_t myPrintedCharacters;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <typename Char>
intptr_t MSF_FormatStringShared(MSF_StringFormatTemplate<Char> const& aStringFormat, Char* aBuffer, size_t aBufferLength, size_t anOffset, Char* (*aReallocFunction)(Char*, size_t, void*), void* aUserData)
{
	MSF_StringFormatter<Char> formatter;
	MSF_PrintResult result = formatter.PrepareFormatter(aStringFormat);

	if (result.HasError())
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
			MSF_ASSERT(MSF_CustomPrint::GetErrorMode() == MSF_ErrorMode::Silent, "ER_AllocationFailed: Failed to allocation additional memory for {} chars ({} bytes)", aBufferLength, aBufferLength * sizeof(Char));
			return -1;
		}

		aBufferLength = result.MaxBufferLength + anOffset;
	}

	// process string
	size_t printed = formatter.FormatString(aBuffer + anOffset, aBufferLength - anOffset);
	MSF_ASSERT(printed <= (size_t)result.MaxBufferLength - 1);
	return printed;
}
//-------------------------------------------------------------------------------------------------
intptr_t MSF_FormatString(MSF_StringFormat const& aStringFormat, char* aBuffer, size_t aBufferLength, size_t anOffset, char* (*aReallocFunction)(char*, size_t, void*), void* aUserData)
{
	return MSF_FormatStringShared(aStringFormat, aBuffer, aBufferLength, anOffset, aReallocFunction, aUserData);
}
//-------------------------------------------------------------------------------------------------
intptr_t MSF_FormatString(MSF_StringFormatUTF8 const& aStringFormat, char8_t* aBuffer, size_t aBufferLength, size_t anOffset, char8_t* (*aReallocFunction)(char8_t*, size_t, void*), void* aUserData)
{
	return MSF_FormatStringShared(
		*(MSF_StringFormatTemplate<char> const*) & aStringFormat,
		(char*)aBuffer,
		aBufferLength,
		anOffset,
		(char* (*)(char*, size_t, void*))aReallocFunction,
		aUserData);
}
//-------------------------------------------------------------------------------------------------
intptr_t MSF_FormatString(MSF_StringFormatUTF16 const& aStringFormat, char16_t* aBuffer, size_t aBufferLength, size_t anOffset, char16_t* (*aReallocFunction)(char16_t*, size_t, void*), void* aUserData)
{
	return MSF_FormatStringShared(aStringFormat, aBuffer, aBufferLength, anOffset, aReallocFunction, aUserData);
}
//-------------------------------------------------------------------------------------------------
intptr_t MSF_FormatString(MSF_StringFormatUTF32 const& aStringFormat, char32_t* aBuffer, size_t aBufferLength, size_t anOffset, char32_t* (*aReallocFunction)(char32_t*, size_t, void*), void* aUserData)
{
	return MSF_FormatStringShared(aStringFormat, aBuffer, aBufferLength, anOffset, aReallocFunction, aUserData);
}
//-------------------------------------------------------------------------------------------------
intptr_t MSF_FormatString(MSF_StringFormatWChar const& aStringFormat, wchar_t* aBuffer, size_t aBufferLength, size_t anOffset, wchar_t* (*aReallocFunction)(wchar_t*, size_t, void*), void* aUserData)
{
	return MSF_FormatStringShared(
		*(MSF_StringFormatTemplate<MSF_WChar> const*)&aStringFormat,
		(MSF_WChar*)aBuffer,
		aBufferLength,
		anOffset,
		(MSF_WChar* (*)(MSF_WChar*, size_t, void*))aReallocFunction,
		aUserData);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <typename Char>
class MSF_StringFormatCopier : public MSF_StringFormatTemplate<Char>
{
public:
	MSF_StringFormatCopier(Char const* aString, uint32_t aCount)
		: MSF_StringFormatTemplate<Char>(aString, aCount)
	{}

	template <typename Alloc>
	static MSF_StringFormatTemplate<Char> const* Copy(MSF_StringFormatTemplate<Char> const& aStringFormat, Alloc anAlloc, bool anIncludeFormatString)
	{
		MSF_StringFormatType const* sourceArgs = aStringFormat.GetArgs();
		uint32_t const nargs = aStringFormat.NumArgs();

		size_t const baseLength = sizeof(MSF_StringFormatTemplate<Char>) + sizeof(MSF_StringFormatType) * nargs;

		size_t argSizes[MSF_MAX_ARGUMENTS];
		size_t argsTotalLength = 0;
		for (uint32_t arg = 0; arg < nargs; ++arg)
		{
			argSizes[arg] = MSF_CustomPrint::GetTypeCopyLength(sourceArgs[arg]);
			argSizes[arg] = (argSizes[arg] + (MSF_COPY_ALIGNMENT - 1)) & (~(MSF_COPY_ALIGNMENT - 1));
			argsTotalLength += argSizes[arg];
		}

		size_t const formatLength = (anIncludeFormatString ? MSF_Strlen(aStringFormat.GetString()) + 1 : 0) * sizeof(Char);
		size_t const totalLength = baseLength + formatLength + argsTotalLength;

		MSF_StringFormatTemplate<Char>* stringFormatCopy = (MSF_StringFormatTemplate<Char>*)anAlloc(totalLength);
		if (stringFormatCopy)
		{
			MSF_StringFormatType* destArgs = (MSF_StringFormatType*)stringFormatCopy->GetArgs();
			char* data = ((char*)stringFormatCopy) + baseLength;

			if (anIncludeFormatString)
			{
				new (stringFormatCopy) MSF_StringFormatCopier((Char const*)data, nargs);
				MSF_CopyChars(data, data + formatLength, (char const*)aStringFormat.GetString(), formatLength);
				data += formatLength;
			}
			else
				new (stringFormatCopy) MSF_StringFormatCopier(aStringFormat.GetString(), nargs);

			for (uint32_t arg = 0; arg < nargs; ++arg)
			{
				MSF_StringFormatType const& sourceArg = sourceArgs[arg];
				MSF_StringFormatType& destArg = destArgs[arg];
				size_t const argSize = argSizes[arg];

				if (argSize)
				{
					destArg.myString = data;
					MSF_CopyChars(data, data + argSizes[arg], sourceArg.myString, argSize);
					data += argSizes[arg];
				}
				else
					destArg.myValue64 = sourceArg.myValue64;

				destArg.myType = sourceArg.myType;
				destArg.myUserData = sourceArg.myUserData;
			}

			MSF_ASSERT(data == ((char const*)stringFormatCopy) + totalLength);
		}
		return stringFormatCopy;
	}
};
//-------------------------------------------------------------------------------------------------
MSF_StringFormat const* MSF_CopyStringFormat(MSF_StringFormat const& aStringFormat, void* (*anAlloc)(size_t), bool anIncludeFormatString)
{
	return MSF_StringFormatCopier<char>::Copy(aStringFormat, anAlloc, anIncludeFormatString);
}
//-------------------------------------------------------------------------------------------------
MSF_StringFormatUTF8 const* MSF_CopyStringFormat(MSF_StringFormatUTF8 const& aStringFormat, void* (*anAlloc)(size_t), bool anIncludeFormatString)
{
	return MSF_StringFormatCopier<char8_t>::Copy(aStringFormat, anAlloc, anIncludeFormatString);
}
//-------------------------------------------------------------------------------------------------
MSF_StringFormatUTF16 const* MSF_CopyStringFormat(MSF_StringFormatUTF16 const& aStringFormat, void* (*anAlloc)(size_t), bool anIncludeFormatString)
{
	return MSF_StringFormatCopier<char16_t>::Copy(aStringFormat, anAlloc, anIncludeFormatString);
}
//-------------------------------------------------------------------------------------------------
MSF_StringFormatUTF32 const* MSF_CopyStringFormat(MSF_StringFormatUTF32 const& aStringFormat, void* (*anAlloc)(size_t), bool anIncludeFormatString)
{
	return MSF_StringFormatCopier<char32_t>::Copy(aStringFormat, anAlloc, anIncludeFormatString);
}
//-------------------------------------------------------------------------------------------------
MSF_StringFormatWChar const* MSF_CopyStringFormat(MSF_StringFormatWChar const& aStringFormat, void* (*anAlloc)(size_t), bool anIncludeFormatString)
{
	return MSF_StringFormatCopier<wchar_t>::Copy(aStringFormat, anAlloc, anIncludeFormatString);
}
//-------------------------------------------------------------------------------------------------
MSF_StringFormat const* MSF_CopyStringFormat(MSF_StringFormat const& aStringFormat, void* (*anAlloc)(size_t, void*), void* aUserData, bool anIncludeFormatString)
{
	return MSF_StringFormatCopier<char>::Copy(aStringFormat, [&](size_t aSize) { return anAlloc(aSize, aUserData); }, anIncludeFormatString);
}
//-------------------------------------------------------------------------------------------------
MSF_StringFormatUTF8 const* MSF_CopyStringFormat(MSF_StringFormatUTF8 const& aStringFormat, void* (*anAlloc)(size_t, void*), void* aUserData, bool anIncludeFormatString)
{
	return MSF_StringFormatCopier<char8_t>::Copy(aStringFormat, [&](size_t aSize) { return anAlloc(aSize, aUserData); }, anIncludeFormatString);
}
//-------------------------------------------------------------------------------------------------
MSF_StringFormatUTF16 const* MSF_CopyStringFormat(MSF_StringFormatUTF16 const& aStringFormat, void* (*anAlloc)(size_t, void*), void* aUserData, bool anIncludeFormatString)
{
	return MSF_StringFormatCopier<char16_t>::Copy(aStringFormat, [&](size_t aSize) { return anAlloc(aSize, aUserData); }, anIncludeFormatString);
}
//-------------------------------------------------------------------------------------------------
MSF_StringFormatUTF32 const* MSF_CopyStringFormat(MSF_StringFormatUTF32 const& aStringFormat, void* (*anAlloc)(size_t, void*), void* aUserData, bool anIncludeFormatString)
{
	return MSF_StringFormatCopier<char32_t>::Copy(aStringFormat, [&](size_t aSize) { return anAlloc(aSize, aUserData); }, anIncludeFormatString);
}
//-------------------------------------------------------------------------------------------------
MSF_StringFormatWChar const* MSF_CopyStringFormat(MSF_StringFormatWChar const& aStringFormat, void* (*anAlloc)(size_t, void*), void* aUserData, bool anIncludeFormatString)
{
	return MSF_StringFormatCopier<wchar_t>::Copy(aStringFormat, [&](size_t aSize) { return anAlloc(aSize, aUserData); }, anIncludeFormatString);
}