[![Actions Status](https://github.com/kudaba/ModernStringFormatTest/workflows/Full%20Build/badge.svg)](https://github.com/kudaba/ModernStringFormatTest/actions)

# Modern String Format
This library is intended to be a sprintf replacement with the following features:
* Type detection to eliminate need for size specifiers
* Extended formatting options that allow use of CSharp "{0}" and Hybrid "{}" style formatting
* Mid process reallocation to eliminate having to process strings multiple times
* Extension system allows binding to additional user types
* Option to be fully compliant with existing platforms or to customize some printing aspects for consistency across platforms
* Faster than existing platform implementations
* Compile time string validation without having to modify most call sites (requires full c++20 support, GCC 11, Clang 12 or Visual Studio 2022 (platform toolset 143))
* Support all character types: ``char, wchar_t, char8_t, char16_t, char32_t``. We assume that all 8 bit types are UTF8 and all 16 bit types are UTF16. ``wchar_t`` Can be either UTF16 or UTF32 depending on platform and build options.

*Note: Since this library is intended to remove size requirements (%d works for all integer types) and add custom printing options ("{}"), it does not work with the standard printf compile time checks introduced in GCC/Clang. You will get compile errors for completely unsupported types, but any other errors are runtime only.*

*Note: This repository is designed to be as minimal as possible and as such no project files or anything extraneous is included. It only contains the source, license and this readme file. A separate repository exists that includes unit and performance testing as well as project file generation. See https://github.com/kudaba/ModernStringFormatTest*

## Example

```cpp
char output[256];
MSF_Format(output, "This sure is printing! %s:%d:%f", "hell's yeah!", 42, 42.424242f); // This sure is printing!hell's yeah!:42:42.424240
// OR
MSF_Format(output, "This sure is printing! {0}:{1}:{2}", "hell's yeah!", 42, 42.424242f); // This sure is printing!hell's yeah!:42:42.4242
// OR
MSF_Format(output, "This sure is printing! {}:{}:{}", "hell's yeah!", 42, 42.424242f); // This sure is printing!hell's yeah!:42:42.4242
// OR
MSF_Format(output, "This sure is printing! {}:{,8:x}:%.3f", "hell's yeah!", 42, 42.424242f); // This sure is printing!hell's yeah!:      2a:42.424
```

```cpp
static char* locRealloc(char* aString, size_t aSize, void* aUserData)
{
	char** outString = (char**)aUserData;
	return *outString = (char*)realloc(aString, aSize);
}

char* string = nullptr;

// Print
MSF_FormatString(MSF_MakeStringFormat("Hello."), string, 0, 0, &locRealloc, &string); // Hello.

// Append
MSF_FormatString(MSF_MakeStringFormat(" Oh hi there."), string, strlen(string), strlen(string), &locRealloc, &string); // Hello. Oh hi there.

free(string);
```
*Note: don't use raw strings like this, this is just an example*


## Setup

Minimum Requirements: c++11 without compile time validations, c++20 with compile time validation

If using a separate project/makefile to build MSF into a library then you must use header injection to affect any config settings. For gcc/clang use the ``-include`` option, for MSVC use ``/FI``.

If using the ``#include`` method to include the cpp files locally then just ``#define`` any config options before including the cpp files.

## Usage

For basic usage as a replacement for ``sprintf`` you can replace call sites with  ``MSF_Format`` and it should 'just work'. It's possible that the compile time validation might cause a few issues here and there but should accept most callsites as-is.

If writing a custom capture, make sure to take in your format string using the MSF_STRING macro to enable compile time validation support:
```cpp
template <typename... Args>
void Format(MSF_STRING(char) aFormat, Args... someArgs)
{
    MSF_FormatString(MSF_MakeStringFormat(aFormat, someArgs...), myString, myCapacity);
}
```

If you want to create the equivalent of ``vsnprintf`` just take in ``MSF_StringFormatTemplate`` as your main argument:
```cpp
void Format_va(MSF_StringFormat const& aFormat)
{
    MSF_FormatString(aFormat, myString, myCapacity);
}
```
There are handy typedefs for the various string sizes (i.e. ``MSF_StringFormat`` for char, ``MSF_StringFormatUTF16`` for char16_t), or you can use the base template type directly: ``MSF_StringFormatTemplate<Char>``.

Some types might not be able to properly convert into a standard type (i.e. string class). For simple cases you can use MSF_DEFINE_TYPE_CONVERSION to specify how to convert the type:
```cpp
MSF_DEFINE_TYPE_CONVERSION(std::string, value.c_str());
```

### Format specifiers

* C style - All supported C style (%d etc) printf specifiers match existing standards. The main difference is you do not need size options (i.e. %ld), they will work but will be ignored in favor of the real type.
  * See https://en.cppreference.com/w/c/io/fprintf
  * See http://msdn.microsoft.com/en-us/library/56e442dc.aspx
* CSharp style - This style uses curly braces and a position specifier ("{0}"). This allows for reuse and reordering of arguments. Format specifiers are mostly the supported, they are restricted to types and formats that are supported by the c standard as the time of writing. This may change in the future. csharp style specifiers are NOT compatible with C style or Hybrid style.
  * See https://docs.microsoft.com/en-us/dotnet/api/system.string.format?view=net-6.0#control-formatting
* Hybrid style - The hybrid style is simply the CSharp style without the position specifier. It uses relative positioning the same as C style but automatically detects the type the same as the CSharp style. Hybrid style and C style can be used together in the same print statement.

C format: ``%[-+ #0][[-]width][.[precision]type``  
C#/hybrid format: ``{[index][,width][:type[precision]]}``

## Features

### Type capture

Using templates and variadic arguments, we are able to capture the type of each argument. This is used to eliminate the need to size specifiers and to enable some rudimentary compile time checks against input. The type capture is also what allows us to support using untyped print specifiers "{0}" and "{}".

### Reallocation Callback

If you manually call MSF_FormatString you can pass in callbacks to allow for dynamically resizing the buffer during the print process. This allows for a big performance win against using sprintf since it eliminates having to call it with a nullptr to get the print size ahead of time. You can even pass in an existing string with an offset to format at the end of an existing string. See the unit test Realloc in MSF_FormatTest.cpp (TODO: Make link) for an example of this in action.

*Note: At the time of writing this, the reallocation system will overestimate the required length of a string in exchange for doing a single allocation between the prepare and printing steps.*

### Extension Types

MSF Supports users to be able to register their own types, validation and printing functions. This is done in a two step process.

1. Publicly declare the support for the type. This can easily be done using the provided macro:
    ```
    MSF_DEFINE_USER_PRINTF_TYPE(MyType, 0)
    ```
    This allows the compiler to pick up the type and pass it into the print system with the appropriate type info.
    *Note: The number here is 0 based index from ``0-(64-<number of built in types>``). The actual type id becomes ``1 << (index+<number of built in types>)``. This was to simplify declarations.*
2. Register Validation and Print functions
    To perform the actual printing you need to provide callbacks to the system to be able to tell it how much space you need as well as writing to the actual buffer. See <https://github.com/kudaba/ModernStringFormatTest/blob/main/src/Tests/MSF_CustomTypesTest.cpp> for an example of this.
3. If Registering a new print character for your type you should also declare it globally to support compile time validation:
    ```
    MSF_MAP_CHAR_TO_TYPES('t', MSF_StringFormatTypeLookup<MyCustomType>::ID);
    ```
    *Note: It's possible to make a print character support multiple types, so make sure to OR(|) together all supported types in the global declaration.*

You can find an example of this in MSF_CustomTypesTest.cpp (TODO: Make link)

### Error Handling

If an error is detected when trying to format a string, users can choose how they want the system to handle it:
* Silent: Do nothing but return the error code as a negative value
* WriteString: Attempt to print error information directly into the provided string. This method WILL attempt to reallocate the string if there is not enough space, otherwise it will use whatever space if available to print as much info as it can.
* Assert: Any errors will instantly trigger an assert to break into code.

This setting can be adjusted globally or per-thread as needed and can be changed at run-time. This is especially useful for writing unit tests against Extension types.

See MSF_FormatPrint.h for more info

### Assert Handling

There is a rudimentary Assert system in-place to enforce certain assumptions. All efforts are made to ensure the code will run safely regardless, however if you are using any Extension Types, it's highly recommended that you run with Asserts on in debug builds to make sure you are matching validation lengths with print lengths.

The Assert system allows users to override both the reporting of asserts and whether to break in code if asserts will cause the code to break or not. See MSF_Assert.h for more info.

### Compile Time Validation

By using some tricky capture to pick between using a ``consteval`` function or not we are able to run full validation code at compile time. The assumption here, that makes this work seamlessly, is that capturing a string array (i.e. ``char const[N]``) likely means it's coming from a compile constant. This does mean that some runtime char arrays might need to be cast to a pointer, or that compile time strings that resolve to ``char const*`` get missed, but this should capture the majority of instances in a give code base. If some happen to be missed then there are still runtime checks that will catch any issues so the formatting is still perfectly safe.
