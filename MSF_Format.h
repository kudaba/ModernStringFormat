#pragma once

#include <stdint.h>
#include <stddef.h>

//-------------------------------------------------------------------------------------------------
// Format type is used to identify incoming data and hold a pointer or reference
// to it.
// TODO: I wonder if we can use typetrait types? improvement for another day
//-------------------------------------------------------------------------------------------------
class MSF_StringFormatType
{
public:
    enum ValidTypes
    {
		TypeString  = (1 << 0),

		// BEGIN DO NOT MOVE: Otherwise changed need to be made in printer
        Type8       = (1 << 1),
        Type16      = (1 << 2),
        Type32      = (1 << 3),
        Type64      = (1 << 4),
        Typefloat	= (1 << 5),
        Typedouble	= (1 << 6),
		// END DO NOT MOVE

		TypeUserIndex = 8, // first user type, can go up to number of bits in uint32_t
		TypeUser	= (1 << TypeUserIndex), 
    };

	enum Flags
	{
		Signed		= (1 << 0),
		Char		= (1 << 1),
		Pointer		= (1 << 2),
		UTF16		= (1 << 3),
		UTF32		= (1 << 4),
	};

    explicit MSF_StringFormatType(int8_t const& aData) : myValue32(aData), myType(Type8), myUserData(Signed) {}
    explicit MSF_StringFormatType(uint8_t const& aData) : myValue32(aData), myType(Type8), myUserData(0) {}
    explicit MSF_StringFormatType(int16_t const& aData) : myValue32(aData), myType(Type16), myUserData(Signed) {}
    explicit MSF_StringFormatType(uint16_t const& aData) : myValue32(aData), myType(Type16), myUserData(0) {}
    explicit MSF_StringFormatType(int32_t const& aData) : myValue32(aData), myType(Type32), myUserData(Signed) {}
    explicit MSF_StringFormatType(uint32_t const& aData) : myValue32(aData), myType(Type32), myUserData(0) {}
    explicit MSF_StringFormatType(int64_t const& aData) : myValue64(aData), myType(Type64), myUserData(Signed) {}
    explicit MSF_StringFormatType(uint64_t const& aData) : myValue64(aData), myType(Type64), myUserData(0) {}
    explicit MSF_StringFormatType(float const& aData) : myfloat(aData), myType(Typefloat), myUserData(0) {}
    explicit MSF_StringFormatType(double const& aData) : mydouble(aData), myType(Typedouble), myUserData(0) {}
	explicit MSF_StringFormatType(long double const& aData) : mydouble((double)aData), myType(Typedouble), myUserData(0) {}

	explicit MSF_StringFormatType(char const& aData) : myValue32(aData), myType(Type8), myUserData(Signed | Char) {}
	explicit MSF_StringFormatType(char const* aData) : myString(aData), myType(TypeString), myUserData(0) {}

	explicit MSF_StringFormatType(char16_t const& aData) : myValue32(aData), myType(Type16), myUserData(Signed | Char) {}
	explicit MSF_StringFormatType(char16_t const* aData) : myUTF16String(aData), myType(TypeString), myUserData(UTF16) {}

	explicit MSF_StringFormatType(char32_t const& aData) : myValue32(aData), myType(Type32), myUserData(Signed | Char) {}
	explicit MSF_StringFormatType(char32_t const* aData) : myUTF32String(aData), myType(TypeString), myUserData(UTF32) {}

	// wchar_t might be utf16 or utf32 depending on compiler flags
	explicit MSF_StringFormatType(wchar_t const& aData) : myValue32(aData), myType(sizeof(wchar_t) == 2 ? Type16 : Type32), myUserData(Signed | Char) {}
	explicit MSF_StringFormatType(wchar_t const* aData) : myUserType((char16_t const* )aData), myType(TypeString), myUserData(sizeof(wchar_t) == 2 ? UTF16 : UTF32) {}

	explicit MSF_StringFormatType(void const* aData) : myUserType(aData), myType(sizeof(void*) * 2), myUserData(Pointer) {}

    // user types
protected:
    MSF_StringFormatType(void const* aData, uint64_t aTypeID, uint64_t someFlags = 0) : myUserType(aData), myType(aTypeID), myUserData(someFlags) {}
public:

    union
    {
		char const* myString;
		char16_t const* myUTF16String;
		char32_t const* myUTF32String;
		uint8_t myValue8;
		uint16_t myValue16;
		uint32_t myValue32;
        uint64_t myValue64;
        float myfloat;
        double mydouble;
		void const* myUserType;
    };
	uint64_t myType;
	uint64_t myUserData;
};

//-------------------------------------------------------------------------------------------------
// A workaround to make all types use format type (which will fail for unsupported types)
//-------------------------------------------------------------------------------------------------
template <typename Type>
struct MSF_StringFormatTypeLookup
{
	using Format = MSF_StringFormatType;
};

//-------------------------------------------------------------------------------------------------
// For types that don't auto convert to a known type you can use this macro to define a conversion
// function to use at call sites.
// 
// Example: MSF_DEFINE_TYPE_CONVERSION(MSF_DEFINE_TYPE_CONVERSION(std::string, aString.c_str()));
//-------------------------------------------------------------------------------------------------
#define MSF_DEFINE_TYPE_CONVERSION(type, ...) \
template <> struct MSF_StringFormatTypeLookup<type> { \
	struct Format : MSF_StringFormatType { \
	Format(type const& aString) : MSF_StringFormatType(__VA_ARGS__) {} \
	}; \
};

//-------------------------------------------------------------------------------------------------
// For everything else, users must define the supported types using the macro to keep
// compile time errors for unsupported types.
// 
// Example: MSF_DEFINE_USER_PRINTF_TYPE(std::chrono::seconds, 0);
// 
// Note: By default custom items will be stored by pointer. If you want to optimize this then you can either use a conversion or
// define your custom type manually.
// Note: Users must manage the TypeIds carefully. There are only 64-MSF_StringFormatType::TypeUserIndex available.
// Note: When using the macro you can use a 0 based index, but if using the MSF_StringFormatTypeCustom directly you
// need to adjust by MSF_StringFormatType::TypeUserIndex.
//-------------------------------------------------------------------------------------------------
template<typename UserType, uint64_t TypeID, uint64_t UserData = 0>
class MSF_StringFormatTypeCustom : public MSF_StringFormatType
{
public:
    MSF_StringFormatTypeCustom(UserType const& aData) : MSF_StringFormatType(&aData, TypeID, UserData)
    {
        static_assert(TypeID >= TypeUser, "Invalid MSF_TypeID, use a higher number");
    }
};

//-------------------------------------------------------------------------------------------------
#define MSF_DEFINE_USER_PRINTF_TYPE(type, index) MSF_DEFINE_USER_PRINTF_TYPE_FLAG(type, index, 0)

//-------------------------------------------------------------------------------------------------
// Use this if you want to include any additional or custom information along with your type to allow sharing
// print functions without consuming additional indexes
//-------------------------------------------------------------------------------------------------
#define MSF_DEFINE_USER_PRINTF_TYPE_FLAG(type, index, flag) template <>\
struct MSF_StringFormatTypeLookup<type>\
{\
	static constexpr uint64_t UserIndex = index; \
	static constexpr uint64_t ID = MSF_StringFormatType::TypeUser << index; \
	static constexpr uint64_t Flag = flag; \
    using Format = MSF_StringFormatTypeCustom<type, ID, Flag>;\
};

//-------------------------------------------------------------------------------------------------
// For int types that don't match our internal types you can extend using this.
// It will convert to the nearest size/signed type (i.e. DWORD on windows)
// 
// Example: MSF_DEFINE_USER_INT_TYPE(DWORD);
//-------------------------------------------------------------------------------------------------
template <int Size, bool Signed>
struct MSF_StringFormatExtraIntTypeInfo {};

template <typename Type>
struct MSF_StringFormatIsSigned { static constexpr bool Value = Type(-1) < Type(0); };

template <> struct MSF_StringFormatExtraIntTypeInfo<1, false> { typedef uint8_t Type; };
template <> struct MSF_StringFormatExtraIntTypeInfo<2, false> { typedef uint16_t Type; };
template <> struct MSF_StringFormatExtraIntTypeInfo<4, false> { typedef uint32_t Type; };
template <> struct MSF_StringFormatExtraIntTypeInfo<8, false> { typedef uint64_t Type; };
template <> struct MSF_StringFormatExtraIntTypeInfo<1, true> { typedef int8_t Type; };
template <> struct MSF_StringFormatExtraIntTypeInfo<2, true> { typedef int16_t Type; };
template <> struct MSF_StringFormatExtraIntTypeInfo<4, true> { typedef int32_t Type; };
template <> struct MSF_StringFormatExtraIntTypeInfo<8, true> { typedef int64_t Type; };

template <typename IntType>
class MSF_StringFormatExtraIntType : public MSF_StringFormatType
{
public:
	using ConvertType = typename MSF_StringFormatExtraIntTypeInfo<sizeof(IntType), MSF_StringFormatIsSigned<IntType>::Value>::Type;

	MSF_StringFormatExtraIntType(IntType anInt) : MSF_StringFormatType((ConvertType)anInt)
	{
	}
};

#define MSF_DEFINE_USER_INT_TYPE(type) template <>\
struct MSF_StringFormatTypeLookup<type>\
{\
    using Format = MSF_StringFormatExtraIntType<type>;\
};

// Add conversions for long/ulong since some platforms defined intptr_t/size_t as that
MSF_DEFINE_USER_INT_TYPE(long);
MSF_DEFINE_USER_INT_TYPE(unsigned long);

MSF_DEFINE_USER_INT_TYPE(long long);
MSF_DEFINE_USER_INT_TYPE(unsigned long long);

//-------------------------------------------------------------------------------------------------
// Type safe version of va_args that build using the MSF_MakeStringFormat below.
// This should never be stored directly, but it safe to pass as a reference
//-------------------------------------------------------------------------------------------------
template <typename Char>
class MSF_StringFormatTemplate
{
public:
	Char const* GetString() const { return myString; }
	MSF_StringFormatType const* GetArgs() const { return (MSF_StringFormatType const* )(this + 1); }
	uint32_t NumArgs() const { return myNumArgs; }

protected:
	MSF_StringFormatTemplate(Char const* aString, uint32_t aCount)
		: myString(aString)
		, myNumArgs(aCount)
	{}

	Char const* myString;
	uint32_t myNumArgs;
};

extern template class MSF_StringFormatTemplate<char>;
extern template class MSF_StringFormatTemplate<char16_t>;
extern template class MSF_StringFormatTemplate<char32_t>;
extern template class MSF_StringFormatTemplate<wchar_t>;

typedef MSF_StringFormatTemplate<char> MSF_StringFormat;
typedef MSF_StringFormatTemplate<char16_t> MSF_StringFormatUTF16;
typedef MSF_StringFormatTemplate<char32_t> MSF_StringFormatUTF32;
typedef MSF_StringFormatTemplate<wchar_t> MSF_StringFormatWChar;

//-------------------------------------------------------------------------------------------------
// Holds all the arguments for a printable string
//-------------------------------------------------------------------------------------------------
template<typename Char, typename ...Args>
class MSF_StringFormatContainer : public MSF_StringFormatTemplate<Char>
{
public:
	MSF_StringFormatContainer(Char const* aString, Args const&... args)
		: MSF_StringFormatTemplate<Char>(aString, sizeof...(Args))
		, myArgs { typename MSF_StringFormatTypeLookup<Args>::Format(args)... }
	{
	}

private:
	MSF_StringFormatType myArgs[sizeof...(Args)];
};

template<typename Char>
class MSF_StringFormatContainer<Char> : public MSF_StringFormatTemplate<Char>
{
public:
	MSF_StringFormatContainer(Char const* aString)
		: MSF_StringFormatTemplate<Char>(aString, 0)
	{
	}
};
//-------------------------------------------------------------------------------------------------
// Start of typesafe printf, this part generates the data that holds references to the inputs
// These do not use templates for the char type since we only support char and char16_t
//-------------------------------------------------------------------------------------------------
extern intptr_t MSF_FormatString(MSF_StringFormat const& aStringFormat, char* aBuffer, size_t aBufferLength, size_t anOffset = 0, char* (*aReallocFunction)(char*, size_t, void*) = nullptr, void* aUserData = nullptr);
extern intptr_t MSF_FormatString(MSF_StringFormatUTF16 const& aStringFormat, char16_t* aBuffer, size_t aBufferLength, size_t anOffset = 0, char16_t* (*aReallocFunction)(char16_t*, size_t, void*) = nullptr, void* aUserData = nullptr);
extern intptr_t MSF_FormatString(MSF_StringFormatUTF32 const& aStringFormat, char32_t* aBuffer, size_t aBufferLength, size_t anOffset = 0, char32_t* (*aReallocFunction)(char32_t*, size_t, void*) = nullptr, void* aUserData = nullptr);
extern intptr_t MSF_FormatString(MSF_StringFormatWChar const& aStringFormat, wchar_t* aBuffer, size_t aBufferLength, size_t anOffset = 0, wchar_t* (*aReallocFunction)(wchar_t*, size_t, void*) = nullptr, void* aUserData = nullptr);

//-------------------------------------------------------------------------------------------------
// MSF_MakeStringFormat gives you an object you can use to perform the printf into any buffer
// 
// Warning: If conversions are used in conjunction with trying to capture the results of MSF_MakeStringFormat
// locally, temporaries will get destroyed. Try to always use MSF_MakeStringFormat when passing directly
// into another function: i.e. LogMessage(MSF_MakeStringFormat(...));
//-------------------------------------------------------------------------------------------------
template<typename ...Args>
MSF_StringFormatContainer<char, Args...> MSF_MakeStringFormat(char const* aString, Args const&... args) { return MSF_StringFormatContainer<char, Args...>(aString, args...); }
template<typename ...Args>
MSF_StringFormatContainer<char16_t, Args...> MSF_MakeStringFormat(char16_t const* aString, Args const&... args) { return MSF_StringFormatContainer<char16_t, Args...>(aString, args...); }
template<typename ...Args>
MSF_StringFormatContainer<char32_t, Args...> MSF_MakeStringFormat(char32_t const* aString, Args const&... args) { return MSF_StringFormatContainer<char32_t, Args...>(aString, args...); }
template<typename ...Args>
MSF_StringFormatContainer<wchar_t, Args...> MSF_MakeStringFormat(wchar_t const* aString, Args const&... args) { return MSF_StringFormatContainer<wchar_t, Args...>(aString, args...); }
//-------------------------------------------------------------------------------------------------
// snprintf style calls to print into static buffers. The variations are to support a wide range
// of character and size types but not too many.
//-------------------------------------------------------------------------------------------------
template<uint32_t Size, typename ...Args>
intptr_t MSF_Format(char (&aBuffer)[Size], char const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char, Args...>(aString, args...), aBuffer, Size); }
template<typename ...Args>
intptr_t MSF_Format(char* aBuffer, size_t aSize, char const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char, Args...>(aString, args...), aBuffer, aSize); }
template<typename ...Args>
intptr_t MSF_Format(char* aBuffer, int aSize, char const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char, Args...>(aString, args...), aBuffer, aSize); }
//-------------------------------------------------------------------------------------------------
template<uint32_t Size, typename ...Args>
intptr_t MSF_Format(char16_t(&aBuffer)[Size], char16_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char16_t, Args...>(aString, args...), aBuffer, Size); }
template<typename ...Args>
intptr_t MSF_Format(char16_t* aBuffer, size_t aSize, char16_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char16_t, Args...>(aString, args...), aBuffer, aSize); }
template<typename ...Args>
intptr_t MSF_Format(char16_t* aBuffer, int aSize, char16_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char16_t, Args...>(aString, args...), aBuffer, aSize); }
//-------------------------------------------------------------------------------------------------
template<uint32_t Size, typename ...Args>
intptr_t MSF_Format(char32_t(&aBuffer)[Size], char32_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char32_t, Args...>(aString, args...), aBuffer, Size); }
template<typename ...Args>
intptr_t MSF_Format(char32_t* aBuffer, size_t aSize, char32_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char32_t, Args...>(aString, args...), aBuffer, aSize); }
template<typename ...Args>
intptr_t MSF_Format(char32_t* aBuffer, int aSize, char32_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char32_t, Args...>(aString, args...), aBuffer, aSize); }
//-------------------------------------------------------------------------------------------------
template<uint32_t Size, typename ...Args>
intptr_t MSF_Format(wchar_t(&aBuffer)[Size], wchar_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<wchar_t, Args...>(aString, args...), aBuffer, Size); }
template<typename ...Args>
intptr_t MSF_Format(wchar_t* aBuffer, size_t aSize, wchar_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<wchar_t, Args...>(aString, args...), aBuffer, aSize); }
template<typename ...Args>
intptr_t MSF_Format(wchar_t* aBuffer, int aSize, wchar_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<wchar_t, Args...>(aString, args...), aBuffer, aSize); }
//-------------------------------------------------------------------------------------------------
// Add intptr_t when it's a different size to allow for more automatic capture without casting
//-------------------------------------------------------------------------------------------------
#if INT32_MAX != INTPTR_MAX
template<typename ...Args>
intptr_t MSF_Format(char* aBuffer, intptr_t aSize, char const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char, Args...>(aString, args...), aBuffer, aSize); }
template<typename ...Args>
intptr_t MSF_Format(char16_t* aBuffer, intptr_t aSize, char16_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char16_t, Args...>(aString, args...), aBuffer, aSize); }
template<typename ...Args>
intptr_t MSF_Format(char32_t* aBuffer, intptr_t aSize, char32_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<char32_t, Args...>(aString, args...), aBuffer, aSize); }
template<typename ...Args>
intptr_t MSF_Format(wchar_t* aBuffer, intptr_t aSize, wchar_t const* aString, Args const&... args) { return MSF_FormatString(MSF_StringFormatContainer<wchar_t, Args...>(aString, args...), aBuffer, aSize); }
#endif

//-------------------------------------------------------------------------------------------------
// Make a backup copy of string format structure. This will request an allocation to hold all relevant data.
//-------------------------------------------------------------------------------------------------
extern MSF_StringFormat const* MSF_CopyStringFormat(MSF_StringFormat const& aStringFormat, void* (*anAlloc)(size_t), bool anIncludeFormatString = true);
extern MSF_StringFormatUTF16 const* MSF_CopyStringFormat(MSF_StringFormatUTF16 const& aStringFormat, void* (*anAlloc)(size_t), bool anIncludeFormatString = true);
extern MSF_StringFormatUTF32 const* MSF_CopyStringFormat(MSF_StringFormatUTF32 const& aStringFormat, void* (*anAlloc)(size_t), bool anIncludeFormatString = true);
extern MSF_StringFormatWChar const* MSF_CopyStringFormat(MSF_StringFormatWChar const& aStringFormat, void* (*anAlloc)(size_t), bool anIncludeFormatString = true);

extern MSF_StringFormat const* MSF_CopyStringFormat(MSF_StringFormat const& aStringFormat, void* (*anAlloc)(size_t, void*), void* aUserData, bool anIncludeFormatString = true);
extern MSF_StringFormatUTF16 const* MSF_CopyStringFormat(MSF_StringFormatUTF16 const& aStringFormat, void* (*anAlloc)(size_t, void*), void* aUserData, bool anIncludeFormatString = true);
extern MSF_StringFormatUTF32 const* MSF_CopyStringFormat(MSF_StringFormatUTF32 const& aStringFormat, void* (*anAlloc)(size_t, void*), void* aUserData, bool anIncludeFormatString = true);
extern MSF_StringFormatWChar const* MSF_CopyStringFormat(MSF_StringFormatWChar const& aStringFormat, void* (*anAlloc)(size_t, void*), void* aUserData, bool anIncludeFormatString = true);

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !defined(MSF_DEFAULT_FMT_SIZE)
#define MSF_DEFAULT_FMT_SIZE 512
#endif
#if !defined(MSF_StrFmt)
#define MSF_StrFmt MSF_StrFmtUTF8<512>
#endif

template <int Size = MSF_DEFAULT_FMT_SIZE>
struct MSF_StrFmtUTF8
{
	template <typename... Args>
	MSF_StrFmtUTF8(char const* aString, Args const&... args) { MSF_FormatString(MSF_StringFormatContainer<char, Args...>(aString, args...), myString, Size); }

	operator char const* () { return myString; }

	char myString[Size];
};
template <int Size = MSF_DEFAULT_FMT_SIZE>
struct MSF_StrFmtUTF16
{
	template <typename... Args>
	MSF_StrFmtUTF16(char16_t const* aString, Args const&... args) { MSF_FormatString(MSF_StringFormatContainer<char16_t, Args...>(aString, args...), myString, Size); }

	operator char16_t const* () { return myString; }

	char16_t myString[Size];
};
template <int Size = MSF_DEFAULT_FMT_SIZE>
struct MSF_StrFmtUTF32
{
	template <typename... Args>
	MSF_StrFmtUTF32(char32_t const* aString, Args const&... args) { MSF_FormatString(MSF_StringFormatContainer<char32_t, Args...>(aString, args...), myString, Size); }

	operator char32_t const* () { return myString; }

	char32_t myString[Size];
};
template <int Size = MSF_DEFAULT_FMT_SIZE>
struct MSF_StrFmtWChar
{
	template <typename... Args>
	MSF_StrFmtWChar(wchar_t const* aString, Args const&... args) { MSF_FormatString(MSF_StringFormatContainer<wchar_t, Args...>(aString, args...), myString, Size); }

	operator wchar_t const* () { return myString; }

	wchar_t myString[Size];
};
