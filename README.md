[![Actions Status](https://github.com/kudaba/ModernStringFormatTest/workflows/Full%20Build/badge.svg)](https://github.com/kudaba/ModernStringFormatTest/actions)

# Modern String Format
This library is intended to be a sprintf replacement with the following features:
* Type detection to eliminate need for size specifiers
* Extended formatting options that allow use of CSharp "{0}" and Hybrid "{}" style formatting
* Mid process reallocation to eliminate having to process strings multiple times
* Extension system allows binding to additional user types
* Option to be fully compliant with existing platforms or to customize some printing aspects for consistency across platforms
* Faster than existing platform implementations
* Compile time string validation without having to modify most call sites (requires full c++20 support, GCC/Clang 11 and Visual Studio 2022 (platform toolset 143))

Note: Since this library is intended to remove size requirements (%d works for all integer types) and add custom printing options ("{}"), it does not work with the standard printf compile time checks introduced in GCC/Clang. You will get compile errors for completely unsupported types, but any other errors are runtime only.

Note: This repository is designed to be as minimal as possible and as such no project files or anything extraneous is included. It only contains the source, license and this readme file. A separate repository exists that includes unit and performance testing as well as project file generation. See https://github.com/kudaba/ModernStringFormatTest

## Setup

Minimum Requirements: c++11x

If using a separate project/makefile to build MSF into a library then you must use header injection to affect any config settings. For gcc/clang use the ``-include`` option, for MSVC use ``/FI``.

If using the ``#include`` method to include the cpp files locally then just ``#define`` any config options before including the cpp files.

None of the config options have any public facing affect so they don't need to be set ahead of any header files with the exception of customizing the assert system.

## Usage

For basic usage as a replacement for sprintf you can replace call sites with  MSF_Format and it should 'just work'.

### Format specifiers

* C style - All C style (%d etc) printf specifiers match existing standards. The main difference is you do not need size options (i.e. %ld), they will work but will be ignored in favor of the real type.
  * See https://en.cppreference.com/w/c/io/fprintf
  * See http://msdn.microsoft.com/en-us/library/56e442dc.aspx
* CSharp style - This style uses curly braces and a position specifier ("{0}"). This allows for reuse and reordering of arguments. Format specifiers are mostly the supported, they are restricted to types and formats that are supported by the c standard as the time of writing. This may change in the future. csharp style specifiers are NOT compatible with C style or Hybrid style.
  * See https://docs.microsoft.com/en-us/dotnet/api/system.string.format?view=net-6.0#control-formatting
* Hybrid style - The hybrid style is simply the CSharp style without the position specifier. It uses relative positioning the same as C style but automatically detects the type the same as the CSharp style. Hybrid style and C style can be used together in the same print statement.

## Features

### Type capture

Using templates and variadic arguments, we are able to capture the type of each argument. This is used to eliminate the need to size specifiers and to enable some rudimentary compile time checks against input. The type capture is also what allows us to support using untyped print specifiers "{0}" and "{}".

### Reallocation Callback

If you manually call MSF_FormatString you can pass in callbacks to allow for dynamically resizing the buffer during the print process. This allows for a big performance win against using sprintf since it eliminates having to call it with a nullptr to get the print size ahead of time. You can even pass in an existing string with an offset to format at the end of an existing string. See the unit test Realloc in MSF_FormatTest.cpp (TODO: Make link) for an example of this in action.

Note: At the time of writing this, the reallocation system will overestimate the required length of a string in exchange for doing a single allocation between the prepare and printing steps.

### Extension Types

MSF Supports users to be able to register their own types, validation and printing functions. This is done in a two step process.

1. Publicly declare the support for the type. This can easily be done using the provided macro:
    ```
    MSF_DEFINE_USER_PRINTF_TYPE(MyType, 0)
    ```
    This allows the compiler to pick up the type and pass it into the print system with the appropriate type info.
2. Register Validation and Print functions
    To perform the actual printing you need to provide callbacks to the system to be able to tell it how much space you need as well as writing to the actual buffer.

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
