#include "MSF_Utilities.h"
#include "MSF_Assert.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
size_t MSF_Strlen(char const* aString)
{
	if (!aString || !*aString)
		return 0;

	char const* end = aString;

	while (*(++end));

	return (size_t)(end - aString);
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
size_t MSF_Strlen(wchar_t const* aString)
{
    if (!aString || !*aString)
        return 0;

    wchar_t const* end = aString;

    while (*(++end));

    return (size_t)(end - aString);
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void MSF_SplatChars(char* aBuffer, char const* aBufferEnd, char aValue, size_t aCount)
{
    if (aBuffer + aCount > aBufferEnd)
    {
        MSF_ASSERT(aBuffer + aCount < aBufferEnd, "Supplied buffer is not big enough to support splatting %d chars. It only has %d chars available", aCount, aBufferEnd - aBuffer);
        return;
    }

    // Increment to an 8 byte boundary
    while ((uintptr_t)aBuffer % 8 != 0 && aCount > 0)
    {
        *aBuffer++ = aValue;
        --aCount;
    }

    // Copy in 8 byte blocks
    if (aCount > 8)
    {
        size_t const aValue16 = aValue | (aValue << 8);
        size_t const aValue32 = aValue16 | (aValue16 << 16);
        size_t const sizeValue = aValue32 | (aValue32 << 32);
        size_t constexpr sizeDiff = sizeof(size_t);

        size_t sizeCount = aCount / sizeDiff;
        size_t* sizeDst = (size_t*)aBuffer;
        while (sizeCount--)
        {
            *sizeDst++ = sizeValue;
        }


        aCount -= (sizeCount * sizeDiff);
        aBuffer += (sizeCount * sizeDiff);
    }

    // copy any remainder
    while (aCount > 0)
    {
        *aBuffer++ = aValue;
        --aCount;
    }
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void MSF_SplatChars(wchar_t* aBuffer, wchar_t const* aBufferEnd, wchar_t aValue, size_t aCount)
{
    if (aBuffer + aCount > aBufferEnd)
    {
        MSF_ASSERT(aBuffer + aCount < aBufferEnd, "Supplied buffer is not big enough to support splatting %d wide chars. It only has %d wide chars available", aCount, aBufferEnd - aBuffer);
        return;
    }

    // Increment to an 8 byte boundary
    while ((uintptr_t)aBuffer % 8 != 0 && aCount > 0)
    {
        *aBuffer++ = aValue;
        --aCount;
    }

    // Copy in 8 byte blocks
    if (aCount > 8)
    {
        size_t const aValue32 = aValue | (aValue << 16);
        size_t const sizeValue = aValue32 | (aValue32 << 32);
        size_t constexpr sizeDiff = sizeof(size_t) / sizeof(wchar_t);

        size_t sizeCount = aCount / sizeDiff;
        size_t* sizeDst = (size_t*)aBuffer;
        while (sizeCount--)
        {
            *sizeDst++ = sizeValue;
        }

        aCount -= sizeCount * sizeDiff;
        aBuffer += sizeCount * sizeDiff;
    }

    // copy any remainder
    while (aCount > 0)
    {
        *aBuffer++ = aValue;
    }
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void MSF_CopyChars(char* aBuffer, char const* aBufferEnd, void const* aSource, size_t aSourceSize)
{
    if (aBuffer + aSourceSize > aBufferEnd)
    {
        MSF_ASSERT(aBuffer + aSourceSize < aBufferEnd, "Supplied buffer is not big enough to support copying %d bytes. It only has %d bytes available", aSourceSize, aBufferEnd - aBuffer);
        return;
    }

    // Copy in local word sized blocks. The worst case for modern cpus is it will be a bit slower
    // if the buffers are unaligned

    size_t sizeCount = aSourceSize / sizeof(size_t);
    size_t aCharCount = aSourceSize - (sizeCount * sizeof(size_t));

    if (aBuffer <= aSource || (char*)aBuffer > (char const*)aSource + aSourceSize)
    {
        size_t* sizeDst = (size_t*)aBuffer;
        size_t const* sizeSrc = (size_t const*)aSource;

        while (sizeCount--)
        {
            *sizeDst++ = *sizeSrc++;
        }
        char* charDst = (char*)sizeDst;
        char const* charSrc = (char const*)sizeSrc;
        while (aCharCount--)
        {
            *charDst++ = *charSrc++;
        }
    }
    else
    {
        size_t* sizeDst = (size_t*)((char*)aBuffer + aSourceSize);
        size_t const* sizeSrc = (size_t const*)((char const*)aSource + aSourceSize);

        while (sizeCount--)
        {
            *--sizeDst = *--sizeSrc;
        }
        char* charDst = (char*)sizeDst;
        char const* charSrc = (char const*)sizeSrc;
        while (aCharCount--)
        {
            *--charDst = *--charSrc;
        }
    }
}
