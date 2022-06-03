# skip-ruby

require_relative './test_helper'

def tokenize(code, include_location_info = false)
  NatalieParser.tokens(code, include_location_info)
end

describe 'NatalieParser' do
  describe '#tokens' do
    it 'tokenizes keywords' do
      %w[
        __ENCODING__
        __LINE__
        __FILE__
        BEGIN
        END
        alias
        and
        begin
        break
        case
        class
        def
        defined?
        do
        else
        elsif
        end
        ensure
        false
        for
        if
        in
        module
        next
        nil
        not
        or
        redo
        rescue
        retry
        return
        self
        super
        then
        true
        undef
        unless
        until
        when
        while
        yield
      ].each { |keyword| expect(tokenize(keyword)).must_equal [{ type: keyword.to_sym }] }
      expect(tokenize('defx = 1')).must_equal [
        { type: :name, literal: :defx },
        { type: :'=' },
        { type: :fixnum, literal: 1 },
      ]
    end

    it 'tokenizes line-continuation backslash' do
      expect(tokenize("foo\\\n1")).must_equal [
        { type: :name, literal: :foo },
        { type: :fixnum, literal: 1 },
      ]
    end

    it 'tokenizes division and regexp' do
      expect(tokenize('1/2')).must_equal [{ type: :fixnum, literal: 1 }, { type: :'/' }, { type: :fixnum, literal: 2 }]
      expect(tokenize('1 / 2')).must_equal [{ type: :fixnum, literal: 1 }, { type: :'/' }, { type: :fixnum, literal: 2 }]
      expect(tokenize('1 / 2 / 3')).must_equal [
        { type: :fixnum, literal: 1 },
        { type: :'/' },
        { type: :fixnum, literal: 2 },
        { type: :'/' },
        { type: :fixnum, literal: 3 },
      ]
      expect(tokenize('foo / 2')).must_equal [
        { type: :name, literal: :foo },
        { type: :'/' },
        { type: :fixnum, literal: 2 },
      ]
      expect(tokenize('foo /2/')).must_equal [
        { type: :name, literal: :foo },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
      ]
      expect(tokenize('foo/2')).must_equal [{ type: :name, literal: :foo }, { type: :'/' }, { type: :fixnum, literal: 2 }]
      expect(tokenize('foo( /2/ )')).must_equal [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
        { type: :')' },
      ]
      expect(tokenize('foo 1,/2/')).must_equal [
        { type: :name, literal: :foo },
        { type: :fixnum, literal: 1 },
        { type: :',' },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
      ]
    end

    it 'tokenizes regexps' do
      expect(tokenize('//mix')).must_equal [{ type: :dregx }, { type: :string, literal: '' }, { type: :dregxend, options: 'mix' }]
      %w[i m x o u e s n].each do |flag|
        expect(tokenize("/foo/#{flag}")).must_equal [
          { type: :dregx },
          { type: :string, literal: 'foo' },
          { type: :dregxend, options: flag },
        ]
      end
      expect(tokenize('/foo/')).must_equal [{ type: :dregx }, { type: :string, literal: 'foo' }, { type: :dregxend }]
      expect(tokenize("# foo\n/bar/")).must_equal [{ type: :dregx }, { type: :string, literal: 'bar' }, { type: :dregxend }]
      expect(tokenize('/\/\*\/\n/')).must_equal [
        { type: :dregx },
        { type: :string, literal: "/\\*/\\n" }, # eliminates unneeded \\
        { type: :dregxend },
      ]
      expect(tokenize('/foo #{1+1} bar/')).must_equal [
        { type: :dregx },
        { type: :string, literal: 'foo ' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :'+' },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :string, literal: ' bar' },
        { type: :dregxend },
      ]
      expect(tokenize('foo =~ /=$/')).must_equal [
        { type: :name, literal: :foo },
        { type: :'=~' },
        { type: :dregx },
        { type: :string, literal: '=$' },
        { type: :dregxend },
      ]
      expect(tokenize('/^$(.)[.]{1}.*.+.?\^\$\.\(\)\[\]\{\}\w\W\d\D\h\H\s\S\R\*\+\?/')).must_equal [
        { type: :dregx },
        { type: :string, literal: "^$(.)[.]{1}.*.+.?\\^\\$\\.\\(\\)\\[\\]\\{\\}\\w\\W\\d\\D\\h\\H\\s\\S\\R\\*\\+\\?" },
        { type: :dregxend },
      ]
      expect(tokenize("/\\n\\\\n/")).must_equal [
        { type: :dregx },
        { type: :string, literal: "\\n\\\\n" },
        { type: :dregxend },
      ]
      expect(tokenize("/\\&\\a\\b/")).must_equal [
        { type: :dregx },
        { type: :string, literal: "\\&\\a\\b" },
        { type: :dregxend },
      ]
      expect(tokenize("%r/a|b|c/")).must_equal [ { type: :dregx }, { type: :string, literal: "a|b|c" }, { type: :dregxend } ]
      expect(tokenize("%r{a/b/c}")).must_equal [ { type: :dregx }, { type: :string, literal: "a/b/c" }, { type: :dregxend } ]
      expect(tokenize("%r(a/b/c)")).must_equal [ { type: :dregx }, { type: :string, literal: "a/b/c" }, { type: :dregxend } ]
      expect(tokenize("%r[a/b/c]")).must_equal [ { type: :dregx }, { type: :string, literal: "a/b/c" }, { type: :dregxend } ]
      expect(tokenize("%r<a/b/c>")).must_equal [ { type: :dregx }, { type: :string, literal: "a/b/c" }, { type: :dregxend } ]
      # TODO: support %r=foo=
      # TODO: support %r;foo;
      %w[~ ` | ! @ # $ % ^ & * , . ? : ' " - _ +].each do |sym|
        expect(tokenize("%r#{sym}a/b/c#{sym}")).must_equal [ { type: :dregx }, { type: :string, literal: "a/b/c" }, { type: :dregxend } ]
      end
    end

    it 'tokenizes operators' do
      operators = %w[
        +
        +=
        -
        -=
        *
        *=
        **
        **=
        /
        /=
        %
        %=
        =
        ==
        ===
        !=
        =~
        !~
        >
        >=
        <
        <=
        <=>
        &
        |
        ^
        ~
        <<
        >>
        &&
        &&=
        ||
        ||=
        !
        ?
        :
        ::
        ..
        ...
        &.
        |=
        &=
        <<=
        >>=
      ]
      expect(tokenize(operators.join(' '))).must_equal operators.map { |o| { type: o.to_sym } }
    end

    it 'tokenizes bignums' do
      expect(tokenize('100000000000000000000 0d100000000000000000000 0xFFFFFFFFFFFFFFFF 0o7777777777777777777777 0b1111111111111111111111111111111111111111111111111111111111111111')).must_equal [
        { type: :bignum, literal: '100000000000000000000' },
        { type: :bignum, literal: '100000000000000000000' },
        { type: :bignum, literal: '0xFFFFFFFFFFFFFFFF' },
        { type: :bignum, literal: '0o7777777777777777777777' },
        { type: :bignum, literal: '0b1111111111111111111111111111111111111111111111111111111111111111' },
      ]
      # too big for MRI fixnum, even though it fits in a long long
      expect(tokenize('18446744073709551627')).must_equal [
        { type: :bignum, literal: '18446744073709551627' }
      ]
    end

    it 'tokenizes fixnums' do
      expect(tokenize('1 123 +1 -456 - 0 100_000_000 0d5 0D6 0o10 0O11 0xff 0XFF 0b110 0B111')).must_equal [
        { type: :fixnum, literal: 1 },
        { type: :fixnum, literal: 123 },
        { type: :'+' },
        { type: :fixnum, literal: 1 },
        { type: :'-' },
        { type: :fixnum, literal: 456 },
        { type: :'-' },
        { type: :fixnum, literal: 0 },
        { type: :fixnum, literal: 100_000_000 },
        { type: :fixnum, literal: 5 }, # 0d5
        { type: :fixnum, literal: 6 }, # 0D6
        { type: :fixnum, literal: 8 }, # 0o10
        { type: :fixnum, literal: 9 }, # 0O11
        { type: :fixnum, literal: 255 }, # 0xff
        { type: :fixnum, literal: 255 }, # 0XFF
        { type: :fixnum, literal: 6 }, # 0b110
        { type: :fixnum, literal: 7 }, # 0B111
      ]
      expect(tokenize('1, (123)')).must_equal [
        { type: :fixnum, literal: 1 },
        { type: :',' },
        { type: :'(' },
        { type: :fixnum, literal: 123 },
        { type: :')' },
      ]
      expect(tokenize('1x')).must_equal [{ type: :fixnum, literal: 1 }, { type: :name, literal: :x }]
      expect(-> { tokenize('0bb') }).must_raise(SyntaxError, "1: syntax error, unexpected 'b'")
      expect(tokenize('123while')).must_equal [{ type: :fixnum, literal: 123 }, { type: :while }]
      expect(tokenize('0xefor')).must_equal [{ type: :fixnum, literal: 239 }, { type: :or }]
    end

    it 'tokenizes floats' do
      expect(tokenize('1.234')).must_equal [{ type: :float, literal: 1.234 }]
      expect(tokenize('-1.234')).must_equal [{ type: :'-' }, { type: :float, literal: 1.234 }]
      expect(tokenize('0.1')).must_equal [{ type: :float, literal: 0.1 }]
      expect(tokenize('-0.1')).must_equal [{ type: :'-' }, { type: :float, literal: 0.1 }]
      expect(tokenize('123_456.00')).must_equal [{ type: :float, literal: 123456.0 }]
      expect(tokenize('0.95')).must_equal [{ type: :float, literal: 0.95 }]
      expect(tokenize('2e5')).must_equal [{ type: :float, literal: 200000.0 }]
      expect(tokenize('2e+5')).must_equal [{ type: :float, literal: 200000.0 }]
      expect(tokenize('2.1E-5')).must_equal [{ type: :float, literal: 0.000021 }]
      expect(tokenize('1.0if')).must_equal [{ type: :float, literal: 1.0 }, { type: :if }]
      expect(-> { tokenize('0.1e') }).must_raise(SyntaxError, "1: syntax error, unexpected 'e'")
      expect(-> { tokenize('0.1e--') }).must_raise(SyntaxError, "1: syntax error, unexpected '-'")
    end

    it 'tokenizes characters' do
      expect(tokenize('?a')).must_equal [{ type: :string, literal: 'a' }]
      expect(tokenize("?a\n")).must_equal [{ type: :string, literal: 'a' }, { type: :"\n" }]
      expect(tokenize("?? ")).must_equal [{ type: :string, literal: '?' }]
      expect(tokenize("?:")).must_equal [{ type: :string, literal: ':' }]
      # \nnn       octal bit pattern, where nnn is 1-3 octal digits ([0-7])
      expect(tokenize('?\7 ?\77 ?\777')).must_equal [{ type: :string, literal: "\7" }, { type: :string, literal: "\77" }, { type: :string, literal: "\777" }]
      # \xnn       hexadecimal bit pattern, where nn is 1-2 hexadecimal digits ([0-9a-fA-F])
      expect(tokenize('?\x77 ?\xaB')).must_equal [{ type: :string, literal: "\x77" }, { type: :string, literal: "\xAB" }]
      # \unnnn     Unicode character, where nnnn is exactly 4 hexadecimal digits ([0-9a-fA-F])
      expect(tokenize('?\u7777 ?\uabcd')).must_equal [{ type: :string, literal: "\u7777" }, { type: :string, literal: "\uabcd" }]
      # \u{nnnn}   Unicode character(s), where nnnn is 1-6 hexadecimal digits ([0-9a-fA-F])
      expect(tokenize('?\u{0066}')).must_equal [{ type: :string, literal: "f" }]
    end

    it 'tokenizes strings' do
      # double quotes
      expect(tokenize('"foo"')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(tokenize('"this is \"quoted\""')).must_equal [
        { type: :dstr },
        { type: :string, literal: "this is \"quoted\"" },
        { type: :dstrend },
      ]

      # single quotes
      expect(tokenize("'foo'")).must_equal [{ type: :string, literal: 'foo' }]
      expect(tokenize("'this is \\'quoted\\''")).must_equal [{ type: :string, literal: "this is 'quoted'" }]

      # escapes
      expect(tokenize('"\t\n"')).must_equal [{ type: :dstr }, { type: :string, literal: "\t\n" }, { type: :dstrend }]
      expect(tokenize("'other escaped chars \\\\ \\n'")).must_equal [
        { type: :string, literal: "other escaped chars \\ \\n" },
      ]

      # no space before next keyword
      expect(tokenize("'foo'if")).must_equal [{ type: :string, literal: 'foo' }, { type: :if }]
      expect(tokenize('"foo"if')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }, { type: :if }]

      # interpolation
      expect(tokenize('"#{:foo} bar #{1 + 1}"')).must_equal [
        { type: :dstr },
        { type: :evstr },
        { type: :symbol, literal: :foo },
        { type: :evstrend },
        { type: :string, literal: ' bar ' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :'+' },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :dstrend },
      ]
      expect(tokenize(%("foo\#{''}bar"))).must_equal [
        { type: :dstr },
        { type: :string, literal: 'foo' },
        { type: :evstr },
        { type: :string, literal: '' },
        { type: :evstrend },
        { type: :string, literal: 'bar' },
        { type: :dstrend },
      ]
      expect(tokenize('"#{1}#{2}"')).must_equal [
        { type: :dstr },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :evstr },
        { type: :fixnum, literal: 2 },
        { type: :evstrend },
        { type: :dstrend },
      ]
      expect(tokenize('"foo #{1 + 2} bar"')).must_equal [
        { type: :dstr },
        { type: :string, literal: 'foo ' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :'+' },
        { type: :fixnum, literal: 2 },
        { type: :evstrend },
        { type: :string, literal: ' bar' },
        { type: :dstrend },
      ]
      expect(tokenize('"#{foo { 1 }}"')).must_equal [
        { type: :dstr },
        { type: :evstr },
        { type: :name, literal: :foo },
        { type: :'{' },
        { type: :fixnum, literal: 1 },
        { type: :'}' },
        { type: :evstrend },
        { type: :dstrend }
      ]
      expect(tokenize('%{ { #{ 1 } } }')).must_equal [
        { type: :dstr },
        { type: :string, literal: " { " },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :string, literal: " } " },
        { type: :dstrend },
      ]
      expect(tokenize("%{ { #\{ \"#\{1}\" } } }")).must_equal [
        { type: :dstr },
        { type: :string, literal: " { " },
        { type: :evstr },
        { type: :dstr },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :dstrend },
        { type: :evstrend },
        { type: :string, literal: " } " },
        { type: :dstrend },
      ]

      # different delimiters
      expect(tokenize('%(foo)')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(tokenize('%[foo]')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(tokenize('%{foo}')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(tokenize('%<foo>')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(tokenize('%q(foo)')).must_equal [{ type: :string, literal: 'foo' }]
      expect(tokenize('%q[foo]')).must_equal [{ type: :string, literal: 'foo' }]
      expect(tokenize('%q{foo}')).must_equal [{ type: :string, literal: 'foo' }]
      expect(tokenize('%q<foo>')).must_equal [{ type: :string, literal: 'foo' }]
      expect(tokenize('%Q(foo)')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(tokenize('%Q[foo]')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(tokenize('%Q{foo}')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(tokenize('%Q<foo>')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(tokenize('%Q[foo [#{bar}] baz]')).must_equal [
        { type: :dstr },
        { type: :string, literal: 'foo [' },
        { type: :evstr },
        { type: :name, literal: :bar },
        { type: :evstrend },
        { type: :string, literal: '] baz' },
        { type: :dstrend },
      ]
      # TODO: support %=foo=
      # TODO: support %;foo;
      %w[~ ` | / ! @ # $ % ^ & * , . ? : ' " - _ +].each do |sym|
        expect(tokenize("%#{sym}foo#{sym}")).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
        expect(tokenize("%q#{sym}foo#{sym}")).must_equal [{ type: :string, literal: 'foo' }]
        expect(tokenize("%Q#{sym}foo#{sym}")).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      end
    end

    it 'tokenizes string character escape sequences' do
      # \nnn           octal bit pattern, where nnn is 1-3 octal digits ([0-7])
      expect(tokenize('"\7 \77 \777"')).must_equal [{ type: :dstr }, { type: :string, literal: "\a ? \xFF" }, { type: :dstrend }]
      # \xnn           hexadecimal bit pattern, where nn is 1-2 hexadecimal digits ([0-9a-fA-F])
      expect(tokenize('"\x77 \xaB"')).must_equal [{ type: :dstr }, { type: :string, literal: "w \xAB" }, { type: :dstrend }]
      # \unnnn         Unicode character, where nnnn is exactly 4 hexadecimal digits ([0-9a-fA-F])
      expect(tokenize('"\u7777 \uabcd"')).must_equal [{ type: :dstr }, { type: :string, literal: "\u7777 \uabcd" }, { type: :dstrend }]
      # \u{nnnn ...}   Unicode character(s), where each nnnn is 1-6 hexadecimal digits ([0-9a-fA-F])
      expect(tokenize('"\u{0066 06f 6F}"')).must_equal [{ type: :dstr }, { type: :string, literal: "foo" }, { type: :dstrend }]
      error = expect(-> { tokenize('"\u{0066x}"') }).must_raise(SyntaxError)
      expect(error.message).must_equal "1: invalid Unicode escape"
    end

    it 'tokenizes backticks and %x()' do
      expect(tokenize('`ls`')).must_equal [{ type: :dxstr }, { type: :string, literal: 'ls' }, { type: :dxstrend }]
      expect(tokenize('%x(ls)')).must_equal [{ type: :dxstr }, { type: :string, literal: 'ls' }, { type: :dxstrend }]
      expect(tokenize("%x(ls \#{path})")).must_equal [
        { type: :dxstr },
        { type: :string, literal: 'ls ' },
        { type: :evstr },
        { type: :name, literal: :path },
        { type: :evstrend },
        { type: :dxstrend },
      ]
    end

    it 'tokenizes symbols' do
      {
        ':foo' => :foo,
        ':FooBar123' => :FooBar123,
        ':foo_bar' => :foo_bar,
        ":'foo bar'" => :'foo bar',
        ':foo?' => :foo?,
        ":'?foo'" => :'?foo',
        ':foo!' => :foo!,
        ":'!foo'" => :'!foo',
        ":'@'" => :'@',
        ':@foo' => :@foo,
        ':@@foo' => :@@foo,
        ":'foo@'" => :'foo@',
        ':$foo' => :$foo,
        ":'foo$'" => :'foo$',
        ':+' => :+,
        ':-' => :-,
        ':*' => :*,
        ':**' => :**,
        ':/' => :/,
        ':==' => :==,
        ':!=' => :!=,
        ':!' => :!,
        ':!~' => :!~,
        ":'='" => :'=',
        ':%' => :%,
        ':$0' => :$0,
        ':[]' => :[],
        ':[]=' => :[]=,
        ':+@' => :+@,
        ':-@' => :-@,
        ":!@" => :'!@',
        ":'!@'" => :'!@',
        ':~@' => :~@,
        ':===' => :===,
        ':=~' => :=~,
        ':>' => :>,
        ':>=' => :>=,
        ':>>' => :>>,
        ':<' => :<,
        ':<=' => :<=,
        ':<=>' => :<=>,
        ':<<' => :<<,
        ':&' => :&,
        ':|' => :|,
        ':^' => :^,
        ':~' => :~,
      }.each { |token, symbol| expect(tokenize(token)).must_equal [{ type: :symbol, literal: symbol }] }
      expect(tokenize(':"foo\nbar"')).must_equal [
        { type: :dsym },
        { type: :string, literal: "foo\nbar" },
        { type: :dsymend },
      ]
      expect(tokenize(':"foo#{1+1}"')).must_equal [
        { type: :dsym },
        { type: :string, literal: "foo" },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :+ },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :dsymend },
      ]
      expect(-> { tokenize(':[1') }).must_raise SyntaxError
    end

    it 'tokenizes arrays' do
      expect(tokenize("['foo']")).must_equal [{ type: :'[' }, { type: :string, literal: 'foo' }, { type: :']' }]
      expect(tokenize("['foo', 1]")).must_equal [
        { type: :'[' },
        { type: :string, literal: 'foo' },
        { type: :',' },
        { type: :fixnum, literal: 1 },
        { type: :']' },
      ]
      def expected(type)
        [
          { type: type.to_sym },
          { type: :string, literal: 'foo' },
          { type: :string, literal: '1' },
          { type: :string, literal: '2' },
          { type: :']' }
        ]
      end
      expect(tokenize("%W(    foo\n 1\t 2  )")).must_equal expected('%W[')
      expect(tokenize("%W[    foo\n 1\t 2  ]")).must_equal expected('%W[')
      expect(tokenize("%W{    foo\n 1\t 2  }")).must_equal expected('%W[')
      expect(tokenize("%W<    foo\n 1\t 2  >")).must_equal expected('%W[')
      expect(tokenize("%W/    foo\n 1\t 2  /")).must_equal expected('%W[')
      expect(tokenize("%W|    foo\n 1\t 2  |")).must_equal expected('%W[')
      expect(tokenize("%w(    foo\n 1\t 2  )")).must_equal expected('%w[')
      expect(tokenize("%w[    foo\n 1\t 2  ]")).must_equal expected('%w[')
      expect(tokenize("%w{    foo\n 1\t 2  }")).must_equal expected('%w[')
      expect(tokenize("%w<    foo\n 1\t 2  >")).must_equal expected('%w[')
      expect(tokenize("%w/    foo\n 1\t 2  /")).must_equal expected('%w[')
      expect(tokenize("%w|    foo\n 1\t 2  |")).must_equal expected('%w[')
      expect(tokenize("%i(    foo\n 1\t 2  )")).must_equal expected('%i[')
      expect(tokenize("%i[    foo\n 1\t 2  ]")).must_equal expected('%i[')
      expect(tokenize("%i{    foo\n 1\t 2  }")).must_equal expected('%i[')
      expect(tokenize("%i<    foo\n 1\t 2  >")).must_equal expected('%i[')
      expect(tokenize("%i/    foo\n 1\t 2  /")).must_equal expected('%i[')
      expect(tokenize("%i|    foo\n 1\t 2  |")).must_equal expected('%i[')
      expect(tokenize("%I(    foo\n 1\t 2  )")).must_equal expected('%I[')
      expect(tokenize("%I[    foo\n 1\t 2  ]")).must_equal expected('%I[')
      expect(tokenize("%I{    foo\n 1\t 2  }")).must_equal expected('%I[')
      expect(tokenize("%I<    foo\n 1\t 2  >")).must_equal expected('%I[')
      expect(tokenize("%I/    foo\n 1\t 2  /")).must_equal expected('%I[')
      expect(tokenize("%I|    foo\n 1\t 2  |")).must_equal expected('%I[')
      expect(tokenize('%W[1 foo#{1 + 1} 3]')).must_equal [
        { type: :'%W[' },
        { type: :string, literal: '1' },
        { type: :dstr },
        { type: :string, literal: 'foo' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :'+' },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :dstrend },
        { type: :string, literal: '3' },
        { type: :']' },
      ]
      expect(tokenize('%W[1 foo#{1 + 1}bar 3]')).must_equal [
        { type: :'%W[' },
        { type: :string, literal: '1' },
        { type: :dstr },
        { type: :string, literal: 'foo' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :'+' },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :string, literal: 'bar' },
        { type: :dstrend },
        { type: :string, literal: '3' },
        { type: :']' },
      ]
      expect(tokenize("%W[\#{1}foo]")).must_equal [
        { type: :'%W[' },
        { type: :dstr },
        { type: :string, literal: '' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :string, literal: 'foo' },
        { type: :dstrend },
        { type: :']' },
      ]
      expect(tokenize("%W[\#{1+1}]")).must_equal [
        { type: :'%W[' },
        { type: :dstr },
        { type: :string, literal: '' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :'+' },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :dstrend },
        { type: :']' },
      ]
      expect(tokenize("%w[1 \#{1 + 1} 3]")).must_equal [
        { type: :'%w[' },
        { type: :string, literal: '1' },
        { type: :string, literal: '#{1' },
        { type: :string, literal: '+' },
        { type: :string, literal: '1}' },
        { type: :string, literal: '3' },
        { type: :']' },
      ]
      expect(tokenize("%i[ [] []= ]")).must_equal [
        { type: :'%i[' },
        { type: :string, literal: '[]' },
        { type: :string, literal: '[]=' },
        { type: :']' },
      ]
      expect(tokenize("%i( () )").size).must_equal(3)
      expect(tokenize("%i{ {} }").size).must_equal(3)
      expect(tokenize("%i< <> >").size).must_equal(3)
      expect(tokenize("%w[ [] ]").size).must_equal(3)
      expect(tokenize("%w( () )").size).must_equal(3)
      expect(tokenize("%w{ {} }").size).must_equal(3)
      expect(tokenize("%w< <> >").size).must_equal(3)
      expect(tokenize("%W[ [] ]").size).must_equal(3)
      expect(tokenize("%W( () )").size).must_equal(3)
      expect(tokenize("%W{ {} }").size).must_equal(3)
      expect(tokenize("%W< <> >").size).must_equal(3)
      expect(tokenize('%i(foo#{1} bar)')).must_equal [
        { type: :'%i[' },
        { type: :string, literal: 'foo#{1}' },
        { type: :string, literal: 'bar' },
        { type: :']' },
      ]
      expect(tokenize('%I(foo#{1} bar)')).must_equal [
        { type: :'%I[' },
        { type: :dstr },
        { type: :string, literal: 'foo' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :dstrend },
        { type: :string, literal: 'bar' },
        { type: :']' },
      ]
    end

    it 'tokenizes hashes' do
      expect(tokenize("{ 'foo' => 1, bar: 2 }")).must_equal [
        { type: :'{' },
        { type: :string, literal: 'foo' },
        { type: :'=>' },
        { type: :fixnum, literal: 1 },
        { type: :',' },
        { type: :symbol_key, literal: :bar },
        { type: :fixnum, literal: 2 },
        { type: :'}' },
      ]
    end

    it 'tokenizes method call with symbol key' do
      expect(tokenize("foo a: b")).must_equal [
        { type: :name, literal: :foo },
        { type: :symbol_key, literal: :a },
        { type: :name, literal: :b },
      ]
      expect(tokenize("foo self: a")).must_equal [
        { type: :name, literal: :foo },
        { type: :symbol_key, literal: :self },
        { type: :name, literal: :a },
      ]
      expect(tokenize("foo 'a': 'b'")).must_equal [
        { type: :name, literal: :foo },
        { type: :symbol_key, literal: :a },
        { type: :string, literal: 'b' },
      ]
      expect(tokenize("foo 'a':'b'")).must_equal [
        { type: :name, literal: :foo },
        { type: :symbol_key, literal: :a },
        { type: :string, literal: 'b' },
      ]
      expect(tokenize(%q(foo "a": 'b'))).must_equal [
        { type: :name, literal: :foo },
        { type: :dstr },
        { type: :string, literal: 'a' },
        { type: :dstrend },
        { type: :dstr_symbol_key },
        { type: :string, literal: 'b' },
      ]
      expect(tokenize(%q(foo "a":'b'))).must_equal [
        { type: :name, literal: :foo },
        { type: :dstr },
        { type: :string, literal: 'a' },
        { type: :dstrend },
        { type: :dstr_symbol_key },
        { type: :string, literal: 'b' },
      ]
      expect(tokenize('foo:bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :symbol, literal: :bar },
      ]
      expect(tokenize('p foo:bar')).must_equal [
        { type: :name, literal: :p },
        { type: :symbol_key, literal: :foo },
        { type: :name, literal: :bar },
      ]
      expect(tokenize("Foo::Bar :baz")).must_equal [
        { type: :constant, literal: :Foo },
        { type: :'::' },
        { type: :constant, literal: :Bar },
        { type: :symbol, literal: :baz },
      ]
      expect(tokenize("Foo::Bar:baz")).must_equal [
        { type: :constant, literal: :Foo },
        { type: :'::' },
        { type: :constant, literal: :Bar },
        { type: :symbol, literal: :baz },
      ]
    end

    it 'tokenizes symbol keys in proper context' do
      expect(tokenize('p foo:bar')).must_include(type: :symbol_key, literal: :foo)
      expect(tokenize('p(foo:bar)')).must_include(type: :symbol_key, literal: :foo)
      expect(tokenize('p{|x|foo:bar}')).must_include(type: :symbol_key, literal: :foo)
      expect(tokenize('p{||foo:bar}')).must_include(type: :symbol_key, literal: :foo)
      expect(tokenize("{\nfoo:bar}")).must_include(type: :symbol_key, literal: :foo)
      expect(tokenize('Hash[foo:bar]')).must_include(type: :symbol_key, literal: :foo)
      expect(tokenize('Hash[foo: bar]')).must_include(type: :symbol_key, literal: :foo)
    end

    it 'tokenizes local variables' do
      expect(tokenize('moo🐄 = 1')).must_equal [{ type: :name, literal: :moo🐄 }, { type: :"=" }, { type: :fixnum, literal: 1 }]
      expect(tokenize('ootpüt = 1')).must_equal [{ type: :name, literal: :ootpüt }, { type: :"=" }, { type: :fixnum, literal: 1 }]
    end

    it 'tokenizes class variables' do
      expect(tokenize('@@foo')).must_equal [{ type: :cvar, literal: :@@foo }]
    end

    it 'tokenizes instance variables' do
      expect(tokenize('@foo')).must_equal [{ type: :ivar, literal: :@foo }]
    end

    it 'tokenizes global variables' do
      expect(tokenize('$foo')).must_equal [{ type: :gvar, literal: :$foo }]
      expect(tokenize('$0')).must_equal [{ type: :gvar, literal: :$0 }]
      %i[$? $! $= $~ $@ $` $' $+ $/ $\\ $; $< $> $$ $* $. $: $" $_ $,].each do |sym|
        expect(tokenize(sym.to_s)).must_equal [{ type: :gvar, literal: sym }]
      end
      %i[$-0 $-F $-l $-I $-K $-a $-d $-i $-p $-v $-w].each do |sym|
        expect(tokenize(sym.to_s)).must_equal [{ type: :gvar, literal: sym }]
      end
    end

    it 'tokenizes back refs and nth refs (they look like globals)' do
      expect(tokenize('$&')).must_equal [{ type: :back_ref, literal: :& }]
      expect(tokenize('$1')).must_equal [{ type: :nth_ref, literal: 1 }]
      expect(tokenize('$12')).must_equal [{ type: :nth_ref, literal: 12 }]
    end

    it 'tokenizes dots' do
      expect(tokenize('foo.bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
      ]
      expect(tokenize('foo . bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
      ]
    end

    it 'tokenizes newlines' do
      expect(tokenize("foo\nbar")).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
      expect(tokenize("foo \n bar")).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
    end

    it 'ignores newlines that are not significant' do
      expect(tokenize("foo(\n1\n)")).must_equal [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :fixnum, literal: 1 },
        { type: :')' },
      ]
    end

    it 'does not tokenize comments' do
      tokens = tokenize(<<-END)
        foo # comment 1
        # comment 2
        bar
      END
      expect(tokens).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
        { type: :"\n" },
      ]
      tokenize('# only a comment') # does not get stuck in a loop :^)
    end

    it 'tokenizes lambdas' do
      expect(tokenize('-> { }')).must_equal [{ type: :'->' }, { type: :'{' }, { type: :'}' }]
      expect(tokenize('->(x) { }')).must_equal [
        { type: :'->' },
        { type: :'(' },
        { type: :name, literal: :x },
        { type: :')' },
        { type: :'{' },
        { type: :'}' },
      ]
    end

    it 'tokenizes blocks' do
      expect(tokenize("foo do |x, y|\nx\nend")).must_equal [
        { type: :name, literal: :foo },
        { type: :do },
        { type: :'|' },
        { type: :name, literal: :x },
        { type: :',' },
        { type: :name, literal: :y },
        { type: :'|' },
        { type: :name, literal: :x },
        { type: :"\n" },
        { type: :end },
      ]
      expect(tokenize('foo { |x, y| x }')).must_equal [
        { type: :name, literal: :foo },
        { type: :'{' },
        { type: :'|' },
        { type: :name, literal: :x },
        { type: :',' },
        { type: :name, literal: :y },
        { type: :'|' },
        { type: :name, literal: :x },
        { type: :'}' },
      ]
    end

    it 'tokenizes method names' do
      expect(tokenize('def foo()')).must_equal [
        { type: :def },
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :')' },
      ]
      expect(tokenize('def foo?')).must_equal [{ type: :def }, { type: :name, literal: :foo? }]
      expect(tokenize('def foo!')).must_equal [{ type: :def }, { type: :name, literal: :foo! }]
      expect(tokenize('def foo=')).must_equal [{ type: :def }, { type: :name, literal: :foo }, { type: :'=' }]
      expect(tokenize('def self.foo=')).must_equal [
        { type: :def },
        { type: :self },
        { type: :'.' },
        { type: :name, literal: :foo },
        { type: :'=' },
      ]
      expect(tokenize('def /')).must_equal [
        { type: :def },
        { type: :/ }
      ]
      expect(tokenize('def %(x)')).must_equal [
        { type: :def },
        { type: :% },
        { type: :'(' },
        { type: :name, literal: :x },
        { type: :')' },
      ]
      expect(tokenize('def self.%(x)')).must_equal [
        { type: :def },
        { type: :self },
        { type: :'.' },
        { type: :% },
        { type: :'(' },
        { type: :name, literal: :x },
        { type: :')' },
      ]
      expect(tokenize('foo.bar=')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
        { type: :'=' },
      ]
      expect(tokenize('foo::bar!')).must_equal [
        { type: :name, literal: :foo },
        { type: :'::' },
        { type: :name, literal: :bar! },
      ]
      expect(tokenize('Foo::bar!')).must_equal [
        { type: :constant, literal: :Foo },
        { type: :'::' },
        { type: :name, literal: :bar! },
      ]
      expect(tokenize('bar=1')).must_equal [{ type: :name, literal: :bar }, { type: :'=' }, { type: :fixnum, literal: 1 }]
      expect(tokenize('nil?')).must_equal [{ type: :name, literal: :nil? }]
      expect(tokenize('foo.nil?')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :nil? },
      ]
    end

    it 'tokenizes method names using keywords' do
      expect(tokenize('def self')).must_equal [{ type: :def }, { type: :name, literal: :self }]
      expect(tokenize('def self.self')).must_equal [{ type: :def }, { type: :self }, { type: :"." }, { type: :name, literal: :self }]
      expect(tokenize('foo.self')).must_equal [{ type: :name, literal: :foo }, { type: :"." }, { type: :name, literal: :self }]
      expect(tokenize('def def')).must_equal [{ type: :def }, { type: :name, literal: :def }]
      expect(tokenize('def self.def')).must_equal [{ type: :def }, { type: :self }, { type: :"." }, { type: :name, literal: :def }]
      expect(tokenize('foo.def')).must_equal [{ type: :name, literal: :foo }, { type: :"." }, { type: :name, literal: :def }]
      expect(tokenize('def while')).must_equal [{ type: :def }, { type: :name, literal: :while }]
      expect(tokenize('def self.while')).must_equal [{ type: :def }, { type: :self }, { type: :"." }, { type: :name, literal: :while }]
      expect(tokenize('foo.while')).must_equal [{ type: :name, literal: :foo }, { type: :"." }, { type: :name, literal: :while }]
    end

    it 'tokenizes unary operator method names' do
      %w[+ - ~ !].each do |op|
        expect(tokenize("foo #{op}@bar")).must_equal [{ type: :name, literal: :foo }, { type: op.to_sym }, { type: :ivar, literal: :@bar }]
        expect(tokenize("foo #{op} @bar")).must_equal [{ type: :name, literal: :foo }, { type: op.to_sym }, { type: :ivar, literal: :@bar }]
        expect(tokenize("def #{op}@")).must_equal [{ type: :def }, { type: :name, literal: :"#{op}@" }]
        expect(tokenize("def self.#{op}@")).must_equal [{ type: :def }, { type: :self }, { type: :'.' }, { type: :name, literal: :"#{op}@" }]
        expect(tokenize("foo.#{op}@")).must_equal [{ type: :name, literal: :foo }, { type: :"." }, { type: :name, literal: :"#{op}@" }]
        expect(tokenize("foo.#{op}@bar")).must_equal [{ type: :name, literal: :foo }, { type: :"." }, { type: :name, literal: :"#{op}@" }, { type: :name, literal: :bar }]
        expect(tokenize("foo.#{op} @bar")).must_equal [{ type: :name, literal: :foo }, { type: :"." }, { type: op.to_sym }, { type: :ivar, literal: :@bar }]
      end
      expect(tokenize("foo.%(x)")).must_equal [{ type: :name, literal: :foo }, { type: :"." }, { type: :% }, { type: :"(" }, { type: :name, literal: :x }, { type: :")" }]
    end

    it 'tokenizes argument forwarding shorthand' do
      expect(tokenize("...")).must_equal [{ type: :'...' }]
    end

    it 'tokenizes ternary operator' do
      expect(tokenize("a ? '': b")).must_equal [
        { type: :name, literal: :a },
        { type: :"?" },
        { type: :string, literal: '' },
        { type: :":" },
        { type: :name, literal: :b }
      ]
      expect(tokenize('a ? "": b')).must_equal [
        { type: :name, literal: :a },
        { type: :"?" },
        { type: :dstr },
        { type: :dstrend },
        { type: :":" },
        { type: :name, literal: :b }
      ]
    end

    it 'tokenizes heredocs' do
      doc_with_interpolation = <<~END
        foo = <<FOO
        \#{0+1}
        2
        FOO
        bar
      END
      expect(tokenize(doc_with_interpolation)).must_equal [
        { type: :name, literal: :foo },
        { type: :'=' },
        { type: :dstr },
        { type: :evstr },
        { type: :fixnum, literal: 0 },
        { type: :+ },
        { type: :fixnum, literal: 1 },
        { type: :evstrend },
        { type: :string, literal: "\n2\n" },
        { type: :dstrend },
        { type: :"\n" },
        { type: :name, literal: :bar },
        { type: :"\n" },
      ]
      doc_starts_on_same_line_with_other_tokens = <<~END
        foo(1, <<-FOO, 2)
         1
        2
          FOO
        bar
      END
      expect(tokenize(doc_starts_on_same_line_with_other_tokens)).must_equal [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :fixnum, literal: 1 },
        { type: :',' },
        { type: :dstr },
        { type: :string, literal: " 1\n2\n" },
        { type: :dstrend },
        { type: :',' },
        { type: :fixnum, literal: 2 },
        { type: :')' },
        { type: :"\n" },
        { type: :name, literal: :bar },
        { type: :"\n" },
      ]
      two_docs_start_on_same_line = <<~END
        foo(<<-FOO, <<-BAR)
         1
        2
          FOO
         3
         4
        BAR
        baz <<-BAZ
        5
        BAZ
      END
      expect(tokenize(two_docs_start_on_same_line)).must_equal [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :dstr },
        { type: :string, literal: " 1\n2\n" },
        { type: :dstrend },
        { type: :',' },
        { type: :dstr },
        { type: :string, literal: " 3\n 4\n" },
        { type: :dstrend },
        { type: :')' },
        { type: :"\n" },
        { type: :name, literal: :baz },
        { type: :dstr },
        { type: :string, literal: "5\n" },
        { type: :dstrend },
        { type: :"\n" },
      ]
      doc_with_method_call = <<~END
        code = <<-BAZ.gsub(/\\n/, '; ')
        foo
        bar
        BAZ
      END
      expect(tokenize(doc_with_method_call)).must_equal [
        { type: :name, literal: :code },
        { type: :"=" },
        { type: :dstr },
        { type: :string, literal: "foo\nbar\n" },
        { type: :dstrend },
        { type: :"." },
        { type: :name, literal: :gsub },
        { type: :"(" },
        { type: :dregx },
        { type: :string, literal: "\\n" },
        { type: :dregxend },
        { type: :"," },
        { type: :string, literal: "; " },
        { type: :")" },
        { type: :"\n" }
      ]
      expect(tokenize("<<'FOO BAR'\n\#{foo}\nFOO BAR")).must_equal [
        { type: :string, literal: "\#{foo}\n" },
        { type: :"\n" },
      ]
      expect(tokenize(%(<<"FOO BAR"\n\#{foo}\nFOO BAR))).must_equal [
        { type: :dstr},
        { type: :evstr },
        { type: :name, literal: :foo },
        { type: :evstrend },
        { type: :string, literal: "\n" },
        { type: :dstrend },
        { type: :"\n" },
      ]
      expect(tokenize("<<`FOO BAR`\n\#{foo}\nFOO BAR")).must_equal [
        { type: :dxstr },
        { type: :evstr },
        { type: :name, literal: :foo },
        { type: :evstrend },
        { type: :string, literal: "\n" },
        { type: :dxstrend },
        { type: :"\n" },
      ]
      expect(tokenize("<<~FOO\n foo\n  bar\n   \nFOO")).must_equal [
        { type: :dstr },
        { type: :string, literal: "foo\n bar\n  \n" },
        { type: :dstrend },
        { type: :"\n" }
      ]
      expect(tokenize("x=<<FOO\nfoo\nFOO")).must_include(type: :dstr)
      expect(tokenize("x*<<FOO\nfoo\nFOO")).must_include(type: :dstr)
    end

    it 'tokenizes left shift (vs heredoc)' do
      # These look kinda like the start of heredocs, but they're not!
      expect(tokenize('x<<y')).must_equal [
        { type: :name, literal: :x },
        { type: :<< },
        { type: :name, literal: :y },
      ]
      expect(tokenize('1<<y')).must_include(type: :<<)
      expect(tokenize('x<<_foo')).must_include(type: :<<)
      expect(tokenize('x<<"foo"')).must_include(type: :<<)
      expect(tokenize('x<<`foo`')).must_include(type: :<<)
      expect(tokenize("x<<'foo'")).must_include(type: :<<)
    end

    it 'attaches embedded docs delimited by =begin and =end to next token' do
      expect(tokenize("=begin\nstuff\n=end\nclass Foo;end")).must_equal [
        { type: :"\n" },
        { type: :class },
        { type: :constant, literal: :Foo },
        { type: :";" },
        { type: :end },
      ]
      expect(tokenize("=begin\nstuff")).must_equal []
    end

    it 'stores line and column numbers with each token' do
      expect(tokenize("foo = 1 + 2 # comment\n# comment\nbar.baz", true)).must_equal [
        { type: :name, literal: :foo, line: 0, column: 0 },
        { type: :'=', line: 0, column: 4 },
        { type: :fixnum, literal: 1, line: 0, column: 6 },
        { type: :'+', line: 0, column: 8 },
        { type: :fixnum, literal: 2, line: 0, column: 10 },
        { type: :"\n", line: 0, column: 21 },
        { type: :name, literal: :bar, line: 2, column: 0 },
        { type: :'.', line: 2, column: 3 },
        { type: :name, literal: :baz, line: 2, column: 4 },
      ]
    end
  end
end
