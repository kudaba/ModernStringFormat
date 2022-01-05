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
inline size_t MSF_GrowValue(char32_t aValue)
{
#if INTPTR_MAX == INT32_MAX
    return aValue;
#elif INTPTR_MAX == INT64_MAX
    return aValue | (size_t(aValue) << 32);
#else
#error "Unsupported register size for optimal copying"
#endif
}
inline size_t MSF_GrowValue(char16_t aValue)
{
    return MSF_GrowValue(char32_t(aValue | (aValue << 16)));
}
inline size_t MSF_GrowValue(char aValue)
{
    return MSF_GrowValue(char16_t(aValue | (aValue << 8)));
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template<typename Char>
void MSF_SplatCharsShared(Char* aBuffer, Char const* aBufferEnd, Char aValue, size_t aCount)
{
    if (aBuffer + aCount > aBufferEnd)
    {
        MSF_ASSERT(aBuffer + aCount < aBufferEnd, "Supplied buffer is not big enough to support splatting %d chars. It only has %d chars available", aCount, aBufferEnd - aBuffer);
        return;
    }

    size_t constexpr alignment = sizeof(size_t);
    size_t constexpr sizeDiff = alignment / sizeof(Char);

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
    MSF_SplatCharsShared(aBuffer, aBufferEnd, aValue, aCount);
}
//-------------------------------------------------------------------------------------------------
void MSF_SplatChars(char16_t* aBuffer, char16_t const* aBufferEnd, char16_t aValue, size_t aCount)
{
    MSF_SplatCharsShared(aBuffer, aBufferEnd, aValue, aCount);
}
//-------------------------------------------------------------------------------------------------
void MSF_SplatChars(char32_t* aBuffer, char32_t const* aBufferEnd, char32_t aValue, size_t aCount)
{
    MSF_SplatCharsShared(aBuffer, aBufferEnd, aValue, aCount);
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <typename Char>
void MSF_CopyCharsShared(Char* aBuffer, Char const* aBufferEnd, Char const* aSource, size_t aSourceLength)
{
    if (aBuffer + aSourceLength > aBufferEnd)
    {
        MSF_ASSERT(aBuffer + aSourceLength < aBufferEnd, "Supplied buffer is not big enough to support copying %d bytes. It only has %d bytes available", aSourceLength, aBufferEnd - aBuffer);
        return;
    }

    // Copy in local register sized blocks. The worst case for modern cpus is it will be a bit slower
    // if the buffers are unaligned

    size_t sizeCount = (aSourceLength * sizeof(Char)) / sizeof(size_t);
    size_t aCharCount = aSourceLength - (sizeCount * sizeof(size_t) / sizeof(Char));

    if (aBuffer <= aSource || aBuffer > aSource + aSourceLength)
    {
        size_t* sizeDst = (size_t*)aBuffer;
        size_t const* sizeSrc = (size_t const*)aSource;

        while (sizeCount--)
        {
            *sizeDst++ = *sizeSrc++;
        }
        Char* charDst = (Char*)sizeDst;
        Char const* charSrc = (Char const*)sizeSrc;
        while (aCharCount--)
        {
            *charDst++ = *charSrc++;
        }
    }
    else
    {
        size_t* sizeDst = (size_t*)(aBuffer + aSourceLength);
        size_t const* sizeSrc = (size_t const*)(aSource + aSourceLength);

        while (sizeCount--)
        {
            *--sizeDst = *--sizeSrc;
        }
        Char* charDst = (Char*)sizeDst;
        Char const* charSrc = (Char const*)sizeSrc;
        while (aCharCount--)
        {
            *--charDst = *--charSrc;
        }
    }
}
//-------------------------------------------------------------------------------------------------
void MSF_CopyChars(char* aBuffer, char const* aBufferEnd, char const* aSource)
{
    MSF_CopyCharsShared(aBuffer, aBufferEnd, aSource, MSF_Strlen(aSource) + 1);
}
//-------------------------------------------------------------------------------------------------
void MSF_CopyChars(char16_t* aBuffer, char16_t const* aBufferEnd, char16_t const* aSource)
{
    MSF_CopyCharsShared(aBuffer, aBufferEnd, aSource, MSF_Strlen(aSource) + 1);
}
//-------------------------------------------------------------------------------------------------
void MSF_CopyChars(char32_t* aBuffer, char32_t const* aBufferEnd, char32_t const* aSource)
{
    MSF_CopyCharsShared(aBuffer, aBufferEnd, aSource, MSF_Strlen(aSource) + 1);
}
//-------------------------------------------------------------------------------------------------
void MSF_CopyChars(char* aBuffer, char const* aBufferEnd, char const* aSource, size_t aSourceLength)
{
    MSF_CopyCharsShared(aBuffer, aBufferEnd, aSource, aSourceLength);
}
//-------------------------------------------------------------------------------------------------
void MSF_CopyChars(char16_t* aBuffer, char16_t const* aBufferEnd, char16_t const* aSource, size_t aSourceLength)
{
    MSF_CopyCharsShared(aBuffer, aBufferEnd, aSource, aSourceLength);
}
//-------------------------------------------------------------------------------------------------
void MSF_CopyChars(char32_t* aBuffer, char32_t const* aBufferEnd, char32_t const* aSource, size_t aSourceLength)
{
    MSF_CopyCharsShared(aBuffer, aBufferEnd, aSource, aSourceLength);
}
