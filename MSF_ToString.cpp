#include "MSF_ToString.h"
#include "MSF_Assert.h"
#include "MSF_FormatPrint.h"
#include "MSF_PlatformConfig.h"
#include "MSF_Utilities.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <float.h>

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template<typename Type, typename Char>
MSF_UnsignedToString<Type, Char>::MSF_UnsignedToString(Type aValue, uint32_t aRadix, char aHexStart)
{
    Char* end = myBuffer + MaxLength;
    Char* string = end;
    *string = '\0';

    // no matter the radix, if the value is 0 return "0".
    if (!aValue)
    {
        myLength = 1;
        *(--string) = '0';
        return;
    }

    aHexStart -= 10;

    unsigned shift = 1;
    switch (aRadix)
    {
    case 32: ++shift;
    case 16: ++shift;
    case 8: ++shift;
    case 4: ++shift;
    case 2:
        for (int i, mask = aRadix - 1; aValue; aValue >>= shift)
        {
            i = aValue & mask;
            *(--string) = (i < 10 ? '0' : aHexStart) + char(i);
        }
        break;
        // specializing for 10 improves performance due to better compiler optimization
    case 10:
        while (aValue)
        {
            *(--string) = char('0' + aValue % 10);
            aValue /= 10;
        }
        break;
    case 0:
    case 1:
        // unsupported, return an empty string
        break;
    default:
        for (int i; aValue; aValue /= (Type)aRadix)
        {
            i = aValue % aRadix;
            *(--string) = (i < 10 ? '0' : aHexStart) + char(i);
        }
        break;
    }

    MSF_ASSERT(string >= myBuffer);
    myLength = uint32_t(end - string);
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template<typename SignedType, typename UnsignedType, typename Char>
MSF_SignedToString<SignedType, UnsignedType, Char>::MSF_SignedToString(SignedType aValue, uint32_t aRadix, char aHexStart)
    : MSF_UnsignedToString<UnsignedType, Char>(aRadix == 10 ? MSF_IntAbs(aValue) : aValue, aRadix, aHexStart)
{
    if (aRadix == 10 && aValue < 0)
    {
        Char* string = this->myBuffer + MSF_UnsignedToString<UnsignedType>::MaxLength - this->myLength;
        MSF_ASSERT(string > this->myBuffer);
        *(--string) = '-'; // we know it's coming from buffer so we're good to write
        ++this->myLength;
    }
}

//-------------------------------------------------------------------------------------------------
// Explicit template instantiation of supported types
//-------------------------------------------------------------------------------------------------
template struct MSF_UnsignedToString<uint8_t, char>;
template struct MSF_UnsignedToString<uint16_t, char>;
template struct MSF_UnsignedToString<uint32_t, char>;
template struct MSF_UnsignedToString<uint64_t, char>;
template struct MSF_SignedToString<int8_t, uint8_t, char>;
template struct MSF_SignedToString<int16_t, uint16_t, char>;
template struct MSF_SignedToString<int32_t, uint32_t, char>;
template struct MSF_SignedToString<int64_t, uint64_t, char>;

template struct MSF_UnsignedToString<uint8_t, char16_t>;
template struct MSF_UnsignedToString<uint16_t, char16_t>;
template struct MSF_UnsignedToString<uint32_t, char16_t>;
template struct MSF_UnsignedToString<uint64_t, char16_t>;
template struct MSF_SignedToString<int8_t, uint8_t, char16_t>;
template struct MSF_SignedToString<int16_t, uint16_t, char16_t>;
template struct MSF_SignedToString<int32_t, uint32_t, char16_t>;
template struct MSF_SignedToString<int64_t, uint64_t, char16_t>;

template struct MSF_UnsignedToString<uint8_t, char32_t>;
template struct MSF_UnsignedToString<uint16_t, char32_t>;
template struct MSF_UnsignedToString<uint32_t, char32_t>;
template struct MSF_UnsignedToString<uint64_t, char32_t>;
template struct MSF_SignedToString<int8_t, uint8_t, char32_t>;
template struct MSF_SignedToString<int16_t, uint16_t, char32_t>;
template struct MSF_SignedToString<int32_t, uint32_t, char32_t>;
template struct MSF_SignedToString<int64_t, uint64_t, char32_t>;

//-------------------------------------------------------------------------------------------------
// Implementation of double to string.  Needs to be cleaned up a bit.
// Currently very fast but it's not 100% conformant and doesn't support 'a'/'A'
//-------------------------------------------------------------------------------------------------
template <typename Char>
int MSF_DoubleToStringShared(double aValue, Char* aBuffer, size_t aBufferLength, char aFormat, uint32_t aWidth, uint32_t aPrecision, uint32_t someFlags)
{
    Char const* error = nullptr;

    if (std::isinf(aValue))
    {
        static Char const errNegInf[] = { '-', 'I', 'n', 'f', 0 };
        if (aValue < 0)
            error = errNegInf;
        else
            error = &errNegInf[1];
    }
    else if (std::isnan(aValue))
    {
        static Char const errNaN[] = { 'N', 'a', 'N', 0 };
        error = errNaN;
    }

    if (error != nullptr)
    {
        size_t const errorLength = MSF_Strlen(error);

        if (aBuffer != nullptr)
        {
            if (aBufferLength < errorLength)
            {
                // null terminate to be safe
                if (aBufferLength > 0)
                    *aBuffer = '\0';

                return -1;
            }

            MSF_CopyChars(aBuffer, aBuffer + aBufferLength, error, errorLength);
        }
        return (uint32_t)errorLength;
    }

    // set up workspace

    // max possible digits <= those need to show all of prec + exp
    // <= ceil(log10(HUGE)) plus space for null, etc.

    uint32_t const worksiz = int((M_LN2 / M_LN10) * DBL_MAX_EXP) + 8;

    // for fractional part
    Char  fwork[worksiz];
    Char* fw = fwork;

    // for integer part
    Char  iwork[worksiz];
    Char* iworkBufferLength = &iwork[sizeof(iwork) / sizeof(Char) - 1];
    Char* iw = iworkBufferLength;
    *iw = 0;

    // for exponent part

    uint32_t const eworksiz = int(M_LN2 * DBL_DIG) + 8;
    Char  ework[eworksiz];
    Char* eworkBufferLength = &ework[sizeof(ework) / sizeof(Char) - 1];
    Char* ew = eworkBufferLength;
    *ew = 0;

    // grab sign & make non-negative
    bool const is_neg = aValue < 0;
    if (is_neg) aValue = -aValue;

    bool caps = false;
    if (aFormat >= 'A' && aFormat <= 'Z')
    {
        caps = true;
        aFormat -= 'A' - 'a';
    }

    // precision matters
    if (aPrecision > worksiz - 2) // can't have more precision than supported
    {
        aPrecision = worksiz - 2;
    }

    if (aPrecision == 0 && aFormat == 'g')
    {
        aPrecision = 1;
    }

    double powprec;
    double const powprecreal = pow(10.0, (long)aPrecision);
    if (aPrecision <= 6)
        powprec = 1.0e6;
    else
        powprec = pow(10.0, (long)aPrecision);

    double rounder = 0.5 / powprec;

    int const f_fmt = aFormat == 'f' ||
        ((aFormat == 'g') && (aValue == 0.0 || (aValue >= 1e-4 - (1e-4 * (0.5 / powprecreal)) && (aValue+0.5f) < powprecreal)));

    uint32_t iwidth = 0;
    uint32_t fwidth = 0;
    uint32_t ewidth = 0;

    if (f_fmt)  // fixed format
    {
        double ipart;
        double const fpart = modf(aValue, &ipart);

        // convert fractional part
        double ffpart = fpart;
        bool firstzero = true;
        uint32_t numfirstzeroes = 0;

        if (fpart >= rounder || aFormat != 'g')
        {
            double ifpart;
            for (uint32_t i = 0; i < aPrecision; ++i)
            {
                ffpart = modf(ffpart * 10.0, &ifpart);
                *fw++ = '0' + char(ifpart);
                if (aFormat == 'g' && firstzero && ifpart == 0.0 && ipart == 0.0)
                {
                    ++numfirstzeroes;
                    ++aPrecision;
                }
                else
                {
                    firstzero = false;
                }
                ++fwidth;
            }
        }

        // convert integer part
        if (ipart == 0.0)
        {
            //if (cvt != 'g' || fwidth < aPrecision || fwidth < aWidth)
            {
                *--iw = '0'; ++iwidth;
            }
        }
        else if (ipart <= double(INT32_MAX)) // a useful speedup
        {
            long li = long(ipart);
            while (li != 0)
            {
                *--iw = '0' + (li % 10);
                li = li / 10;
                ++iwidth;
            }
        }
        else // the slow way
        {
            while (ipart > 0.5)
            {
                double ff = modf(ipart / 10.0, &ipart);
                ff = (ff + 0.05) * 10.0;
                *--iw = '0' + char(ff);
                ++iwidth;
            }
        }

        bool doRoundUp = true;

        // g-fmt: kill part of frac if precision exceeded
        if (aFormat == 'g')
        {
            uint32_t test = fwidth;
            test += iwidth > 1 ? iwidth : (iwidth == 1 && *iw != '0');
            if (test > aPrecision)
            {
                doRoundUp = false;

                uint32_t adj = test - aPrecision;
                for (Char* f = &fwork[fwidth - 1]; f >= fwork && adj > 0; --adj, --f)
                {
                    --fw;
                    --fwidth;
                    Char ch = *f;
                    *f = 0;
                    if (adj == 1 && ch >= '5') // properly round: unavoidable propagation
                    {
                        int carry = 1;
                        Char* p;
                        for (p = f - 1; p >= fwork && carry; --p)
                        {
                            ++* p;
                            if (*p > '9')
                                *p = '0';
                            else
                                carry = 0;
                        }
                        if (carry)
                        {
                            for (p = iworkBufferLength - 1; p >= iw && carry; --p)
                            {
                                ++* p;
                                if (*p > '9')
                                    *p = '0';
                                else
                                    carry = 0;
                            }
                            if (carry)
                            {
                                *--iw = '1';
                                ++iwidth;
                                --adj;
                            }
                        }
                    }
                }
            }
        }

        // If we didn't remove numbers from precision, check for rounding the last value
        if (doRoundUp)
        {
            // round up?
            if (ffpart >= 0.5)
            {
                bool add = false;
                int remainingPrecision = aPrecision;
                if (remainingPrecision)
                {
                    Char* ffix = fw - 1;
                    while (*ffix == '9' && ffix >= fwork)
                    {
                        *ffix-- = '0';
                    }

                    if (ffix >= fwork)
                    {
                        if (*ffix == '0' && !firstzero && uint32_t(ffix - fwork) < numfirstzeroes)
                        {
                            --remainingPrecision;
                            if (someFlags & PRINT_PREFIX) --fwidth;
                        }
                        ++*ffix;
                    }
                    else
                    {
                        add = true;
                    }
                }
                if (!remainingPrecision || add)
                {
                    int carry = 1;
                    Char* p;
                    for (p = iworkBufferLength - 1; p >= iw && carry; --p)
                    {
                        ++* p;
                        if (*p > '9')
                            *p = '0';
                        else
                            carry = 0;
                    }
                    if (carry)
                    {
                        *--iw = '1';
                        ++iwidth;
                    }
                }
            }
        }
    }
    else  // e-fmt
    {
        // normalize
        int exp = 0;
        while (aValue >= 10.0)
        {
            aValue *= 0.1;
            ++exp;
        }
        double const almost_one = 1.0 - rounder;
        while (aValue > 0.0 && aValue < almost_one)
        {
            aValue *= 10.0;
            --exp;
        }

        double ipart;
        double const fpart = modf(aValue, &ipart);

        if (aFormat == 'g')     // used up one digit for int part...
        {
            --aPrecision;
            powprec /= 10.0;
            rounder = 0.5 / powprec;
        }

        // convert fractional part -- almost same as above
        if (fpart >= rounder || aFormat != 'g')
        {
            double ffpart = fpart;
            double ifpart;
            for (uint32_t i = 0; i < aPrecision; ++i)
            {
                ffpart = modf(ffpart * 10.0, &ifpart);
                *fw++ = '0' + Char(ifpart);
                ++fwidth;
            }
            // round up?
            if (ffpart >= 0.5)
            {
                bool add = false;
                if (aPrecision)
                {
                    Char* ffix = fw - 1;
                    while (*ffix == '9' && ffix >= fwork)
                    {
                        *ffix-- = '0';
                    }

                    if (ffix >= fwork)
                    {
                        ++* ffix;
                    }
                    else
                    {
                        add = true;
                    }
                }
                if (!aPrecision || add)
                {
                    ipart += 1.0;
                    if (ipart >= 10.0)
                    {
                        ++exp;
                        ipart /= 10.0;
                    }
                }
            }
        }

        // convert exponent
        bool const eneg = exp < 0;
        if (eneg) exp = -exp;

        while (exp > 0)
        {
            *--ew = '0' + (exp % 10);
            exp /= 10;
            ++ewidth;
        }

        while (ewidth < 2)  // ensure at least 2 zeroes
        {
            *--ew = '0';
            ++ewidth;
        }

        *--ew = eneg ? '-' : '+';
        *--ew = (caps) ? 'E' : 'e';

        ewidth += 2;

        // convert the one-digit integer part
        *--iw = '0' + Char(ipart);
        ++iwidth;
    }

    if (aFormat == 'g')
    {
        // remove trailing zeroes when not in prefix mode
        if (!(someFlags & PRINT_PREFIX))
        {
            for (Char* p = fw - 1; p >= fwork && *p == '0'; --p)
            {
                *p = 0;
                --fw;
                --fwidth;
            }
        }
		else if (someFlags & PRINT_PRECISION)
		{
            while (iwidth + fwidth < aPrecision)
            {
                *(fw++) = '0';
                ++fwidth;
            }
        }

        if (f_fmt)
        {
            // Adjust to precision, integer doesn't count when zero
            if (iwidth == 1 && *iw == '0')
            {
                while (fwidth > aPrecision && *(fw - 1) == '0')
                {
                    *(--fw) = '\0';
                    --fwidth;
                }

				// introduce trailing zeroes up to precision when in prefix mode
				if (aValue == 0 && (someFlags & PRINT_PREFIX))
                {
#if MSF_FLOAT_PREFIX_INCLUDE_WHOLE_NUMBER
                    while (iwidth + fwidth < aPrecision)
#else
                    while (fwidth < aPrecision)
#endif
                    {
                        *(fw++) = '0';
                        ++fwidth;
                    }
                }
            }
            else
            {
                while (iwidth + fwidth > aPrecision && fwidth > 0 && *(fw - 1) == '0')
                {
                    *(--fw) = '\0';
                    --fwidth;
                }
            }
        }
    }

    // arrange everything in returned string
    uint32_t const showdot = fwidth > 0 || someFlags & PRINT_PREFIX;
    uint32_t const showneg = is_neg || (someFlags & (PRINT_SIGN | PRINT_BLANK));
    uint32_t const fmtwidth = iwidth + showdot + fwidth + ewidth + showneg;
    uint32_t const requiredLength = MSF_IntMax(fmtwidth, aWidth);

    if (aBuffer == nullptr)
        return requiredLength;

    if (aBufferLength < requiredLength + 1) // ensure space for null terminator
        return -1;

    int pad = aWidth - fmtwidth;
    if (pad < 0) pad = 0;

    Char* fmt = aBuffer;
    Char const* fmtEnd = aBuffer + aBufferLength;
    if (pad && !(someFlags & (PRINT_LEFTALIGN | PRINT_ZERO)))
    {
        MSF_SplatChars(fmt, fmtEnd, ' ', pad);
        fmt += pad;
    }

    if (is_neg) *fmt++ = '-';
    else if (someFlags & PRINT_SIGN) *fmt++ = '+';
    else if (someFlags & PRINT_BLANK) *fmt++ = ' ';

    if (pad && !(someFlags & PRINT_LEFTALIGN) && (someFlags & PRINT_ZERO))
    {
        MSF_SplatChars(fmt, fmtEnd, '0', pad);
        fmt += pad;
    }

    for (uint32_t i = 0; i < iwidth; ++i) *fmt++ = *iw++;

    if (showdot)
    {
        *fmt++ = '.';
        fw = fwork;
        for (uint32_t i = 0; i < fwidth; ++i) *fmt++ = *fw++;
    }

    for (uint32_t i = 0; i < ewidth; ++i) *fmt++ = *ew++;

    if (someFlags & PRINT_LEFTALIGN)
    {
        MSF_SplatChars(fmt, fmtEnd, (someFlags & PRINT_ZERO) ? '0' : ' ', pad);
        fmt += pad;
    }

    *fmt = 0;

    return uint32_t(fmt - aBuffer);
}

int MSF_DoubleToString(double aValue, char* anOutput, size_t aLength, char aFormat, uint32_t aWidth, uint32_t aPrecision, uint32_t someFlags)
{
    return MSF_DoubleToStringShared(aValue, anOutput, aLength, aFormat, aWidth, aPrecision, someFlags);
}
int MSF_DoubleToString(double aValue, char16_t* anOutput, size_t aLength, char aFormat, uint32_t aWidth, uint32_t aPrecision, uint32_t someFlags)
{
    return MSF_DoubleToStringShared(aValue, anOutput, aLength, aFormat, aWidth, aPrecision, someFlags);
}
int MSF_DoubleToString(double aValue, char32_t* anOutput, size_t aLength, char aFormat, uint32_t aWidth, uint32_t aPrecision, uint32_t someFlags)
{
    return MSF_DoubleToStringShared(aValue, anOutput, aLength, aFormat, aWidth, aPrecision, someFlags);
}
