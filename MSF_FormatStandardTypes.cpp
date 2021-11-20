#include "MSF_FormatStandardTypes.h"
#include "MSF_Assert.h"
#include "MSF_Utilities.h"
#include "MSF_ToString.h"

#if MSF_FORMAT_LOCAL_PLATFORM

#undef MSF_ALLOW_LEADING_ZERO
#undef MSF_POINTER_FORCE_PRECISION
#undef MSF_POINTER_ADD_PREFIX
#undef MSF_POINTER_ADD_SIGN_OR_BLANK

#if defined(__ANDROID__)
#define MSF_ALLOW_LEADING_ZERO 1
#define MSF_POINTER_FORCE_PRECISION 0
#define MSF_POINTER_ADD_PREFIX 1
#define MSF_POINTER_ADD_SIGN_OR_BLANK 0
#define MSF_POINTER_PRINT_CAPS 0
#elif defined(__APPLE__)
#define MSF_ALLOW_LEADING_ZERO 1
#define MSF_POINTER_FORCE_PRECISION 0
#define MSF_POINTER_ADD_PREFIX 1
#define MSF_POINTER_ADD_SIGN_OR_BLANK 0
#define MSF_POINTER_PRINT_CAPS 0
#elif defined(__linux__)
#define MSF_ALLOW_LEADING_ZERO 0
#define MSF_POINTER_FORCE_PRECISION 0
#define MSF_POINTER_ADD_PREFIX 1
#define MSF_POINTER_ADD_SIGN_OR_BLANK 1
#define MSF_POINTER_PRINT_CAPS 0
#elif defined(WIN64)
#define MSF_ALLOW_LEADING_ZERO 1
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
#if MSF_ALLOW_LEADING_ZERO
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
	size_t Validate(MSF_PrintData& aPrintData, MSF_StringFormatType const& aValue)
	{
		(void)aValue;
		MSF_ASSERT(aValue.myType & ValidTypes);
		return MSF_IntMax<uint32_t>(aPrintData.myWidth, 1);
	}
	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData)
	{
		char newChar = aData.myValue->myType == MSF_StringFormatType::Type64 ? (char)aData.myValue->myValue64 : (char)aData.myValue->myValue32;
		char* bufferWrite = aBuffer;

		if (aData.myFlags & PRINT_LEFTALIGN)
		{
			*bufferWrite++ = newChar;
		}

		if (aData.myWidth > 1)
		{
			size_t const splatChars = aData.myWidth - 1;
			MSF_SplatChars(bufferWrite, aBufferEnd, locStringLeadingCharacter(aData), splatChars);
			bufferWrite += splatChars;
		}

		if (!(aData.myFlags & PRINT_LEFTALIGN))
		{
			*bufferWrite++ = newChar;
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
		return ValidateStringLength(aPrintData, MSF_Strlen(aValue.myString));
	}
	size_t Print(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData)
	{
		return PrintString(aBuffer, aBufferEnd, aData, aData.myValue->myString);
	}

	size_t ValidateStringLength(MSF_PrintData& aPrintData, size_t aLength)
	{
		aPrintData.myUserData = aLength;

		if (aPrintData.myFlags & PRINT_PRECISION)
		{
			aPrintData.myUserData = MSF_IntMin<size_t>(aPrintData.myUserData, aPrintData.myPrecision);
		}

		return MSF_IntMax<size_t>(aPrintData.myUserData, aPrintData.myWidth);
	}

	size_t PrintString(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData, char const* aString)
	{
		char* bufferWrite = aBuffer;
		if (!(aData.myFlags & PRINT_LEFTALIGN) && aData.myWidth > aData.myUserData)
		{
			size_t const splatChars = aData.myWidth - aData.myUserData;
			MSF_SplatChars(bufferWrite, aBufferEnd, locStringLeadingCharacter(aData), splatChars);
			bufferWrite += splatChars;
		}

		if (aData.myUserData > 0)
		{
			size_t const copyChars = aData.myUserData;
			MSF_CopyChars(bufferWrite, aBufferEnd, aString, copyChars);
			bufferWrite += copyChars;
		}

		if (aData.myFlags & PRINT_LEFTALIGN && aData.myWidth > aData.myUserData)
		{
			size_t const splatChars = aData.myWidth - aData.myUserData;
			MSF_SplatChars(bufferWrite, aBufferEnd, locStringLeadingCharacter(aData), splatChars);
			bufferWrite += splatChars;
		}

		return bufferWrite - aBuffer;
	}
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
		size_t maxLength = size_t(aValue.myType / 2 * 3 - (aValue.myType >> 2) + 1);
		maxLength = MSF_IntMax<size_t>(maxLength, aPrintData.myPrecision);
		return MSF_IntMax<size_t>(maxLength, aPrintData.myWidth);
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
