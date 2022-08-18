# Bifrost 2 C++ Style Guide

### Meta

* Do not make changes to the coding standard that affect existing code unless
  justified. However, if such a change is warranted, change the Uncrustify
  configuration to conform as necessary and fix all affected code in a single,
  well-marked commit.
* Prioritize the present over the past. Try to get style right the first time,
  but if there is code that unambiguously does not follow the style, fix it in a
  distinct, clearly marked commit. All else being equal, a consistent,
  easy-to-navigate-at-a-glance code base that is consistent with the style
  guidelines is superior to a pristine git history.
* In the event of ambiguity or omission from this coding standard, understand
  and follow the style of the existing code and update the coding standard
  if necessary. The existing code is the authority.
* You are an intelligent sentient being working on code that you can see. Style
  guides do not have the same common sense you possess. You are free to ignore
  the style guide, on occasion, when it makes sense to do so.

### General

#### Naming

* Do not abbreviate things unnecessarily.
* Use names that describe the purpose or intent of the thing. Avoid
  abbreviations, especially those that would likely be unknown to someone
  outside of this project (especially acronyms and initialisms). Universally
  known abbreviations are acceptable.
* File names must be unambiguous on case-insensitive file systems to address the
  ["Sean Connery" problem](https://youtu.be/PaFSkWfFhO0?t=117), whereby files
  "Swords" and "SWords" cannot co-exist in a directory. Spaces are represented
  as underscores, and if there are namespaces that cannot or should not be
  represented by subdirectories, they are delimited with hyphens. *Rationale:*
  Most UIs treat dashes as word delimiters, making double-click selection do the
  right thing if underscores are used as spaces in names, and hyphens delimit
  names. An example of this is the executable name `dcp-file_reader-test`, which
  is a test for the DCP::FileReader type; double-clicking anywhere in the
  "file_reader" portion of the name on MacOS causes only the "file_reader" name
  to be selected.

#### Formatting

* Indent with 2 spaces; no tabs.
* Try to limit lines to 80 characters.
* When splitting code across multiple lines:
  * Match indentation level to brace/bracket/parenthesis nesting level.
  * Place the opening brace/bracket/parentheses on the same line. *Rationale:*
    * Vertical scrolling is minimized.
  * Place the closing brace/bracket/parenthesis either on the same line as
    the opening brace/bracket/parenthesis, or at the start of its line.
  * Examples:
  ```cpp
  // C++
  if (
    condition && (
      anotherCondition || possiblyAnotherCondition
    )
  ) {
    do {
      ++index;
    } while (index < total);
  } else {
    return false;
  }
  ```
  ```cmake
  # CMake
  if(
    CONDITION AND (
      ANOTHER_CONDITION OR POSSIBLY_ANOTHER_CONDITION
    )
  )
    add_custom_target("${PROJECT_NAME}-check" ALL
      COMMAND
      "${CMAKE_CTEST_COMMAND}" "."
      "--build-config" "$<CONFIG>"
      "--output-on-failure"
      "--schedule-random"
      WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
      VERBATIM
    )
  endif()
  ```
* Do not place more than one statement on a line.
* Do not insert more than one blank line in a row.

#### Character Sets

* Assume `char` strings to be [UTF-8](http://utf8everywhere.org), and convert
  to/from other encodings as close to where they are needed as possible.
* Restrict source code to ASCII.
* No tabs (code point 9) allowed in files.
* No carriage-returns (code point 13) allowed in files that are committed to the
  repository. Git should be configured to add them automatically when checking
  out code on Windows, and remove them when checking in.

#### Literals

* Hex encoding in string literals may be used where permitted by the language,
  and encouraged where it enhances readability. For example, "\xEF\xBB\xBF", or,
  even more simply, u8"\uFEFF" in C++, is the Unicode zero-width no-break space
  character, which would be invisible if included in the source as straight
  UTF-8.
* Floating-point literals should always have a radix point, with digits on both
  sides, even if they use exponential notation. Readability is improved if all
  floating-point literals take this familiar form, as this helps ensure that
  they are not mistaken for integer literals, and that the E/e of the
  exponential notation is not mistaken for a hexadecimal digit. It is fine to
  initialize a floating-point variable with an integer literal (assuming the
  variable type can exactly represent that integer), but note that a number in
  exponential notation is never an integer literal.

### C and C++

For any best practices not covered by this standard, refer to
[C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
for general advice and references to even more code guideline documents.

#### Standard

* Use modern C++: target the latest C++ standard supported by the compilers,
  which at the time of writing is C++17. Do not use non-standard extensions.
  This should be enforced in CMake as follows:
  ```cmake
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_CXX_STANDARD "17")
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  ```

#### Source Files

* Use the following source file extensions:
  * .c: C source file
  * .cc: C++ source file
* Try to add a source file for every header file; however, it is not necessary
  if no symbols are defined by either the header or the source file (eg. if
  the header only contains preprocessor definitions), since this can in fact
  cause a "no symbols" warning when compiling library binaries.

#### Header Files

* Use the following header file extensions:
  * .h: C header file
  * .hh: C++ header file
* Header files should be self-contained (compile on their own).
* Make each header define one top-level thing (eg. `namespace`, `struct`,
  function) and name the header file for it.
* As the first non-comment line, each header file should have a `define` guard
  to prevent multiple inclusion, and the format of the name should map to the
  header file path relative to the include directory, whereby folders are
  CamelCaps, directory separators are underscores, and the name is terminated
  with an underscore. For example, the include guard for "dcp/file_reader.hh"
  would be `DCP_FileReader_`.
* Prefer enabling interprocedural optimization over inlining functions in the
  header file.

#### Includes

* Use angle brackets for including paths relative to CMake-specified library
  include paths, and double-quotes for including paths relative to the current
  file. *Rationale:* This is how GCC, at least, treats includes; see
  https://gcc.gnu.org/onlinedocs/cpp/Search-Path.html.
* Group includes in order if decreasing specificity (*Rationale:* This helps
  identify missing includes from headers), with each group separated by a blank
  line and ordered alphanumerically within:
	* The header corresponding to the current source file, if applicable.
	* Double-quoted includes.
	* Angle-braced includes, grouped by project in order of increasing generality.
* Use `extern "C"` only on the including side, and put these includes at the
  start of the appropriate include group. *Rationale:* This allows for inclusion
  of both pure C and C++-friendly C headers.
* Do not include .c/.cc source files.
* If a source or header file refers to a symbol defined elsewhere, the file
  should directly include a header file which properly intends to provide a
  declaration or definition of that symbol. It should not include header files
  for any other reason.
* Do not rely on transitive inclusions. This allows people to remove
  no-longer-needed `include` statements from their headers without breaking
  things elsewhere. This also applies to related headers: `foo.cc` should
  include `bar.hh` if it uses a symbol from it even if `foo.hh` includes
  `bar.hh`.

#### Naming

* Emulate namespaces in C and macros via naming. Name things as follows (despite
  most recommendations to use all caps for macro names, which results in
  ambiguity between spaces and namespace delimiters):
  ```cpp
  #if !defined(DCP_NamespaceName_StructName_)
    #define DCP_NamespaceName_StructName_

    #define DCP_NamespaceName_StructName_macroName(argumentName) !(argumentName)

    #define DCP_NamespaceName_StructName_definitionName 42

  namespace DCP::NamespaceName {

    struct StructName {

      int functionName(int argumentName) {
        int variableName = argumentName;
        return variableName;
      }

      int memberName;

    };

  }

  #endif
  ```

#### Namespaces and Scoping

* All declarations should be in a namespace, with the root namespace being
  `DCP`. Do not use inline namespaces for declarations.
* Each namespace must be represented by a subdirectory in both the `include` and
  `source` directory, and should reflect the relative path.
* Do not declare anything in namespace `std`.
* Do not use a `using` directive to make all names from a namespace available.
* Do not use namespace aliases at namespace scope in header files.
* When definitions in a source file do not need to be referenced outside that
  file, give them internal linkage by placing them in an anonymous namespace or
  declaring them static. Do not do either of these things in header files.
* Format unnamed namespaces like named spaces.
* Place a function's variables in the narrowest scope possible, and initialize
  variables in the declaration.
* Avoid global-scope or class-scope static duration objects unless they are a
  trivial type. Non-trivial static duration objects must be function-scoped in
  order to avoid the
  [static initialization order fiasco](https://en.cppreference.com/w/cpp/language/siof).

#### Classes

* Avoid virtual method calls in constructors.
* Mark all argument-based (i.e. function-like, vs. value-based) constructors as
  `explicit`. Only add implicit conversions if you really know what you are
  doing.
* A class's public API must make clear whether the class is copyable,
  move-only, or neither.
  ```cpp
  struct Copyable {
    Copyable(Copyable const &other) = default;
    Copyable &operator=(Copyable const &other) = default;

    // The implicit move operations are suppressed by the declarations above.
    // You may explicitly declare move operations to support efficient moves.
  };

  struct MoveOnly {
    MoveOnly(MoveOnly &&other) = default;
    MoveOnly &operator=(MoveOnly &&other) = default;

    // The copy operations are implicitly deleted, but you can
    // spell that out explicitly if you want:
    MoveOnly(MoveOnly const &) = delete;
    MoveOnly &operator=(MoveOnly const &) = delete;
  };

  struct NotCopyableOrMovable {
    // Not copyable or movable
    NotCopyableOrMovable(NotCopyableOrMovable const &) = delete;
    NotCopyableOrMovable &operator=(NotCopyableOrMovable const &) = delete;

    // The move operations are implicitly disabled, but you can
    // spell that out explicitly if you want:
    NotCopyableOrMovable(NotCopyableOrMovable &&) = delete;
    NotCopyableOrMovable& operator=(NotCopyableOrMovable &&) = delete;
  };
  ```
* Despite what most other coding standards say, only use `struct`, not `class`.
  *Rationale:* The `struct` default of public access and inheritance is more
  useful: there are almost always public members, and inheritance is almost
  always public.
* Declare `struct`s `final` where possible.
* Prefer to use a `struct` instead of a pair or tuple whenever the elements can
  have meaningful names.
* Prefer making `struct` data members private (accessible/modifiable via
  getters/setters), unless they are trivial constants or the class is intended
  to be used as a basic POD type.
* Group similar member declarations together. Order `struct` members as follows,
  omitting empty groups:
  * Primary:
    * Public (implicit)
    * Protected
    * Private
  * Secondary:
    * Types
    * Constants
    * Constructors:
      * Default
      * Implicit
      * Explicit
    * Operators
    * Functions
    * Destructor
    * Static data members
    * Non-static data members
  * Tertiary:
    * Static
    * Non-static

#### Functions

* Function attributes (eg. `[[nodiscard]]`) go on their own line.
* Document acceptable argument values and assert (rather than checking and
  throwing) since there is no reason to incur a performance penalty of an extra
  check for programmer error.
* Prefer using return values over output parameters.
* Prefer returning by value, or failing that, by reference. Avoid returning a
  pointer unless it can be null.
* Non-optional input parameters should usually be values or `const` references.
* Non-optional output and input/output parameters should usually be references
  (which cannot be null).
* Generally, use `std::optional` to represent optional by-value inputs, and use
  a `const` pointer when the non-optional form would have used a reference. Note
  that const pointers do not temporarily extend lifetimes the way that const
  references do.
* Use non-`const` pointers to represent optional outputs and optional
  input/output parameters.
* When ordering function parameters, put all input-only parameters before any
  output parameters. Variadic functions may require you to break this rule.
* Prefer pure functions with no side-effects, and breaking up code into such
  functions without harming the structure of the program.
* Use overloaded functions (including constructors and operator overloads) only
  if a reader looking at the call site can get a good idea of what is happening
  without having to first figure out exactly which overload is being called.
* Default arguments are allowed as long as they always have the same value, but
  similar caution with overloaded functions should be used.
* Only use trailing return types if using the ordinary syntax is impractical or
  less readable.

#### Statements

* *Always* enclose the body of an `if`, `for`, `do`, `while`, and `switch` in
  braces. *Rationale:*
  * If not bracing single-line bodies, adding debugging statements to the body
    requires temporary addition of braces (and then removal when finished
    debugging), which is annoying.
  * This requirement could have prevented the
    [Apple "goto fail" OpenSSL vulnerability](https://dwheeler.com/essays/apple-goto-fail.html).
* Do not indent `switch` `case` statements. *Rationale:*
  * This is consistent with `goto` labels and access specifiers.
* Fall-through from one case label to another must be annotated using the
  `[[fallthrough]];` attribute, unless there are consecutive case labels without
  intervening code.

#### Declarations

* Each declaration goes on its own line; avoid comma-separated declarations.
* Use `const` wherever it is meaningful and accurate. `constexpr` is sometimes a
  better choice.
* Use "east const":
  ```cpp
  Type /*modifiers*/ /*qualifiers*/
  ```
  Example:
  ```cpp
  int const *method(int unsigned const *const argument) const;
  ```
  *Rationale:*
  * `const` always follows the thing it applies to.
  * For a pointer `typedef`, `const` applies to the pointer, not the type; east
    `const` reflects this and `west` const does not.
* Right-cuddle (i.e. space before, but not after) `&`, `&&`, and `*` in
  declarations.
* Left-cuddle (i.e. space after, but not before) `...` (when related to variadic
  templates, vs. `catch (...)` or variadic macros) in declarations.
* Place no spaces around period or arrow: pointer operators do not have trailing
  spaces. The following are examples of correctly-formatted pointer and
  reference expressions:
  ```cpp
  x = *p;
  p = &x;
  x = r.y;
  x = r->y;
  ```
* Only declare `rvalue` references for move constructors and move assignment
  operators, &&-qualified methods that logically "consume" *this, forwarding
  references, or for defining pairs of overloads such as ones taking `Foo &&`
  and `Foo const &`.

#### Initialization

##### Variables

For a single value whose type can be inferred:
```cpp
auto /*modifiers*/ /*qualifiers*/ theVariable = theValue;
```

Otherwise, for an explicit (i.e. function-like) constructor:
```cpp
Type /*modifiers*/ /*qualifiers*/ theVariable(…);
```

Otherwise:
  ```cpp
  Type /*modifiers*/ /*qualifiers*/ theVariable{…};
  ```

##### In-Place Member Initialization

For data members, use C++17 in-place member initialization where possible, and
use the same initialization format as for variables.

##### Member Initializer Lists

For all data members that cannot be initialized in-place, initialize via
constructor initializer lists. These *must* initialize the members in order that
they appear.

For an explicit (i.e. function-like) constructor:
```cpp
theMember(…)
```

Otherwise:
```cpp
theMember{…}
```

##### Aggregate Initialization

For struct C-style aggregate initialization, use designated initializers only:
```cpp
{
  .thisMember = theValue,
  .thisOtherMember = theOtherValue
};
```

#### Expressions

* Use parentheses to make order of operations explicit.
* Avoid using run-time type information (`typeid` or `dynamic_cast`). Using RTTI
  in tests is fine.
* Use `nullptr` for pointers, and '\0' for chars (not the `0` literal).
* Prefer using `sizeof(varname)` over `sizeof(type)`.

#### Operators

* For operator overloads, elide space after `operator` keyword if possible;
  Doxygen has a hard time with this space.
* Overload operators as long as their meaning is obvious, unsurprising, and
  consistent with the corresponding built-in operators. However, do not overload
  `&&`, `||`, `.`, `,` (comma), or unary `&`, or `operator""`; i.e do not
  introduce user-defined literals.

#### Templates

* Use `typename` for template parameters.

#### Comments

The following rules describe what and where to comment, but remember that while
comments are important, the best code is self-documenting. Giving sensible names
to things is much better than using obscure names that require comments for
explanation.

* Do not use trailing comments for namespaces (eg. `} // namespace`).
  *Rationale:* Often these closing braces are far from the namespace
  declaration, making it easy to forget to update the comment when the namespace
  name changes.
* Familiarize yourself with [Doxygen](https://www.doxygen.nl/manual/), which is
  what this project uses for documentation.
* Do not duplicate comments in header and implementation files, as they will
  inevitably diverge.
* For file comments, include an author line, and a date line for when the file
  was added. If you make significant changes to a file, add yourself as an
  author to the author line. The file-level comment should broadly describe the
  contents of the file, and how the abstractions are related. A 1 or 2 sentence
  file-level comment may be sufficient. The detailed documentation about
  individual abstractions belongs with those abstractions, not at the file
  level.
* Every non-obvious class or struct declaration should have an accompanying
  comment that describes what it is for and how it should be used. When
  sufficiently separated (e.g., .hh and .cc files), comments describing the use
  of the class should go together with its interface definition; comments about
  the class operation and implementation should accompany the implementation of
  the class's methods.
* Almost every function declaration should have comments immediately preceding
  it that describe what the function does and how to use it. These comments may
  be omitted only if the function is simple and obvious (e.g., simple accessors
  for obvious properties of the class). Private methods and functions declared
  in `.cc` files are not exempt.
* When documenting function overrides, focus on the specifics of the override
  itself, rather than repeating the comment from the overridden function. In
  many of these cases, the override needs no additional documentation and thus
  no comment is required. Note that the `copydoc` Doxygen command can be used to
  duplicate documentation for overloads.
* Tricky or complicated code blocks should have comments before them explaining
  the implementation.
* Do not state the obvious. In particular, don't literally describe what code
  does, unless the behavior is nonobvious to a reader who understands C++ well.
  Instead, provide higher level comments that describe why the code does what it
  does, or make the code self describing.
* Use TODO comments for code that is temporary, a short-term solution, or
  good-enough but not perfect. TODOs should include the string TODO in all caps,
  followed by the name, e-mail address, bug ID, or other identifier of the
  person or issue with the best context about the problem referenced by the
  TODO.

#### Preprocessor

* Preprocessor directives follow their own indentation, independent of the
  indentation of the surrounding source code:
  ```cpp
  #if DCP_someValue
    if (someValue) {
    #if DCP_Namespace_anotherValue
      doAThing();
    #endif
      doAnotherThing();
    }
  #endif
  ```
* Avoid defining macros, especially in header files, unless absolutely
  necessary. Prefer using `inline` functions, enums, and `const` variables.

#### Miscellaneous

* Use `[[maybe_unused]]` in both declaration and definition; otherwise, Doxygen
  has trouble matching up declarations with definitions.
* Friend functions should usually be defined in the same file as the related
  `struct`.