# Changelog

## 2.3.0 (2022-12-31)

- FIX: Add for loop variables to local scope
- FIX: Add lexer support for leading-zero (bare) octal - e.g. 0777
- FIX: Allow non-local variable in rescue
- FIX: Allow trailing comma after for loop variables
- FIX: Fix for-loop with attr assignment
- FIX: Fix lexing of symbol keys with trailing punctuation
- FIX: Fix precedence of 'do' keyword in use with 'for'
- FIX: Parse for loop with splat assignment
- FIX: Use proper make on OpenBSD

## 2.2.0 (2022-10-24)

- FIX: Allow backreference as first arg of implicit call
- FIX: Exclude `?` and `!` from class/instance/global variable name
- FIX: Fix parsing of hashes containing assignments
- FIX: Parse aliases of unary operators and methods named as keywords
- FIX: Parse alias of keyword-like method to assignment method
- FIX: Parse alias/undef of assignment methods
- FIX: Parse backtick as method name
- FIX: Parse class/module definition in dynamic class/module
- FIX: Parse `def !`
- FIX: Parse `def foo::bar`
- FIX: Parse `foo(<<FOO)`
- FIX: Parse `for ... do` and `while ... do`
- FIX: Parse global variables beginning with `$_`
- FIX: Parse keywords/operators as method names for `::`
- FIX: Parse __LINE__ constant
- FIX: Parse more percent strings
- FIX: Parse more regexes
- FIX: Parse more stabby procs without parentheses
- FIX: Parse multiple parameters set to the same default value
- FIX: Parse parentheses-less stabby proc with keyword arg
- FIX: Parse undef of methods with special names
- FIX: Parse `unless ... then`
- FIX: Parse variable names starting with extended ASCII characters
- FIX: Raise SyntaxError for `def ====`
- FIX: Raise syntax error for `def +@.foo`
- FIX: Tokenize unterminated ternary at EOF
- FIX: Use lower precedence for block pass

## 2.1.0 (2022-08-12)

- FEAT: Parse for loops
- FIX: Fix bug parsing defined? with parens
- FIX: Fix parsing of keyword splat next to other keyword args
- FIX: Parse block pass after bare/implicit hash
- FIX: Parse if statements with match conditions
- FIX: Parse regexps with leading space preceeded by keywords
- FIX: Parse symbol key after super keyword
- FIX: Parse unless statements with match conditions
- FIX: Parse while/until statements with match conditions
- FIX: Reset block association level inside array

## 2.0.0 (2022-06-24)

- FEAT: Differentiate between bare/implicit hash and explicit one
- FIX: Fix calling colon2
- FIX: Parse implicit method calls with nth ref argument

## 1.2.1 (2022-06-16)

- FIX: Fix regression with unary/infix operators (+/-)

## 1.2.0 (2022-06-16)

- CHORE: Enable true random fuzzing
- FEAT: Add Node::debug() function to help with debugging
- FEAT: Parse more pattern matching cases
- FIX: Don't error if ext/natalie_parser/build.log isn't written yet
- FIX: Fix block association inside call with parentheses
- FIX: Fix bug negating an already-negative number
- FIX: Fix bug parsing unary operator as an infix operation
- FIX: Fix method definition with lonely ensure
- FIX: Fix parsing endless ranges inside case/when and other odd places
- FIX: Make sure every token knows if it has preceding whitespace
- FIX: Parse method calls with constant receiver

## 1.1.1 (2022-06-04)

- FIX: Workaround for clang declspec bug

## 1.1.0 (2022-06-04)

- CHORE: Add ccache and compiledb for the ext/natalie_parser directory
- CHORE: Add tests for numbered block arg shorthand
- FEAT: Parse arg forwarding (...) shorthand
- FEAT: Parse complex and rational numbers
- FIX: Fix panic when closing word array delimiter is not found
- FIX: Fix precedence bug with op assign operators (+= et al)

## 1.0.0 (2022-06-03)

### Summary

This is the initial public release. The 1.0 milestone was chosen as soon
as NatalieParser was useful for integration back with the upstream Natalie
compiler project, i.e. it could fully replace RubyParser as the parser
in use by Natalie.

That is not to say that NatalieParser is _complete_ -- it is merely _useful_.

These are the features known to still be missing in this release:

- [ ] Support different source encodings
- [ ] Support more of the Ruby 3.0 syntax
  - [ ] Argument forwarding (`...`)
  - [ ] Pattern matching
  - [ ] Numbered block parameters (`_1`, `_2`, etc.)
  - [ ] Non-ASCII identifiers
  - [ ] Rational and Complex literals (`1r` and `2i`)
