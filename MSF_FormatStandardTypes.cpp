#include "MSF_FormatStandardTypes.h"
#include "MSF_Assert.h"
#include "MSF_Utilities.h"
#include "MSF_ToString.h"
#include "MSF_UTF.h"

#if MSF_FORMAT_LOCAL_PLATFORM

#undef MSF_STRING_ALLOW_LEADING_ZERO
#undef MSF_STRING_PRECISION_IS_CHARACTERS
#undef MSF_POINTER_FORCE_PRECISION
#undef MSF_POINTER_ADD_PREFIX
#undef MSF_POINTER_ADD_SIGN_OR_BLANK

#if defined(__ANDROID__)
#define MSF_STRING_ALLOW_LEADING_ZERO 1
#define MSF_STRING_PRECISION_IS_CHARACTERS 0
#define MSF_POINTER_FORCE_PRECISION 0
#define MSF_POINTER_ADD_PREFIX 1
#define MSF_POINTER_ADD_SIGN_OR_BLANK 0
#define MSF_POINTER_PRINT_CAPS 0
#elif defined(__APPLE__)
#define MSF_STRING_ALLOW_LEADING_ZERO 1
#define MSF_STRING_PRECISION_IS_CHARACTERS 0
#define MSF_POINTER_FORCE_PRECISION 0
#define MSF_POINTER_ADD_PREFIX 1
#define MSF_POINTER_ADD_SIGN_OR_BLANK 0
#define MSF_POINTER_PRINT_CAPS 0
#elif defined(__linux__)
#define MSF_STRING_ALLOW_LEADING_ZERO 0
#define MSF_STRING_PRECISION_IS_CHARACTERS 0
#define MSF_POINTER_FORCE_PRECISION 0
#define MSF_POINTER_ADD_PREFIX 1
#define MSF_POINTER_ADD_SIGN_OR_BLANK 1
#define MSF_POINTER_PRINT_CAPS 0
#elif defined(_MSC_VER)
#define MSF_STRING_ALLOW_LEADING_ZERO 1
#define MSF_STRING_PRECISION_IS_CHARACTERS 1
#define MSF_POINTER_FORCE_PRECISION 1
#define MSF_POINTER_ADD_PREFIX 0
#define MSF_POINTER_ADD_SIGN_OR_BLANK 0
#define MSF_POINTER_PRINT_CAPS 1
#else
#error "Unsupported platform"
#endif

#endif // MSF_FORMAT_LOCAL_PLATFORM


//-------------------------------------------------------------------------------------------------
// Windows printf respects the 0 flag, posix doesn't
//-------------------------------------------------------------------------------------------------
static char locStringLeadingCharacter(MSF_PrintData const& aData)
{
#if MSF_STRING_ALLOW_LEADING_ZERO
	return (aData.myFlags & PRINT_ZERO) ? '0' : ' ';
#else
	(void)aData;
	return ' ';
#endif
}
//------------------------------------------------------------------------------------------------- 
//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatChar
{
	struct UTF8Char
	{
		uint32_t Length;
		char Data[4];
	};

	size_t Validate(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);

		static_assert(sizeof(UTF8Char) <= sizeof(aPrintData.myUserData), "Not enough storage space for temp data");
		UTF8Char* utf8Char = (UTF8Char*)&aPrintData.myUserData;

		if (aValue.myType == MSF_StringFormatType::Type8)
		{
			utf8Char->Data[0] = aValue.myValue8;
			utf8Char->Length = 1;
		}
		else
		{
			uint32_t codePoint;
			if (aValue.myType == MSF_StringFormatType::Type16 && (aValue.myUserData & MSF_StringFormatType::Char))
			{
				char16_t data[2] = { aValue.myValue16, 0 };
				codePoint = MSF_ReadCodePoint(data).CodePoint;
			}
			else
			{
				// If not a wchar then just assume the value is a unicode code point
				codePoint = aValue.myValue32;
			}

			utf8Char->Length = MSF_WriteCodePoint(codePoint, utf8Char->Data);
		}

		return MSF_IntMax<uint32_t>(aPrintData.myWidth, utf8Char->Length);
	}
	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData)
	{
		char* bufferWrite = aBuffer;
		UTF8Char const utf8Char = *(UTF8Char const*)&aData.myUserData;

		if (aData.myFlags & PRINT_LEFTALIGN)
		{
			MSF_CopyChars(bufferWrite, aBufferEnd, utf8Char.Data, utf8Char.Length);
			bufferWrite += utf8Char.Length;
		}

		if (aData.myWidth > utf8Char.Length)
		{
			size_t const splatChars = aData.myWidth - utf8Char.Length;
			MSF_SplatChars(bufferWrite, aBufferEnd, locStringLeadingCharacter(aData), splatChars);
			bufferWrite += splatChars;
		}

		if (!(aData.myFlags & PRINT_LEFTALIGN))
		{
			MSF_CopyChars(bufferWrite, aBufferEnd, utf8Char.Data, utf8Char.Length);
			bufferWrite += utf8Char.Length;
		}

		return bufferWrite - aBuffer;
	}
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatString
{
	size_t Validate(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);

		if ((aValue.myUserData & (MSF_StringFormatType::UTF16 | MSF_StringFormatType::UTF32)) == 0)
			return ValidateStringLength(aPrintData, aValue.myString);

		if (aValue.myUserData & MSF_StringFormatType::UTF16)
			return ValidateStringLength(aPrintData, aValue.myUTF16String);

		return ValidateStringLength(aPrintData, aValue.myUTF32String);
	}

	size_t ValidateStringLength(MSF_PrintData& aPrintData, char const* aString)
	{
		aPrintData.myUserData = MSF_Strlen(aString);

		if (aPrintData.myFlags & PRINT_PRECISION)
		{
			aPrintData.myUserData = MSF_IntMin<uint64_t>(aPrintData.myUserData, aPrintData.myPrecision);
		}

		return MSF_IntMax<size_t>((size_t)aPrintData.myUserData, aPrintData.myWidth);
	}

	template <typename Char>
	size_t ValidateStringLengthShared(MSF_PrintData& aPrintData, Char const* aString)
	{
		MSF_CharactersWritten written;

		if (aPrintData.myFlags & PRINT_PRECISION)
		{
#if MSF_STRING_PRECISION_IS_CHARACTERS
			written = MSF_UTFCopyLength<char>(aString, aPrintData.myPrecision);
#else
			written = MSF_UTFCopy((char*)nullptr, aPrintData.myPrecision, aString);
#endif
		}
		else
		{
			written = MSF_UTFCopyLength<char>(aString);
		}

#if MSF_STRING_PRECISION_IS_CHARACTERS
		aPrintData.myUserData = written.Characters;

		size_t elementsRequired = written.Elements;
		if (written.Characters < aPrintData.myWidth)
		{
			elementsRequired += aPrintData.myWidth - written.Characters;
		}
		return elementsRequired;
#else
		aPrintData.myUserData = written.Elements;

		return MSF_IntMax<size_t>(written.Elements, aPrintData.myWidth);
#endif
	}

	size_t ValidateStringLength(MSF_PrintData& aPrintData, char16_t const* aString) { return ValidateStringLengthShared(aPrintData, aString); }
	size_t ValidateStringLength(MSF_PrintData& aPrintData, char32_t const* aString) { return ValidateStringLengthShared(aPrintData, aString); }
	size_t ValidateStringLength(MSF_PrintData& aPrintData, wchar_t const* aString) { return ValidateStringLengthShared(aPrintData, aString); }

	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData)
	{
		if ((aData.myValue->myUserData & (MSF_StringFormatType::UTF16 | MSF_StringFormatType::UTF32)) == 0)
			return PrintString(aBuffer, aBufferEnd, aData, aData.myValue->myString);

		if (aData.myValue->myUserData & MSF_StringFormatType::UTF16)
			return PrintString(aBuffer, aBufferEnd, aData, aData.myValue->myUTF16String);

		return PrintString(aBuffer, aBufferEnd, aData, aData.myValue->myUTF32String);
	}

	size_t PrintString(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, char const* aString)
	{
		char* bufferWrite = aBuffer;
		if (!(aData.myFlags & PRINT_LEFTALIGN) && aData.myWidth > aData.myUserData)
		{
			size_t const splatChars = size_t(aData.myWidth - aData.myUserData);
			MSF_SplatChars(bufferWrite, aBufferEnd, locStringLeadingCharacter(aData), splatChars);
			bufferWrite += splatChars;
		}

		if (aData.myUserData > 0)
		{
			size_t const copyChars = (size_t)aData.myUserData;
			MSF_CopyChars(bufferWrite, aBufferEnd, aString, copyChars);
			bufferWrite += copyChars;
		}

		if (aData.myFlags & PRINT_LEFTALIGN && aData.myWidth > aData.myUserData)
		{
			size_t const splatChars = size_t(aData.myWidth - aData.myUserData);
			MSF_SplatChars(bufferWrite, aBufferEnd, locStringLeadingCharacter(aData), splatChars);
			bufferWrite += splatChars;
		}

		return bufferWrite - aBuffer;
	}

	template <typename Char>
	size_t PrintStringShared(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, Char const* aString)
	{
		char* bufferWrite = aBuffer;
		if (!(aData.myFlags & PRINT_LEFTALIGN) && aData.myWidth > aData.myUserData)
		{
			size_t const splatChars = size_t(aData.myWidth - aData.myUserData);
			MSF_SplatChars(bufferWrite, aBufferEnd, locStringLeadingCharacter(aData), splatChars);
			bufferWrite += splatChars;
		}

		if (aData.myUserData > 0)
		{
#if MSF_STRING_PRECISION_IS_CHARACTERS
			bufferWrite += MSF_UTFCopy(bufferWrite, aBufferEnd - bufferWrite, aString, (size_t)aData.myUserData).Elements;
#else
			bufferWrite += MSF_UTFCopy(bufferWrite, (size_t)aData.myUserData, aString).Elements;
#endif
		}

		if (aData.myFlags & PRINT_LEFTALIGN && aData.myWidth > aData.myUserData)
		{
			size_t const splatChars = size_t(aData.myWidth - aData.myUserData);
			MSF_SplatChars(bufferWrite, aBufferEnd, locStringLeadingCharacter(aData), splatChars);
			bufferWrite += splatChars;
		}

		return bufferWrite - aBuffer;
	}

	size_t PrintString(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, char16_t const* aString) { return PrintStringShared(aBuffer, aBufferEnd, aData, aString); }
	size_t PrintString(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, char32_t const* aString) { return PrintStringShared(aBuffer, aBufferEnd, aData, aString); }
	size_t PrintString(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, wchar_t const* aString) { return PrintStringShared(aBuffer, aBufferEnd, aData, aString); }
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatInt
{
	size_t Validate(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);
		static_assert(MSF_StringFormatType::Type8 == 2, "Format type indexes changed, this formula no longer works");

		// type = (2/4/8/16) target = (3/5/10/20)
		// type / 2 * 3 = (3/6/12/24) - type >> 2 (0/1/2/4) = (3/5/10/20), exactly what we need, + 1 for sign, just in case its negative
		size_t maxLength = size_t(aValue.myType / 2 * 3 - (aValue.myType >> 2));
		maxLength = MSF_IntMax<size_t>(maxLength, aPrintData.myPrecision);
		return MSF_IntMax<size_t>(maxLength, aPrintData.myWidth) + 1;
	}

	size_t ValidateOctal(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);

		// type = (2/4/8/16) target = (3/6/11/23)
		// type / 2 * 3 = (3/6/12/24) which is close enough for me, + prefix 'o'
		size_t maxLength = size_t((aValue.myType / 2 * 3));
		maxLength = MSF_IntMax<size_t>(maxLength, aPrintData.myPrecision) + (aPrintData.myFlags & PRINT_PREFIX);
		return MSF_IntMax<size_t>(maxLength, aPrintData.myWidth);
	}

	size_t ValidateHex(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);

		// make sure these calculations stay the same as those in GetTypeMaxLength
		// type = (2/4/8/16)... perfect, add prefix of '0x'.
		size_t maxLength = size_t((aValue.myType));
		maxLength = MSF_IntMax<size_t>(maxLength, aPrintData.myPrecision) + (aPrintData.myFlags & PRINT_PREFIX) * 2;
		return MSF_IntMax<size_t>(maxLength, aPrintData.myWidth);
	}

	size_t ValidatePointer(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		(void)aValue;
		MSF_ASSERT(aValue.myType & ValidTypes);

		// always print pointer sized
		size_t maxLength = sizeof(void*) * 2;
		maxLength = MSF_IntMax<size_t>(maxLength, aPrintData.myPrecision);

#if MSF_POINTER_ADD_PREFIX
		maxLength += 2;
#endif // MSF_POINTER_ADD_PREFIX

#if MSF_POINTER_ADD_SIGN_OR_BLANK
		maxLength += !!(aPrintData.myFlags & (PRINT_SIGN | PRINT_BLANK));
#endif // MSF_POINTER_ADD_SIGN_OR_BLANK

		return MSF_IntMax<size_t>(maxLength, aPrintData.myWidth);
	}

	template <typename Type, typename SignedType>
	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, Type aValue)
	{
		bool isSigned = aData.myPrintChar == 'd' || aData.myPrintChar == 'i';
		bool addPrefix = false;
		bool printSign = false;
		char sign = ' ';

		char start = (aData.myPrintChar == 'X' || aData.myPrintChar == 'P' || (MSF_POINTER_PRINT_CAPS && aData.myPrintChar == 'p')) ? 'A' : 'a';
		auto radix = 10;
		auto precision = aData.myPrecision;
		auto flags = aData.myFlags;

		switch (aData.myPrintChar)
		{
		case 'o':
			radix = 8;
			addPrefix = (flags & PRINT_PREFIX) != 0;
			break;
		case 'X':
		case 'x':
			radix = 16;
			addPrefix = (flags & PRINT_PREFIX) != 0;
			break;
		case 'P':
		case 'p':
			radix = 16;

#if MSF_POINTER_FORCE_PRECISION
			precision = sizeof(void*) * 2;
			flags |= PRINT_PRECISION;
#endif // MSF_POINTER_FORCE_PRECISION

#if MSF_POINTER_ADD_PREFIX
			addPrefix = true;
#endif // MSF_POINTER_ADD_PREFIX

#if MSF_POINTER_ADD_SIGN_OR_BLANK
			if (flags & PRINT_SIGN)
			{
				printSign = true;
				sign = '+';
			}
			else if (flags & PRINT_BLANK)
			{
				printSign = true;
				sign = ' ';
			}
#endif // MSF_POINTER_ADD_SIGN_OR_BLANK
			break;
		}

		if (flags & PRINT_PRECISION)
		{
			flags &= ~PRINT_ZERO;
		}

		Type value;
		if (isSigned)
		{
			SignedType signedValue = aValue;
			if (signedValue < 0)
			{
				value = -signedValue;

				printSign = true;
				sign = '-';
			}
			else
			{
				value = signedValue;
				if (flags & PRINT_SIGN)
				{
					printSign = true;
					sign = '+';
				}
				else if (flags & PRINT_BLANK)
				{
					printSign = true;
					sign = ' ';
				}
			}
		}
		else
			value = aValue;

		auto utoa = MSF_IntegerToString(value, radix, start);
		char const* string = utoa;
		auto length = utoa.Length();

		const char prefix[2] = { '0', char(start + ('x' - 'a')) };
		uint32_t prefixLen = 0;
		if (string[0] != '0' && addPrefix)
		{
			if (radix == 8)
			{
				prefixLen = 1;
				if (precision)
				{
					--precision;
				}
			}
			else if (radix == 16)
			{
				prefixLen = 2;
			}
		}
		else if (string[0] == '0' && (flags & PRINT_PRECISION) && precision == 0)
		{
			// Odd hack where we're actually printing the octal prefix 0 not the value 0
			if (aData.myPrintChar != 'o' || !addPrefix)
				--length;
		}

		uint32_t requiredLength = MSF_IntMax<uint32_t>(length, precision) + prefixLen + printSign;

		char* bufferWrite = aBuffer;

		// pad with space if we're right aligned, have space left and are using blanks
		if (requiredLength < aData.myWidth && !(flags & (PRINT_LEFTALIGN | PRINT_ZERO)))
		{
			size_t const splatChars = aData.myWidth - requiredLength;
			MSF_SplatChars(bufferWrite, aBufferEnd, ' ', splatChars);
			bufferWrite += splatChars;
		}
		// if there's a +/- or space before the number write it now (before padding with zeros)
		if (printSign)
		{
			*bufferWrite++ = sign;
		}
		// write the prefix before padding with zeros
		if (prefixLen)
		{
			MSF_CopyChars(bufferWrite, aBufferEnd, prefix, prefixLen);
			bufferWrite += prefixLen;
		}
		// if we're right aligned and have space and are using zeros write them now
		if (requiredLength < aData.myWidth && !(flags & PRINT_LEFTALIGN) && flags & PRINT_ZERO)
		{
			size_t const splatChars = aData.myWidth - requiredLength;
			MSF_SplatChars(bufferWrite, aBufferEnd, '0', splatChars);
			bufferWrite += splatChars;
		}
		// if we're less than the wanted precision pad with some zeros
		if (length < precision)
		{
			size_t const splatChars = precision - length;
			MSF_SplatChars(bufferWrite, aBufferEnd, '0', splatChars);
			bufferWrite += splatChars;
		}
		// write the value
		MSF_CopyChars(bufferWrite, aBufferEnd, string, length);
		bufferWrite += length;

		// if we're left aligned, have space and are using spaces then write them now
		if (requiredLength < aData.myWidth && flags & PRINT_LEFTALIGN)
		{
			size_t const splatChars = aData.myWidth - requiredLength;
			MSF_SplatChars(bufferWrite, aBufferEnd, ' ', splatChars);
			bufferWrite += splatChars;
		}

		return bufferWrite - aBuffer;
	}

	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData)
	{
		switch (aData.myValue->myType)
		{
		case MSF_StringFormatType::Type64:
			return Print<uint64_t, int64_t>(aBuffer, aBufferEnd, aData, aData.myValue->myValue64);
		case MSF_StringFormatType::Type32:
			return Print<uint32_t, int32_t>(aBuffer, aBufferEnd, aData, aData.myValue->myValue32);
		case MSF_StringFormatType::Type16:
			return Print<uint16_t, int16_t>(aBuffer, aBufferEnd, aData, aData.myValue->myValue16);
		case MSF_StringFormatType::Type8:
			return Print<uint8_t, int8_t>(aBuffer, aBufferEnd, aData, aData.myValue->myValue8);
		}

		MSF_ASSERT(false);
		return 0;
	}
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatFloat
{
	size_t Validate(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);
		static_assert(MSF_StringFormatType::Typefloat == 32, "Format type indexes changed, this formula no longer works");

		// make sure these calculations stay the same as those in GetTypeMaxLength
		int const extra = 7; // add sign, decimal place, and extra for exponent (e+999)
		if (!(aPrintData.myFlags & PRINT_PRECISION)) aPrintData.myPrecision = 6;
		size_t maxLength = size_t(aValue.myType / 2); // max length of floats is 16/32 chars for 32 bit and 64 bit respectively, + '.' '-'
		return MSF_IntMax<size_t>(aPrintData.myWidth + aPrintData.myPrecision, maxLength) + extra;
	}
	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData)
	{
		MSF_ASSERT(aData.myMaxLength < size_t(aBufferEnd - aBuffer));

		double value = aData.myValue->myType == MSF_StringFormatType::Typefloat ? aData.myValue->myfloat : aData.myValue->mydouble;
		return MSF_DoubleToString(value, aBuffer, MSF_IntMin<size_t>(aData.myMaxLength, aBufferEnd - aBuffer), aData.myPrintChar, aData.myWidth, aData.myPrecision, aData.myFlags);
	}
}
