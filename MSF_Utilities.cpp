#include "MSF_Utilities.h"
#include "MSF_Assert.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <typename Char>
size_t MSF_StrlenShared(Char const* aString)
{
    if (!aString || !*aString)
        return 0;

    Char const* end = aString;

    while (*(++end));

    return (size_t)(end - aString);
}
//-------------------------------------------------------------------------------------------------
size_t MSF_Strlen(char const* aString) { return MSF_StrlenShared(aString); }
size_t MSF_Strlen(char16_t const* aString) { return MSF_StrlenShared(aString); }
size_t MSF_Strlen(char32_t const* aString) { return MSF_StrlenShared(aString); }
size_t MSF_Strlen(wchar_t const* aString) { return MSF_StrlenShared(aString); }
//-------------------------------------------------------------------------------------------------
// Helper to upgrade char/wchar into size_t for optimal copying
//-------------------------------------------------------------------------------------------------
inline size_t MSF_GrowValue(char16_t aValue)
{
    size_t const aValue32 = aValue | (aValue << 16);

#if INTPTR_MAX == INT32_MAX
    return aValue32;
#elif INTPTR_MAX == INT64_MAX
    return aValue32 | (aValue32 << 32);
#else
#error "Unsupported register size for optimal copying"
#endif
}
inline size_t MSF_GrowValue(char aValue)
{
    return MSF_GrowValue(char16_t(aValue | (aValue << 8)));
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template<typename Char>
void MSF_StrlenShared(Char* aBuffer, Char const* aBufferEnd, Char aValue, size_t aCount)
{
    if (aBuffer + aCount > aBufferEnd)
    {
        MSF_ASSERT(aBuffer + aCount < aBufferEnd, "Supplied buffer is not big enough to support splatting %d chars. It only has %d chars available", aCount, aBufferEnd - aBuffer);
        return;
    }

    size_t constexpr alignment = sizeof(size_t);
    size_t constexpr sizeDiff = alignment / sizeof(char);

    // Increment to an alignemnt boundary, if not aligned to sizeof(Char) then this will end up doing all the copying very slowly
    while ((uintptr_t)aBuffer % alignment != 0 && aCount > 0)
    {
        *aBuffer++ = aValue;
        --aCount;
    }

    // Copy in optimal blocks
    if (aCount > alignment)
    {
        size_t const sizeValue = MSF_GrowValue(aValue);
        size_t sizeCount = aCount / sizeDiff;
        size_t* sizeDst = (size_t*)aBuffer;

        aCount -= (sizeCount * sizeDiff);
        aBuffer += (sizeCount * sizeDiff);

        while (sizeCount--)
        {
            *sizeDst++ = sizeValue;
        }
    }

    // copy any remainder
    while (aCount > 0)
    {
        *aBuffer++ = aValue;
        --aCount;
    }
}
//-------------------------------------------------------------------------------------------------
void MSF_SplatChars(char* aBuffer, char const* aBufferEnd, char aValue, size_t aCount)
{
    MSF_StrlenShared(aBuffer, aBufferEnd, aValue, aCount);
}
//-------------------------------------------------------------------------------------------------
void MSF_SplatChars(char16_t* aBuffer, char16_t const* aBufferEnd, char16_t aValue, size_t aCount)
{
    MSF_StrlenShared(aBuffer, aBufferEnd, aValue, aCount);
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
