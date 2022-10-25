# Natalie Parser

[![Gem Version](https://badge.fury.io/rb/natalie_parser.svg)](https://badge.fury.io/rb/natalie_parser)
[![github build status](https://github.com/natalie-lang/natalie_parser/actions/workflows/build.yml/badge.svg)](https://github.com/natalie-lang/natalie_parser/actions?query=workflow%3ABuild+branch%3Amaster)
[![MIT License](https://img.shields.io/badge/license-MIT-blue)](https://github.com/natalie-lang/natalie_parser/blob/master/LICENSE)

This is a parser for the Ruby programming language, written in C++.
It was extracted from the [Natalie](https://github.com/natalie-lang/natalie) project.

You can use this library directly from a C/C++ project, or you can
build it as a Ruby gem and use it from Ruby itself.

We are currently targeting Ruby 3.0 syntax, but that will probably
change over time, depending on what things we want to support and
what kind of help we get from the community.

NOTE: This project is still very new and there are certainly bugs.
See the list below for things we already know about, but expect there
are more we don't know about yet. **We don't recommend you use this in
production applications.**

## To Do

- [x] Parse the [Natalie](https://github.com/natalie-lang/natalie) compiler and standard library
- [x] Pass (mostly) the [RubyParser](https://github.com/seattlerb/ruby_parser) test suite
- [ ] Support different source encodings
- [ ] Support more of the Ruby 3.0 syntax
  - [x] "Endless" method definition (`def foo = bar`)
  - [x] Argument forwarding (`...`)
  - [x] Numbered block parameters (`_1`, `_2`, etc.)
  - [x] Rational and Complex literals (`1r` and `2i`)
  - [ ] Non-ASCII identifiers
  - [ ] Pattern matching

## Development

You'll need:

- gcc or clang
- ruby-dev (dev headers)
- ccache (optional)
- compiledb (optional)

```sh
rake
ruby -I lib:ext -r natalie_parser -e "p NatalieParser.parse('1 + 2')"
# => s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)))
```

### Running Tests

```sh
rake test
```

## Copyright & License

Natalie is copyright 2022, Tim Morgan and contributors. Natalie is licensed
under the MIT License; see the `LICENSE` file in this directory for the full text.

### Note about Outside Sources

The file `test/test_ruby_parser.rb` is copyright Ryan Davis and is licensed MIT.
