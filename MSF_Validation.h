#pragma once

#include "MSF_Utilities.h"

//-------------------------------------------------------------------------------------------------
// This is required to get compilation to work. I think it's to defer the type lookup or something, idk
//-------------------------------------------------------------------------------------------------
template <typename Type>
struct MSF_TypeWrapper
{
	using Value = Type;
};

//-------------------------------------------------------------------------------------------------
// Valid types wrapper occurs in a template with a lookup wrapper to allow to late lookup which enabled
// users to add additional mappings later
//-------------------------------------------------------------------------------------------------
template <typename Lookup>
struct MSF_ValidTypes
{
	static uint32_t constexpr GetType(char c)
	{
		uint32_t constexpr theTypes[] =
		{
			Lookup:: template CharToTypes<'A'>::Type,
			Lookup:: template CharToTypes<'B'>::Type,
			Lookup:: template CharToTypes<'C'>::Type,
			Lookup:: template CharToTypes<'D'>::Type,
			Lookup:: template CharToTypes<'E'>::Type,
			Lookup:: template CharToTypes<'F'>::Type,
			Lookup:: template CharToTypes<'G'>::Type,
			Lookup:: template CharToTypes<'H'>::Type,
			Lookup:: template CharToTypes<'I'>::Type,
			Lookup:: template CharToTypes<'J'>::Type,
			Lookup:: template CharToTypes<'K'>::Type,
			Lookup:: template CharToTypes<'L'>::Type,
			Lookup:: template CharToTypes<'M'>::Type,
			Lookup:: template CharToTypes<'N'>::Type,
			Lookup:: template CharToTypes<'O'>::Type,
			Lookup:: template CharToTypes<'P'>::Type,
			Lookup:: template CharToTypes<'Q'>::Type,
			Lookup:: template CharToTypes<'R'>::Type,
			Lookup:: template CharToTypes<'S'>::Type,
			Lookup:: template CharToTypes<'T'>::Type,
			Lookup:: template CharToTypes<'U'>::Type,
			Lookup:: template CharToTypes<'V'>::Type,
			Lookup:: template CharToTypes<'W'>::Type,
			Lookup:: template CharToTypes<'X'>::Type,
			Lookup:: template CharToTypes<'Y'>::Type,
			Lookup:: template CharToTypes<'Z'>::Type,

			// characters between alpha sets, this allows for simpler lookup condition later
			0, 0, 0, 0, 0, 0,

			Lookup:: template CharToTypes<'a'>::Type,
			Lookup:: template CharToTypes<'b'>::Type,
			Lookup:: template CharToTypes<'c'>::Type,
			Lookup:: template CharToTypes<'d'>::Type,
			Lookup:: template CharToTypes<'e'>::Type,
			Lookup:: template CharToTypes<'f'>::Type,
			Lookup:: template CharToTypes<'g'>::Type,
			Lookup:: template CharToTypes<'h'>::Type,
			Lookup:: template CharToTypes<'i'>::Type,
			Lookup:: template CharToTypes<'j'>::Type,
			Lookup:: template CharToTypes<'k'>::Type,
			Lookup:: template CharToTypes<'l'>::Type,
			Lookup:: template CharToTypes<'m'>::Type,
			Lookup:: template CharToTypes<'n'>::Type,
			Lookup:: template CharToTypes<'o'>::Type,
			Lookup:: template CharToTypes<'p'>::Type,
			Lookup:: template CharToTypes<'q'>::Type,
			Lookup:: template CharToTypes<'r'>::Type,
			Lookup:: template CharToTypes<'s'>::Type,
			Lookup:: template CharToTypes<'t'>::Type,
			Lookup:: template CharToTypes<'u'>::Type,
			Lookup:: template CharToTypes<'v'>::Type,
			Lookup:: template CharToTypes<'w'>::Type,
			Lookup:: template CharToTypes<'x'>::Type,
			Lookup:: template CharToTypes<'y'>::Type,
			Lookup:: template CharToTypes<'z'>::Type,
		};

		return theTypes[c - 'A'];
	}
};

//-------------------------------------------------------------------------------------------------
// Helper to map print characters to supported types to check for type mismatches
// This is required by compile time validation, but declared outside of it for consistency and maybe some
// possibly extensions. I.e. You should really define this for your custom types
//-------------------------------------------------------------------------------------------------
template <typename C>
struct MSF_CharToTypesLookup
{
	template <char Char>
	using CharToTypes = MSF_CharToTypes<Char>;
};

//-------------------------------------------------------------------------------------------------
// Hack to be able to cause a compile error. In GCC/Clang the string literal will be printed on the 
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_ValidationError)
void MSF_ValidationError(char const*);
#endif

#if !defined(MSF_ValidationErrorExtra)
#define MSF_ValidationErrorExtra()
#endif

#include <concepts>

//-------------------------------------------------------------------------------------------------
// Validator captures the incoming string and assumes that a sized string is known at compile time and
// uses the compile time only branch to validate the format
//-------------------------------------------------------------------------------------------------
template <typename Char, typename... Args>
struct MSF_Validator
{
	template <typename T>
	requires std::is_same_v<T, Char const*>
	constexpr MSF_Validator(T aString)
		: myString(aString)
	{
	}

	template <int Size>
	constexpr MSF_Validator(Char(&aString)[Size])
		: myString(aString)
	{
	}

	template <int Size>
	consteval MSF_Validator(Char const (&aString)[Size])
		: myString(aString)
	{
		int constexpr len = sizeof...(Args);

		if constexpr (len > MSF_MAX_ARGUMENTS)
			MSF_ValidationError("Max number of arguments [" MSF_STR(MSF_MAX_ARGUMENTS) "] exceeded");

		uint64_t constexpr types[len+1] = { MSF_StringFormatTypeLookup<Args>::ID..., 0};

		int numArgs = 0;
		uint64_t usedArgs = 0;
		int printMode = 0; // 1 = auto, 2 = indexed
		for (int i = 0; aString[i];)
		{
			switch (aString[i++])
			{
			case '%':
				if (aString[i] == '%')
				{
					++i;
				}
				else
				{
#if MSF_ERROR_PEDANTIC
					bool hasFlag[256] = {};
#endif
					while (aString[i] == '-' || aString[i] == '+' || aString[i] == ' ' || aString[i] == '#' || aString[i] == '0')
					{
#if MSF_ERROR_PEDANTIC
						if (hasFlag[(int)aString[i]])
							MSF_ValidationError("Duplicate Flag Detected");

						if ((aString[i] == '0' && hasFlag[(int)'-']) ||
							(aString[i] == '-' && hasFlag[(int)'0']))
						{
							MSF_ValidationError("0 flag has no effect when left aligned");
						}
						hasFlag[(int)aString[i]] = true;
#endif
						++i;
					}

					if (aString[i] == '*')
					{
						if (numArgs >= len)
							MSF_ValidationError("Index out of range");

						if ((MSF_ValidTypes<MSF_CharToTypesLookup<Char>>::GetType('d') & types[numArgs]) == 0)
							MSF_ValidationError("Expected integer type for width wildcard");

						usedArgs |= 1LL << numArgs;
						++numArgs;
						++i;
					}
					else
					{
						while (MSF_IsDigit(aString[i]))
							++i;
					}

					if (aString[i] == '.')
					{
						++i;

						if (aString[i] == '*')
						{
							if (numArgs >= len)
								MSF_ValidationError("Index out of range");

							if ((MSF_ValidTypes<MSF_CharToTypesLookup<Char>>::GetType('d') & types[numArgs]) == 0)
								MSF_ValidationError("Expected integer type for precision wildcard");

							usedArgs |= 1LL << numArgs;
							++numArgs;
							++i;
						}
						else if (MSF_IsDigit(aString[i]))
						{
							while (MSF_IsDigit(aString[i]))
								++i;
						}
					}

					if (MSF_IsAsciiAlpha(aString[i]))
					{
						if (numArgs >= len)
							MSF_ValidationError("Index out of range");

						if (MSF_ValidTypes<MSF_CharToTypesLookup<Char>>::GetType((char)aString[i]) == 0)
							MSF_ValidationError("Unknown type");

						if ((MSF_ValidTypes<MSF_CharToTypesLookup<Char>>::GetType((char)aString[i]) & types[numArgs]) == 0)
							MSF_ValidationError("Type mismatch");

						usedArgs |= 1LL << numArgs;
						++numArgs;
					}
					else
					{
						MSF_ValidationError("Unexpected end of print specifier");
					}

					if (printMode == 0) printMode = 1;
					else if (printMode == 2)
						MSF_ValidationError("Mismatched print modes, don't mix %x,{} with {N}");
				}
				break;
			case '{':
				if (aString[i] == '{')
				{
					++i;
				}
				else
				{
					int index = numArgs;
					if (MSF_IsDigit(aString[i]))
					{
						index = 0;
						while (MSF_IsDigit(aString[i]))
							index = index * 10 + aString[i++] - '0';

						if (printMode == 0) printMode = 2;
						else if (printMode == 1)
							MSF_ValidationError("Mismatched print modes, don't mix %x,{} with {N}");
					}
					else
					{
						if (printMode == 0) printMode = 1;
						else if (printMode == 2)
							MSF_ValidationError("Mismatched print modes, don't mix %x,{} with {N}");
					}

					if (index >= len)
						MSF_ValidationError("Index out of range");

					if (aString[i] == ',')
					{
						++i;
						if (aString[i] == '-')
							++i;

						if (!MSF_IsDigit(aString[i]))
							MSF_ValidationError("Expected digit for width");

						while (MSF_IsDigit(aString[i])) ++i;
					}

					if (aString[i] == ':')
					{
						++i;

						if (!MSF_IsAsciiAlpha(aString[i]))
							MSF_ValidationError("Expected type specifier");

						if (MSF_ValidTypes<MSF_CharToTypesLookup<Char>>::GetType((char)aString[i]) == 0)
							MSF_ValidationError("Unknown type");

						if ((MSF_ValidTypes<MSF_CharToTypesLookup<Char>>::GetType((char)aString[i]) & types[index]) == 0)
							MSF_ValidationError("Type mismatch");

						++i;

						while (MSF_IsDigit(aString[i])) ++i;
					}

					if (aString[i] != '}')
						MSF_ValidationError("Expected closing brace");

					usedArgs |= 1LL << index;
					++numArgs;
					++i;
				}
				break;
			case '}':
				if (aString[i] == '}')
				{
					++i;
				}
				else
				{
					MSF_ValidationError("Mismatched closing brace");
				}
				break;
			}
		}

		if (usedArgs != (1LL << len) - 1)
			MSF_ValidationError("Not all arguments were used");
	}

	constexpr operator Char const* () const { return myString; }

	Char const* myString;
	char const* myError = nullptr; // Mainly for unit tests
};
