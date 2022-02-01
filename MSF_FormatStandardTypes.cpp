#include "MSF_FormatStandardTypes.h"
#include "MSF_Assert.h"
#include "MSF_PlatformConfig.h"
#include "MSF_ToString.h"
#include "MSF_UTF.h"
#include "MSF_Utilities.h"

//-------------------------------------------------------------------------------------------------
// Windows printf respects the 0 flag, posix doesn't
//-------------------------------------------------------------------------------------------------
static char locStringLeadingCharacter(MSF_PrintData const& aData)
{
#if MSF_STRING_ALLOW_LEADING_ZERO
	return ((aData.myFlags & PRINT_ZERO) && aData.myWidth) ? '0' : ' ';
#else
	(void)aData;
	return ' ';
#endif
}
//------------------------------------------------------------------------------------------------- 
//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatChar
{
	struct CharData
	{
		uint32_t Length;
		union
		{
			char UTF8[4];
			char16_t UTF16[2];
			char32_t UTF32[1];
		} Data;
	};

	size_t ValidateUTF8(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);

		static_assert(sizeof(CharData) <= sizeof(aData.myUserData), "Not enough storage space for temp data");
		CharData* utf8Char = (CharData*)&aData.myUserData;

		if (aValue.myType == MSF_StringFormatType::Type8)
		{
			utf8Char->Data.UTF8[0] = aValue.myValue8;
			utf8Char->Length = 1;
		}
		else
		{
			uint32_t codePoint;
			if (aValue.myType == MSF_StringFormatType::Type16)
			{
				char16_t data[2] = { aValue.myValue16, 0 };
				codePoint = MSF_ReadCodePoint(data).CodePoint;
			}
			else
			{
				// If not a wchar then just assume the value is a unicode code point
				codePoint = aValue.myValue32;
			}

			utf8Char->Length = MSF_WriteCodePoint(codePoint, utf8Char->Data.UTF8);
		}

		return MSF_IntMax<uint32_t>(aData.myWidth, utf8Char->Length);
	}
	size_t ValidateUTF16(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);

		static_assert(sizeof(CharData) <= sizeof(aData.myUserData), "Not enough storage space for temp data");
		CharData* utf8Char = (CharData*)&aData.myUserData;

		if (aValue.myType == MSF_StringFormatType::Type8)
		{
			utf8Char->Data.UTF16[0] = aValue.myValue8;
			utf8Char->Length = 1;
		}
		else
		{
			if (aValue.myType == MSF_StringFormatType::Type16)
			{
				utf8Char->Data.UTF16[0] = aValue.myValue16;
				utf8Char->Length = 1;
			}
			else
			{
				// If not a wchar then just assume the value is a unicode code point
				utf8Char->Length = MSF_WriteCodePoint(aValue.myValue32, utf8Char->Data.UTF16);
			}
		}

		return MSF_IntMax<uint32_t>(aData.myWidth, utf8Char->Length);
	}
	size_t ValidateUTF32(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);

		static_assert(sizeof(CharData) <= sizeof(aData.myUserData), "Not enough storage space for temp data");
		CharData* utf8Char = (CharData*)&aData.myUserData;

		utf8Char->Length = 1;

		if (aValue.myType == MSF_StringFormatType::Type8)
		{
			utf8Char->Data.UTF32[0] = aValue.myValue8;
		}
		else if (aValue.myType == MSF_StringFormatType::Type16)
		{
			utf8Char->Data.UTF32[0] = aValue.myValue16;
		}
		else
		{
			utf8Char->Data.UTF32[0] = aValue.myValue16;
		}

		return MSF_IntMax<uint32_t>(aData.myWidth, utf8Char->Length);
	}

	template <typename Char>
	size_t Print(Char* aBuffer, Char const* aBufferEnd, MSF_PrintData const& aData)
	{
		Char* bufferWrite = aBuffer;
		CharData const charData = *(CharData const*)&aData.myUserData;

		if (!(aData.myFlags & PRINT_LEFTALIGN) && aData.myWidth > charData.Length)
		{
			size_t const splatChars = aData.myWidth - charData.Length;
			MSF_SplatChars(bufferWrite, aBufferEnd, locStringLeadingCharacter(aData), splatChars);
			bufferWrite += splatChars;
		}

		MSF_CopyChars(bufferWrite, aBufferEnd, (Char const*)charData.Data.UTF8, charData.Length);
		bufferWrite += charData.Length;

		if ((aData.myFlags & PRINT_LEFTALIGN) && aData.myWidth > charData.Length)
		{
			size_t const splatChars = aData.myWidth - charData.Length;
			MSF_SplatChars(bufferWrite, aBufferEnd, ' ', splatChars);
			bufferWrite += splatChars;
		}

		return bufferWrite - aBuffer;
	}
	size_t PrintUTF8(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData) { return Print(aBuffer, aBufferEnd, aData); }
	size_t PrintUTF16(char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aData) { return Print(aBuffer, aBufferEnd, aData); }
	size_t PrintUTF32(char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aData) { return Print(aBuffer, aBufferEnd, aData); }
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatString
{
	template <typename CharTo, typename CharFrom>
	struct Helper
	{
		static size_t Validate(MSF_PrintData& aData, CharFrom const* aString, size_t aLength)
		{
			MSF_CharactersWritten written;

			if (aData.myFlags & PRINT_PRECISION)
			{
#if MSF_STRING_PRECISION_IS_CHARACTERS
				if (aLength == SIZE_MAX)
					written = MSF_UTFCopyLength<CharTo>(aString, aData.myPrecision);
				else
					written = MSF_UTFCopy((CharTo*)nullptr, aLength, aString, aData.myPrecision);
#else
				written = MSF_UTFCopy((CharTo*)nullptr, MSF_IntMin<size_t>(aLength, aData.myPrecision), aString);
#endif
			}
			else
			{
				written = MSF_UTFCopyLength<CharTo>(aString);
			}

#if MSF_STRING_PRECISION_IS_CHARACTERS
			aData.myUserData = written.Characters;

			size_t elementsRequired = written.Elements;
			if (aData.myWidth > written.Characters)
			{
				elementsRequired += aData.myWidth - written.Characters;
			}
			return elementsRequired;
#else
			aData.myUserData = written.Elements;

			return MSF_IntMax<size_t>(written.Elements, aData.myWidth);
#endif
		}

		static size_t Print(CharTo* aBuffer, CharTo const* aBufferEnd, MSF_PrintData const& aData, CharFrom const* aString)
		{
			CharTo* bufferWrite = aBuffer;
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
				MSF_SplatChars(bufferWrite, aBufferEnd, ' ', splatChars);
				bufferWrite += splatChars;
			}

			return bufferWrite - aBuffer;
		}
	};

	template <typename Char>
	struct Helper<Char, Char>
	{
		static size_t Validate(MSF_PrintData& aData, Char const* aString, size_t aLength)
		{
			aData.myUserData = aLength == SIZE_MAX ? MSF_Strlen(aString) : aLength;

			if (aData.myFlags & PRINT_PRECISION)
			{
				aData.myUserData = MSF_IntMin<uint64_t>(aData.myUserData, aData.myPrecision);
			}

			return MSF_IntMax<size_t>((size_t)aData.myUserData, aData.myWidth);
		}

		static size_t Print(Char* aBuffer, Char const* aBufferEnd, MSF_PrintData const& aData, Char const* aString)
		{
			Char* bufferWrite = aBuffer;
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
	};

	template <typename Char>
	size_t ValidateShared(MSF_PrintData& aData, MSF_StringFormatType const& aValue, size_t aLength = SIZE_MAX)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);

		if (aValue.myString == nullptr)
		{
#if MSF_STRING_NULL_ALL_OR_NOTHING
			if (!(aData.myFlags & PRINT_PRECISION) || aData.myPrecision > 5)
				return Helper<Char, char>::Validate(aData, "(null)", 6);
			return Helper<Char, char>::Validate(aData, "", 0);
#else
			return Helper<Char, char>::Validate(aData, "(null)", 6);
#endif
		}

		if ((aValue.myUserData & (MSF_StringFormatType::UTF16 | MSF_StringFormatType::UTF32)) == 0)
			return Helper<Char, char>::Validate(aData, aValue.myString, aLength);

		if (aValue.myUserData & MSF_StringFormatType::UTF16)
			return Helper<Char, char16_t>::Validate(aData, aValue.myUTF16String, aLength);

		return Helper<Char, char32_t>::Validate(aData, aValue.myUTF32String, aLength);
	}

	size_t ValidateUTF8(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		return ValidateShared<char>(aData, aValue);
	}

	size_t ValidateUTF16(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		return ValidateShared<char16_t>(aData, aValue);
	}

	size_t ValidateUTF32(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		return ValidateShared<char32_t>(aData, aValue);
	}

	size_t ValidateUTF8(MSF_PrintData& aData, MSF_StringFormatType const& aValue, size_t aLength)
	{
		return ValidateShared<char>(aData, aValue, aLength);
	}

	size_t ValidateUTF16(MSF_PrintData& aData, MSF_StringFormatType const& aValue, size_t aLength)
	{
		return ValidateShared<char16_t>(aData, aValue, aLength);
	}

	size_t ValidateUTF32(MSF_PrintData& aData, MSF_StringFormatType const& aValue, size_t aLength)
	{
		return ValidateShared<char32_t>(aData, aValue, aLength);
	}

	template <typename Char>
	size_t PrintShared(Char* aBuffer, Char const* aBufferEnd, MSF_PrintData const& aData)
	{
		if (aData.myValue->myString == nullptr)
		{
#if MSF_STRING_NULL_ALL_OR_NOTHING
			if (!(aData.myFlags & PRINT_PRECISION) || aData.myPrecision > 5)
				return Helper<Char, char>::Print(aBuffer, aBufferEnd, aData, "(null)");
			return Helper<Char, char>::Print(aBuffer, aBufferEnd, aData, "");
#else
			return Helper<Char, char>::Print(aBuffer, aBufferEnd, aData, "(null)");
#endif
		}

		if ((aData.myValue->myUserData & (MSF_StringFormatType::UTF16 | MSF_StringFormatType::UTF32)) == 0)
			return Helper<Char, char>::Print(aBuffer, aBufferEnd, aData, aData.myValue->myString);

		if (aData.myValue->myUserData & MSF_StringFormatType::UTF16)
			return Helper<Char, char16_t>::Print(aBuffer, aBufferEnd, aData, aData.myValue->myUTF16String);

		return Helper<Char, char32_t>::Print(aBuffer, aBufferEnd, aData, aData.myValue->myUTF32String);
	}

	size_t PrintUTF8(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData)
	{
		return PrintShared(aBuffer, aBufferEnd, aData);
	}
	size_t PrintUTF16(char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aData)
	{
		return PrintShared(aBuffer, aBufferEnd, aData);
	}
	size_t PrintUTF32(char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aData)
	{
		return PrintShared(aBuffer, aBufferEnd, aData);
	}

	size_t CopyLength(MSF_StringFormatType const& aValue)
	{
		if (aValue.myString == nullptr)
			return 0;
		if (aValue.myUserData & MSF_StringFormatType::UTF32)
			return (MSF_Strlen(aValue.myUTF32String) + 1) * 4;
		if (aValue.myUserData & MSF_StringFormatType::UTF16)
			return (MSF_Strlen(aValue.myUTF16String) + 1) * 2;
		return MSF_Strlen(aValue.myString) + 1;
	}
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatInt
{
	size_t Validate(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);
		static_assert(MSF_StringFormatType::Type8 == 2, "Format type indexes changed, this formula no longer works");

		// type = (2/4/8/16) target = (3/5/10/20)
		// type / 2 * 3 = (3/6/12/24) - type >> 2 (0/1/2/4) = (3/5/10/20), exactly what we need, + 1 for sign, just in case its negative
		size_t maxLength = size_t(aValue.myType / 2 * 3 - (aValue.myType >> 2));
		maxLength = MSF_IntMax<size_t>(maxLength, aData.myPrecision);
		return MSF_IntMax<size_t>(maxLength, aData.myWidth) + 1;
	}

	size_t ValidateOctal(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);

		// type = (2/4/8/16) target = (3/6/11/23)
		// type / 2 * 3 = (3/6/12/24) which is close enough for me, + prefix 'o'
		size_t maxLength = size_t((aValue.myType / 2 * 3));
		maxLength = MSF_IntMax<size_t>(maxLength, aData.myPrecision) + (aData.myFlags & PRINT_PREFIX);
		return MSF_IntMax<size_t>(maxLength, aData.myWidth);
	}

	size_t ValidateHex(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);

		// make sure these calculations stay the same as those in GetTypeMaxLength
		// type = (2/4/8/16)... perfect, add prefix of '0x'.
		size_t maxLength = size_t((aValue.myType));
		maxLength = MSF_IntMax<size_t>(maxLength, aData.myPrecision) + (aData.myFlags & PRINT_PREFIX) * 2;
		return MSF_IntMax<size_t>(maxLength, aData.myWidth);
	}

	size_t ValidatePointer(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		(void)aValue;
		MSF_ASSERT(aValue.myType & ValidTypes);

		// always print pointer sized
		size_t maxLength = sizeof(void*) * 2;
		maxLength = MSF_IntMax<size_t>(maxLength, aData.myPrecision);

#if MSF_POINTER_ADD_PREFIX
		maxLength += 2;
#endif // MSF_POINTER_ADD_PREFIX

#if MSF_POINTER_ADD_SIGN_OR_BLANK
		maxLength += !!(aData.myFlags & (PRINT_SIGN | PRINT_BLANK));
#endif // MSF_POINTER_ADD_SIGN_OR_BLANK

		return MSF_IntMax<size_t>(maxLength, aData.myWidth);
	}

	template <typename Char, typename Type, typename SignedType>
	size_t Print(Char* aBuffer, Char const* aBufferEnd, MSF_PrintData const& aData, Type aValue)
	{
		bool isSigned = aData.myPrintChar == 'd' || aData.myPrintChar == 'i';
		bool addPrefix = false;
		bool printSign = false;
		Char sign = ' ';

		char start = (aData.myPrintChar == 'X' || aData.myPrintChar == 'P' || (MSF_POINTER_PRINT_CAPS && aData.myPrintChar == 'p')) ? 'A' : 'a';
		auto radix = 10;
		auto precision = aData.myPrecision;
		auto flags = aData.myFlags;

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

#if MSF_POINTER_PRINT_NIL
			if (value == 0)
			{
				Char nil[] = { '(', 'n', 'i', 'l', ')', 0 };
				aData.myUserData = 5;
				return MSF_CustomPrint::PrintType('s', aBuffer, aBufferEnd, aData, MSF_StringFormatType(nil));
			}
#endif

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

		auto utoa = MSF_UnsignedToString<Type, Char>(value, radix, start);
		Char const* string = utoa;
		auto length = utoa.Length();

		Char const prefix[2] = { '0', Char(start + ('x' - 'a')) };
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

		Char* bufferWrite = aBuffer;

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

	template <typename Char>
	size_t PrintShared(Char* aBuffer, Char const* aBufferEnd, MSF_PrintData const& aData)
	{
		switch (aData.myValue->myType)
		{
		case MSF_StringFormatType::Type64:
			return Print<Char, uint64_t, int64_t>(aBuffer, aBufferEnd, aData, aData.myValue->myValue64);
		case MSF_StringFormatType::Type32:
			return Print<Char, uint32_t, int32_t>(aBuffer, aBufferEnd, aData, aData.myValue->myValue32);
		case MSF_StringFormatType::Type16:
			return Print<Char, uint16_t, int16_t>(aBuffer, aBufferEnd, aData, aData.myValue->myValue16);
		case MSF_StringFormatType::Type8:
			return Print<Char, uint8_t, int8_t>(aBuffer, aBufferEnd, aData, aData.myValue->myValue8);
		}

		MSF_ASSERT(false);
		return 0;
	}

	size_t PrintUTF8(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData)
	{
		return PrintShared(aBuffer, aBufferEnd, aData);
	}
	size_t PrintUTF16(char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aData)
	{
		return PrintShared(aBuffer, aBufferEnd, aData);
	}
	size_t PrintUTF32(char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aData)
	{
		return PrintShared(aBuffer, aBufferEnd, aData);
	}
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
namespace MSF_StringFormatFloat
{
	size_t Validate(MSF_PrintData& aData, MSF_StringFormatType const& aValue)
	{
		MSF_ASSERT(aValue.myType & ValidTypes);
		static_assert(MSF_StringFormatType::Typefloat == 32, "Format type indexes changed, this formula no longer works");

		// make sure these calculations stay the same as those in GetTypeMaxLength
		int const extra = 7; // add sign, decimal place, and extra for exponent (e+999)
		if (!(aData.myFlags & PRINT_PRECISION)) aData.myPrecision = 6;
		size_t maxLength = size_t(aValue.myType / 2); // max length of floats is 16/32 chars for 32 bit and 64 bit respectively, + '.' '-'
		return MSF_IntMax<size_t>(aData.myWidth + aData.myPrecision, maxLength) + extra;
	}

	template <typename Char>
	size_t PrintShared(Char* aBuffer, Char const* aBufferEnd, MSF_PrintData const& aData)
	{
		double value = aData.myValue->myType == MSF_StringFormatType::Typefloat ? aData.myValue->myfloat : aData.myValue->mydouble;
		return MSF_DoubleToString(value, aBuffer, MSF_IntMin<size_t>(aData.myMaxLength, aBufferEnd - aBuffer), aData.myPrintChar, aData.myWidth, aData.myPrecision, aData.myFlags);
	}

	size_t PrintUTF8(char* aBuffer, char const* aBufferEnd, MSF_PrintData const& aData)
	{
		return PrintShared(aBuffer, aBufferEnd, aData);
	}
	size_t PrintUTF16(char16_t* aBuffer, char16_t const* aBufferEnd, MSF_PrintData const& aData)
	{
		return PrintShared(aBuffer, aBufferEnd, aData);
	}
	size_t PrintUTF32(char32_t* aBuffer, char32_t const* aBufferEnd, MSF_PrintData const& aData)
	{
		return PrintShared(aBuffer, aBufferEnd, aData);
	}
}
