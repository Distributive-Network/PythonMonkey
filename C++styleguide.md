# Bifrost 2 C++ Style Guide

This document is a style guide for C++ code for the Bifrost 2 project. It takes inspiration from both the Google C++ Style Guide and Wes Garland's JS Style Guide.

Last Updated: August 2022

## Cardinal Rules

 1. You are an intelligent sentient being working on code that you can see. While rules, linters, and style guides are great innovations, they do not have the same common sense you possess.   You are free to ignore the style guide, on occasion, when it makes sense to do so.
 2. We do not change lines unless we are changing code.  Incorrectly-indented code, for example, should be left incorrectly indented if it has already been been merged that way, until it needs to be modified.  It is more important to preserve the git history than it is to have beautiful code.
 *Corollary: please do your best to submit only beautiful code for MR,  as it won't be fixed for purely cosmetic reasons later!*

## C++ Version
  Code should target C++17. Do not use non-standard extensions.

## Header Files
 - In general, every `.cpp` file should have an associated `.h` file. There are some common exceptions, such as unit tests and small `.cpp` files containing just a main() function.
 - Header files should be self-contained (compile on their own) and end in `.h`.
 - All header files should have `#define` guards to prevent multiple inclusion. The format of the symbol name should be `BIFROST_PATH_FILE_H`. To guarantee uniqueness, they should be based on the full path in the project's source tree. For example, the file `/include/foo/bar.h` should have the following guard:
```cpp
#ifndef BIFROST_FOO_BAR_H
#define BIFROST_FOO_BAR_H
...
#endif // BIFROST_FOO_BAR_H
```
 - If a source or header file refers to a symbol defined elsewhere, the file should directly include a header file which properly intends to provide a declaration or definition of that symbol. It should not include header files for any other reason.
 - Do not rely on transitive inclusions. This allows people to remove no-longer-needed `#include` statements from their headers without breaking things elsewhere. This also applies to related headers - `foo.cpp` should include `bar.h` if it uses a symbol from it even if `foo.h` includes `bar.h`.
 - Define functions `inline` only when they are small, approximately 10 lines or fewer. Feel free to inline getters and mutators.
 - Includes headers in the following order: Related header, C system headers, C++ standard library headers, other libraries' headers, other Bifrost2 headers.
 - All Bifrost 2 header files should be listed as descendants of the project's source directory without use of UNIX directory aliases . (the current direcotry) or .. (the parent directory). For example, /include/foo/bar.h should be included as:
 ```cpp
 #include "foo/bar.h"
 ```
 - Within each section the includes should be ordered alphabetically.

## Scoping
 - Place code in a namespace. Do not use inline namespaces. 
 - Terminate namespaces with comments of the form ``// namespace foo``.
 ```cpp
 namespace foo {
  ...
 } // namespace foo
 ```
 - Name spaces wrap the entire source file after includes.
 - Do not declare anything in namespace ``std``.
 - Do not use a using-directive to make all names from a namespace available.
 - Do not use namespace aliases at namespace scope in header files.
 - When definitions in a `.cpp` file do not need to be referenced outside that file, give them internal linkage by placing them in an unnamed namespace or declaring them ``static``. Do not do either of these things in `.h` files.
 - Format unnamed namespaces like named spaces. In the terminating comment, leave the namespace name empty.
 ```cpp
 namespace {
  ...
 } // namespace
 ```
  - Place a function's variables in the narrowest scope possible, and initialize variables in the declaration.
  - Do not use objects with [static storage duration](https://en.cppreference.com/w/cpp/language/storage_duration#Storage_duration) unless they are [trivially destructible](https://en.cppreference.com/w/cpp/types/is_destructible) (has no user-defined or virtual destructor and all bases and non-static members are trivially destructible).

## Classes
 - Avoid virtual method calls in constructors.
 - Do not define implicit conversions. Use the explicit keyword for conversion operators and single-argument constructors.
 - A class's public API must make clear whether the class is copyable, move-only, or neither.
 ```cpp
 class Copyable {
 public:
  Copyable(const Copyable& other) = default;
  Copyable& operator=(const Copyable& other) = default;

  // The implicit move operations are suppressed by the declarations above.
  // You may explicitly declare move operations to support efficient moves.
};

class MoveOnly {
 public:
  MoveOnly(MoveOnly&& other) = default;
  MoveOnly& operator=(MoveOnly&& other) = default;

  // The copy operations are implicitly deleted, but you can
  // spell that out explicitly if you want:
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
};

class NotCopyableOrMovable {
 public:
  // Not copyable or movable
  NotCopyableOrMovable(const NotCopyableOrMovable&) = delete;
  NotCopyableOrMovable& operator=(const NotCopyableOrMovable&)
      = delete;

  // The move operations are implicitly disabled, but you can
  // spell that out explicitly if you want:
  NotCopyableOrMovable(NotCopyableOrMovable&&) = delete;
  NotCopyableOrMovable& operator=(NotCopyableOrMovable&&)
      = delete;
};
```
 - Use a `struct` only for passive objects that carry data; everything else is a `class`. 
 - Prefer to use a `struct` instead of a pair or tuple whenever the elements can have meaningful names.
 - Overload operators as long as their meaning is obvious, unsurprising, and consistent with the corresponding built-in operators. However, do not overload `&&`, `||`, `.`, `,` (comma), or unary `&`, or `operator""` i.e, do not introduce user-defined literals.
 - Make classes' data members private, unless they are constants, and add getters if necessary.
 - Group similar declarations together. A class definition should usually start with `public:`, followed by `protected:` then `private:`, omitting empty sections. Within each section, prefer grouping similar kinds of declarations together, and prefer the following order:
 1. Types and type aliases (typedef, using, enum, nested structs and classes, and friend types)
 2. Static constants
 3. Factory functions
 4. Constructors and assignment operators
 5. Destructor
 6. All other functions (static and non-static member functions, and friend functions)
 7. Data members (static and non-static)

 ## Functions
 - Prefer using return values over output parameters.
 - Prefer returning by value, or failing that, by reference. Avoid returning a pointer unless it can be NULL.
 - Non-optional input parameters should usually be values or `const` references.
 - Non-optional output and input/output parameters should usually be references (which cannot be NULL). 
 - Generally, use `std::optional` to represent optional by-value inputs, and use a `const` pointer when the non-optional form would have used a reference. 
 - Use non-`const` pointers to represent optional outputs and optional input/output parameters.
 - When ordering function parameters, put all input-only parameters before any output parameters. Variadic functions may require you to break this rule.
 - Prefer short functions. If a function exceeds ~40 lines, consider how it can be broken up without harming the structure of the program.
 - Use overloaded functions (including constructors and operator overloads) only if a reader looking at the call site can get a good idea of what is happening without having to first figure out exactly which overload is being called.
 - Default arguments are allowed as long as they always have the same value, but similar caution with overloaded functions should be used.
 - Only use trailing return types if using the ordinary syntax is impractical or less readable.

## Other C++ Features
 - Only use rvalue references for move constructors and move assignment operators, &&-qualified methods that logically "consume" *this, for supporting [perfect forwarding](https://en.cppreference.com/w/cpp/utility/forward), or for defining pairs of overloads such as ones taking `Foo&&` and `const Foo&`.
 - Friend functions should usually be defined in the same file as the related class.
 - Avoid using run-time type information (`typeid` or `dynamic_cast`). Using RTTI in tests is fine.
 - In APIs, use `const` wherever it is meaningful and accurate. `constexpr` is sometimes a better choice.
 - Avoid defining macros, especially in header files, unless absolutely necessary. Prefer using `inline` functions, enums, and `const` variables. All macros must be prefixed with `BIFROST`.
  - Use `nullptr` for pointers, and `'\0'` for chars (not the `0` literal).
  - Prefer using `sizeof(varname)` over `sizeof(type)`. 
  - Only use type deduction (`auto` variables, return type deduction, etc.) if it makes the code clearer or safer. Do not use it just to avoid the inconvenience of writing an explicit type.

## Naming Rules
 - Use names that describe the purpose or intent of the thing. Avoid abbreviations that would likely be unknown to someone outside of this project, especially acronyms and initialisms. Universally known abbreviations are fine, like `i` for an iterator or `T` for a template parameter.
 - Filenames use `kebab-case`
 - Type names (classes, structs, type aliases, enums, type template parameters) use `CamelCaseWithUppercaseFirstCharacter` i.e. `PascalCase`
 - Variable names use `snake_case`, with the exception of `const` or `constexpr` variables, which use `UPPER_CASE`
 - Function names use `camelCase`
 - Namespace names use `snake_case`
 - Enums use `UPPER_CASE`
 - Macros use `UPPER_CASE`

## Comments
The following rules describe what and where to comment, but remember that while comments are important, the best code is self-documenting. Giving sensible names to things is much better than using obscure names that require comments for explanation.
 - Familiarize yourself with [Doxygen](https://www.doxygen.nl/manual/), which is what this project uses for documentation.
 - Do not duplicate comments in header and implementation files, as they will inevitably diverge.
 - For file comments, include an author line, and a date line for when the file was added. If you make significant changes to a file, add yourself as an author to the author line. The file-level comment should broadly describe the contents of the file, and how the abstractions are related. A 1 or 2 sentence file-level comment may be sufficient. The detailed documentation about individual abstractions belongs with those abstractions, not at the file level.
 - Every non-obvious class or struct declaration should have an accompanying comment that describes what it is for and how it should be used. When sufficiently separated (e.g., .h and .cc files), comments describing the use of the class should go together with its interface definition; comments about the class operation and implementation should accompany the implementation of the class's methods.
 ```cpp
/** Iterates over the contents of a GargantuanTable.
 Example:
    std::unique_ptr<GargantuanTableIterator> iter = table->NewIterator();
    for (iter->Seek("foo"); !iter->done(); iter->Next()) {
      process(iter->key(), iter->value());
    }
*/
class GargantuanTableIterator {
  ...
};
```
 - Almost every function declaration should have comments immediately preceding it that describe what the function does and how to use it. These comments may be omitted only if the function is simple and obvious (e.g., simple accessors for obvious properties of the class). Private methods and functions declared in `.cpp` files are not exempt.
 ```cpp
/** Returns an iterator for this table, positioned at the first entry
  lexically greater than or equal to `start_word`. If there is no
  such entry, returns a null pointer. The client must not use the
  iterator after the underlying GargantuanTable has been destroyed.

  This method is equivalent to:
    std::unique_ptr<Iterator> iter = table->NewIterator();
    iter->Seek(start_word);
    return iter;
  
  @author John Smith
  @date August 2022

  @param start_word - The entry in the table to begin iterating from
*/
std::unique_ptr<Iterator> GetIterator(absl::string_view start_word) const;
```
 - When documenting function overrides, focus on the specifics of the override itself, rather than repeating the comment from the overridden function. In many of these cases, the override needs no additional documentation and thus no comment is required.
 - Tricky or complicated code blocks should have comments before them explaining the implementation.
 - Do not state the obvious. In particular, don't literally describe what code does, unless the behavior is nonobvious to a reader who understands C++ well. Instead, provide higher level comments that describe why the code does what it does, or make the code self describing.
 - Use TODO comments for code that is temporary, a short-term solution, or good-enough but not perfect. TODOs should include the string TODO in all caps, followed by the name, e-mail address, bug ID, or other identifier of the person or issue with the best context about the problem referenced by the TODO. 

## Formatting
 ### Character Sets
 - UTF-8 formatting must be used. Use of characters above code point 127 (end of US-ASCII) in source code is discouraged, but permitted when they transmit domain-specific information (for example, using the Greek alpha symbol to represent a threshold value in a statistics module).
 - Hex encoding is also OK, and encouraged where it enhances readability — for example, "\xEF\xBB\xBF", or, even more simply, u8"\uFEFF", is the Unicode zero-width no-break space character, which would be invisible if included in the source as straight UTF-8.
 - Use only spaces, and indent 2 spaces at a time.
 - No tabs (code point 9) allowed in files
 - No carriage-returns (code point 13) allowed in files

### Floating Pointer Numbers
 - Floating-point literals should always have a radix point, with digits on both sides, even if they use exponential notation. Readability is improved if all floating-point literals take this familiar form, as this helps ensure that they are not mistaken for integer literals, and that the E/e of the exponential notation is not mistaken for a hexadecimal digit. It is fine to initialize a floating-point variable with an integer literal (assuming the variable type can exactly represent that integer), but note that a number in exponential notation is never an integer literal.

### Conditionals
  - In an `if` statement, including its optional else if and else clauses, put one space between the if and the opening parenthesis, and between the closing parenthesis and the curly brace (if any), but no spaces between the parentheses and the condition or initializer. If the optional initializer is present, put a space or newline after the semicolon, but not before.
  - Braces are always on their own line, and matching opening and closing braces have the same level of indentation. Braces may be omitted for one-statement blocks.
  ```cpp
  if (a)
  {
    b = 42;
    return 1;
  }
  else if (b)
    return 0;
  else
  {
    b = 41;
    return -1;
  }
  ```

### Loops and Switches
 - Switch statements may use braces for blocks. `case` blocks in `switch` statements may use braces. If not conditional on an enumerated value, `switch` statements should always have a `default` case.
 - Fall-through from one case label to another must be annoted using the [[fallthrough]]; attribute, unless there are consecutive case levels without intervening code.
 ```cpp
 switch (x) 
 {
  case 41:  // No annotation needed here.
  case 43:
    if (dont_be_picky) 
    {
      // Use this instead of or along with annotations in comments.
      [[fallthrough]];
    } else 
    {
      CloseButNoCigar();
      break;
    }
  case 42:
    DoSomethingSpecial();
    [[fallthrough]];
  default:
    DoSomethingGeneric();
    break;
}
```
 - Braces are optional for single-statement loops. Empty loop bodies should use either empty braces or `continue`.

 ### Pointers and References
 - No spaces around period or arrow. Pointer operators do not have trailing spaces. The following are examples of correctly-formatted pointer and reference expressions:
```cpp
x = *p;
p = &x;
x = r.y;
x = r->y;
```
 - When referring to a pointer or reference (variable declarations or definitions, arguments, return types, template parameters, etc), you may place the space before or after the asterisk/ampersand. In the trailing-space style, the space is elided in some cases (template parameters, etc).
```cpp
// These are fine, space preceding.
char *c;
const std::string &str;
int *GetPointer();
std::vector<char *>

// These are fine, space following (or elided).
char* c;
const std::string& str;
int* GetPointer();
std::vector<char*>  // Note no space between '*' and '>'
```

### Preprocessor Directives
 - The hash mark that starts a preprocessor directive should always be at the beginning of the line. Even when preprocessor directives are within the body of indented code, the directives should start at the beginning of the line.
 ```cpp
 // Good - directives at beginning of line
  if (lopsided_score) {
#if DISASTER_PENDING      // Correct -- Starts at beginning of line
    DropEverything();
# if NOTIFY               // OK but not required -- Spaces after #
    NotifyClient();
# endif
#endif
    BackToNormal();
  }
```

