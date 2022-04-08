# Natalie Parser

[![github build status](https://github.com/natalie-lang/natalie_parser/actions/workflows/build.yml/badge.svg)](https://github.com/natalie-lang/natalie_parser/actions?query=workflow%3ABuild+branch%3Amaster)
[![MIT License](https://img.shields.io/badge/license-MIT-blue)](https://github.com/natalie-lang/natalie_parser/blob/master/LICENSE)

This is a **very** work-in-progress parser for the Ruby programming language. It was extracted from the [Natalie](https://github.com/natalie-lang/natalie) project.

Probably best not to use this apart from Natalie for awhile. There's still a lot of work to be done... (Feel free to help!)

## Development

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
