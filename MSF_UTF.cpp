#include "MSF_UTF.h"
#include "MSF_Assert.h"
#include "MSF_Utilities.h"

#define MSF_WCHAR_IS_16 WCHAR_MAX == UINT16_MAX

//-------------------------------------------------------------------------------------------------
// The leader bits of the the first character tells you how many bytes to use.
// 0b0xxxxxxx - ascii character, 1 byte
// 0b110xxxxx - 2 bytes, 5 bits in this segment (total 11 bits)
// 0b1110xxxx - 3 bytes, 4 bits in this segment (total 16 bits)
// 0b11110xxx - 4 bytes, 3 bits in this segment (total 21 bits)
// 
// Subsequent bytes all have the same form of 0xb10xxxxxx to have 6 bits each.
//-------------------------------------------------------------------------------------------------
MSF_CodeRead MSF_ReadCodePoint(char const* aString)
{
	// read as unsigned to make checks work
	uint32_t const lead = *(uint8_t const*)aString;

	if (lead <= 0x7f)
	{
		return { lead, 1 };
	}

	if (!aString[1])
	{
		return { 0, 1 };
	}

	if (lead < 0b11100000)
	{
		uint32_t const code =
			((lead & 0b00011111) << 6) |
			((aString[1] & 0b00111111));

		return { code, 2 };
	}

	if (!aString[2])
	{
		return { 0, 2 };
	}

	if (lead < 0b11110000)
	{
		uint32_t const code =
			((lead & 0b00001111) << 12) |
			((aString[1] & 0b00111111) << 6) |
			((aString[2] & 0b00111111));

		return { code, 3 };
	}

	if (!aString[3])
	{
		return { 0, 3 };
	}

	uint32_t const code =
		((lead & 0b00000111) << 18) |
		((aString[1] & 0b00111111) << 12) |
		((aString[2] & 0b00111111) << 6) |
		((aString[3] & 0b00111111));

	return { code, 4 };
}
//-------------------------------------------------------------------------------------------------
// Values in the range 0xD800-0xDfff are reserved and used to identify when a unicode character is split
// into two UTF16 segments.
// 
// When split, 0x10000 is removed from the total value which allows use of more space in the pair which
// combined only contains 20 bits of total information
//-------------------------------------------------------------------------------------------------
MSF_CodeRead MSF_ReadCodePoint(char16_t const* aString)
{
	uint32_t const lead = *(uint16_t const*)aString;

	// if (code >= 0xD800 && code < 0xE000) but with a single branch
	if ((lead - 0xD800) < 0xE000 - 0xD800)
	{
		// unexpected end of file, read the bytes but return null terminator
		if (!aString[1])
		{
			return { 0, 1 };
		}

		uint32_t const code =
			0x10000 |
			((lead & 0b1111111111) << 10) |
			(aString[1] & 0b1111111111);
		return { code, 2 };
	}

	return { lead, 1 };
}
//-------------------------------------------------------------------------------------------------
MSF_CodeRead MSF_ReadCodePoint(char32_t const* aString)
{
	return MSF_CodeRead{ *aString, 1 };
}
//-------------------------------------------------------------------------------------------------
MSF_CodeRead MSF_ReadCodePoint(wchar_t const* aString)
{
#if MSF_WCHAR_IS_16
	return MSF_ReadCodePoint((char16_t const*)aString);
#else
	return MSF_CodeRead{ (uint32_t)*aString, 1 };
#endif
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
uint32_t MSF_WriteCodePoint(uint32_t aCodePoint, char aStringOut[4])
{
	if (aCodePoint <= 0x7f)
	{
		aStringOut[0] = char(aCodePoint);
		return 1;
	}

	if (aCodePoint <= 0x7ff)
	{
		aStringOut[0] = 0b11000000 | char((aCodePoint >> 6));
		aStringOut[1] = 0b10000000 | char((aCodePoint >> 0) & 0b00111111);
		return 2;
	}

	if (aCodePoint <= 0xffff)
	{
		aStringOut[0] = 0b11100000 | char((aCodePoint >> 12));
		aStringOut[1] = 0b10000000 | char((aCodePoint >> 6) & 0b00111111);
		aStringOut[2] = 0b10000000 | char((aCodePoint >> 0) & 0b00111111);
		return 3;
	}

	aStringOut[0] = 0b11110000 | char((aCodePoint >> 18) & 0b00000111);
	aStringOut[1] = 0b10000000 | char((aCodePoint >> 12) & 0b00111111);
	aStringOut[2] = 0b10000000 | char((aCodePoint >> 6) & 0b00111111);
	aStringOut[3] = 0b10000000 | char((aCodePoint >> 0) & 0b00111111);
	return 4;
}
//-------------------------------------------------------------------------------------------------
uint32_t MSF_WriteCodePoint(uint32_t aCodePoint, char16_t aStringOut[2])
{
	if (aCodePoint <= 0xffff)
	{
		MSF_ASSERT(aCodePoint < 0xd800 || aCodePoint >= 0xE000);
		aStringOut[0] = (char16_t)aCodePoint;
		return 1;
	}

	aCodePoint -= 0x10000;

	aStringOut[0] = char16_t(0xD800 | (aCodePoint >> 10));
	aStringOut[1] = char16_t(0xDC00 | (aCodePoint & 0b1111111111));
	return 2;
}
//-------------------------------------------------------------------------------------------------
uint32_t MSF_WriteCodePoint(uint32_t aCodePoint, char32_t aStringOut[1])
{
	*aStringOut = aCodePoint;
	return 1;
}
//-------------------------------------------------------------------------------------------------
uint32_t MSF_WriteCodePoint(uint32_t aCodePoint, wchar_t aStringOut[2])
{
#if MSF_WCHAR_IS_16
	if (aCodePoint <= 0xffff)
	{
		MSF_ASSERT(aCodePoint < 0xd800 || aCodePoint >= 0xE000);
		aStringOut[0] = (char16_t)aCodePoint;
		return 1;
}

	aCodePoint -= 0x10000;

	aStringOut[0] = char16_t(0xD800 | (aCodePoint >> 10));
	aStringOut[1] = char16_t(0xDC00 | (aCodePoint & 0b1111111111));
	return 2;
#else
	*aStringOut = aCodePoint;
	return 1;
#endif
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <typename CharTo, typename CharFrom>
MSF_CharactersWritten MSF_UTFCopyShared(CharTo* aStringOut, size_t aBufferLength, CharFrom const* aStringIn, size_t aCharacterLimit)
{
	MSF_CharactersWritten written = { 0, 0 };

	while (*aStringIn && written.Characters < aCharacterLimit)
	{
		MSF_CodeRead const read = MSF_ReadCodePoint(aStringIn);
		aStringIn += read.CharsRead;

		union
		{
			CharTo String[4 / sizeof(CharTo)];
			uint32_t Block;
		} Optim;
		
		uint32_t const write = MSF_WriteCodePoint(read.CodePoint, Optim.String);

		if (write > aBufferLength)
			break;

		if (aStringOut)
		{
			if (aBufferLength > 4 / sizeof(CharTo))
			{
				// fast copy just copy a whole 4 byte block as long as we have space
				*(uint32_t*)aStringOut = Optim.Block;
			}
			else
			{
				for (uint32_t i = 0; i < write; ++i)
				{
					aStringOut[i] = Optim.String[i];
				}
			}
			aStringOut += write;
		}

		aBufferLength -= write;
		written.Elements += write;
		++written.Characters;
	}

	if (aStringOut && aBufferLength)
		*aStringOut = 0;

	return written;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MSF_CharactersWritten MSF_UTFCopy(char* aStringOut, size_t aBufferLength, char const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(char* aStringOut, size_t aBufferLength, char16_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(char* aStringOut, size_t aBufferLength, char32_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(char* aStringOut, size_t aBufferLength, wchar_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }

MSF_CharactersWritten MSF_UTFCopy(char16_t* aStringOut, size_t aBufferLength, char const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(char16_t* aStringOut, size_t aBufferLength, char16_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(char16_t* aStringOut, size_t aBufferLength, char32_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(char16_t* aStringOut, size_t aBufferLength, wchar_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }

MSF_CharactersWritten MSF_UTFCopy(char32_t* aStringOut, size_t aBufferLength, char const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(char32_t* aStringOut, size_t aBufferLength, char16_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(char32_t* aStringOut, size_t aBufferLength, char32_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(char32_t* aStringOut, size_t aBufferLength, wchar_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }

MSF_CharactersWritten MSF_UTFCopy(wchar_t* aStringOut, size_t aBufferLength, char const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(wchar_t* aStringOut, size_t aBufferLength, char16_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(wchar_t* aStringOut, size_t aBufferLength, char32_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
MSF_CharactersWritten MSF_UTFCopy(wchar_t* aStringOut, size_t aBufferLength, wchar_t const* aStringIn, size_t aCharacterLimit) { return MSF_UTFCopyShared(aStringOut, aBufferLength, aStringIn, aCharacterLimit); }
