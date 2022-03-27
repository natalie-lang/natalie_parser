# skip-ruby

require_relative './test_helper'

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
      ].each { |keyword| expect(NatalieParser.tokens(keyword)).must_equal [{ type: keyword.to_sym }] }
      expect(NatalieParser.tokens('defx = 1')).must_equal [
        { type: :name, literal: :defx },
        { type: :'=' },
        { type: :fixnum, literal: 1 },
      ]
    end

    it 'tokenizes division and regexp' do
      expect(NatalieParser.tokens('1/2')).must_equal [{ type: :fixnum, literal: 1 }, { type: :'/' }, { type: :fixnum, literal: 2 }]
      expect(NatalieParser.tokens('1 / 2')).must_equal [{ type: :fixnum, literal: 1 }, { type: :'/' }, { type: :fixnum, literal: 2 }]
      expect(NatalieParser.tokens('1 / 2 / 3')).must_equal [
        { type: :fixnum, literal: 1 },
        { type: :'/' },
        { type: :fixnum, literal: 2 },
        { type: :'/' },
        { type: :fixnum, literal: 3 },
      ]
      expect(NatalieParser.tokens('foo / 2')).must_equal [
        { type: :name, literal: :foo },
        { type: :'/' },
        { type: :fixnum, literal: 2 },
      ]
      expect(NatalieParser.tokens('foo /2/')).must_equal [
        { type: :name, literal: :foo },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
      ]
      expect(NatalieParser.tokens('foo/2')).must_equal [{ type: :name, literal: :foo }, { type: :'/' }, { type: :fixnum, literal: 2 }]
      expect(NatalieParser.tokens('foo( /2/ )')).must_equal [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
        { type: :')' },
      ]
      expect(NatalieParser.tokens('foo 1,/2/')).must_equal [
        { type: :name, literal: :foo },
        { type: :fixnum, literal: 1 },
        { type: :',' },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
      ]
    end

    it 'tokenizes regexps' do
      expect(NatalieParser.tokens('//mix')).must_equal [{ type: :dregx }, { type: :dregxend, options: 'mix' }]
      %w[i m x o u e s n].each do |flag|
        expect(NatalieParser.tokens("/foo/#{flag}")).must_equal [
          { type: :dregx },
          { type: :string, literal: 'foo' },
          { type: :dregxend, options: flag },
        ]
      end
      expect(NatalieParser.tokens('/foo/')).must_equal [{ type: :dregx }, { type: :string, literal: 'foo' }, { type: :dregxend }]
      expect(NatalieParser.tokens('/\/\*\/\n/')).must_equal [
        { type: :dregx },
        { type: :string, literal: "/\\*/\\n" }, # eliminates unneeded \\
        { type: :dregxend },
      ]
      expect(NatalieParser.tokens('/foo #{1+1} bar/')).must_equal [
        { type: :dregx },
        { type: :string, literal: 'foo ' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :'+' },
        { type: :fixnum, literal: 1 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: ' bar' },
        { type: :dregxend },
      ]
      expect(NatalieParser.tokens('foo =~ /=$/')).must_equal [
        { type: :name, literal: :foo },
        { type: :'=~' },
        { type: :dregx },
        { type: :string, literal: '=$' },
        { type: :dregxend },
      ]
      expect(NatalieParser.tokens('/^$(.)[.]{1}.*.+.?\^\$\.\(\)\[\]\{\}\w\W\d\D\h\H\s\S\R\*\+\?/')).must_equal [
        { type: :dregx },
        { type: :string, literal: "^$(.)[.]{1}.*.+.?\\^\\$\\.\\(\\)\\[\\]\\{\\}\\w\\W\\d\\D\\h\\H\\s\\S\\R\\*\\+\\?" },
        { type: :dregxend },
      ]
      expect(NatalieParser.tokens("/\\n\\\\n/")).must_equal [
        { type: :dregx },
        { type: :string, literal: "\\n\\\\n" },
        { type: :dregxend },
      ]
      expect(NatalieParser.tokens("/\\&\\a\\b/")).must_equal [
        { type: :dregx },
        { type: :string, literal: "\\&\\a\\b" },
        { type: :dregxend },
      ]
      expect(NatalieParser.tokens("%r{a/b/c}")).must_equal [
        { type: :dregx },
        { type: :string, literal: "a/b/c" },
        { type: :dregxend },
      ]
      expect(NatalieParser.tokens("%r(a/b/c)")).must_equal [
        { type: :dregx },
        { type: :string, literal: "a/b/c" },
        { type: :dregxend },
      ]
      expect(NatalieParser.tokens("%r|a/b/c|")).must_equal [
        { type: :dregx },
        { type: :string, literal: "a/b/c" },
        { type: :dregxend },
      ]
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
      expect(NatalieParser.tokens(operators.join(' '))).must_equal operators.map { |o| { type: o.to_sym } }
    end

    it 'tokenizes bignums' do
      expect(NatalieParser.tokens('100000000000000000000 0d100000000000000000000 0xFFFFFFFFFFFFFFFF 0o7777777777777777777777 0b1111111111111111111111111111111111111111111111111111111111111111')).must_equal [
        { type: :bignum, literal: '100000000000000000000' },
        { type: :bignum, literal: '100000000000000000000' },
        { type: :bignum, literal: '0xFFFFFFFFFFFFFFFF' },
        { type: :bignum, literal: '0o7777777777777777777777' },
        { type: :bignum, literal: '0b1111111111111111111111111111111111111111111111111111111111111111' },
      ]
    end

    it 'tokenizes fixnums' do
      expect(NatalieParser.tokens('1 123 +1 -456 - 0 100_000_000 0d5 0D6 0o10 0O11 0xff 0XFF 0b110 0B111')).must_equal [
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
      expect(NatalieParser.tokens('1, (123)')).must_equal [
        { type: :fixnum, literal: 1 },
        { type: :',' },
        { type: :'(' },
        { type: :fixnum, literal: 123 },
        { type: :')' },
      ]
      expect(-> { NatalieParser.tokens('1x') }).must_raise(SyntaxError)
      expect(-> { NatalieParser.tokens('0bb') }).must_raise(SyntaxError, "1: syntax error, unexpected 'b'")
      expect(-> { NatalieParser.tokens('0dc') }).must_raise(SyntaxError, "1: syntax error, unexpected 'c'")
      expect(-> { NatalieParser.tokens('0d2d') }).must_raise(SyntaxError, "1: syntax error, unexpected 'd'")
      expect(-> { NatalieParser.tokens('0o2e') }).must_raise(SyntaxError, "1: syntax error, unexpected 'e'")
      expect(-> { NatalieParser.tokens('0x2z') }).must_raise(SyntaxError, "1: syntax error, unexpected 'z'")
    end

    it 'tokenizes floats' do
      expect(NatalieParser.tokens('1.234')).must_equal [{ type: :float, literal: 1.234 }]
      expect(NatalieParser.tokens('-1.234')).must_equal [{ type: :'-' }, { type: :float, literal: 1.234 }]
      expect(NatalieParser.tokens('0.1')).must_equal [{ type: :float, literal: 0.1 }]
      expect(NatalieParser.tokens('-0.1')).must_equal [{ type: :'-' }, { type: :float, literal: 0.1 }]
      expect(NatalieParser.tokens('123_456.00')).must_equal [{ type: :float, literal: 123456.0 }]
      expect(NatalieParser.tokens('0.95')).must_equal [{ type: :float, literal: 0.95 }]
      expect(-> { NatalieParser.tokens('0.1a') }).must_raise(SyntaxError, "1: syntax error, unexpected 'a'")
    end

    it 'tokenizes strings' do
      expect(NatalieParser.tokens('"foo"')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(NatalieParser.tokens('"this is \"quoted\""')).must_equal [
        { type: :dstr },
        { type: :string, literal: "this is \"quoted\"" },
        { type: :dstrend },
      ]
      expect(NatalieParser.tokens("'foo'")).must_equal [{ type: :string, literal: 'foo' }]
      expect(NatalieParser.tokens("'this is \\'quoted\\''")).must_equal [{ type: :string, literal: "this is 'quoted'" }]
      expect(NatalieParser.tokens('"\t\n"')).must_equal [{ type: :dstr }, { type: :string, literal: "\t\n" }, { type: :dstrend }]
      expect(NatalieParser.tokens("'other escaped chars \\\\ \\n'")).must_equal [
        { type: :string, literal: "other escaped chars \\ \\n" },
      ]
      expect(NatalieParser.tokens('%(foo)')).must_equal [{ type: :string, literal: 'foo' }]
      expect(NatalieParser.tokens('%[foo]')).must_equal [{ type: :string, literal: 'foo' }]
      expect(NatalieParser.tokens('%/foo/')).must_equal [{ type: :string, literal: 'foo' }]
      expect(NatalieParser.tokens('%|foo|')).must_equal [{ type: :string, literal: 'foo' }]
      expect(NatalieParser.tokens('%q(foo)')).must_equal [{ type: :string, literal: 'foo' }]
      expect(NatalieParser.tokens('%Q(foo)')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(NatalieParser.tokens('"#{:foo} bar #{1 + 1}"')).must_equal [
        { type: :dstr },
        { type: :string, literal: '' },
        { type: :evstr },
        { type: :symbol, literal: :foo },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: ' bar ' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :'+' },
        { type: :fixnum, literal: 1 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :dstrend },
      ]
      expect(NatalieParser.tokens(%("foo\#{''}bar"))).must_equal [
        { type: :dstr },
        { type: :string, literal: 'foo' },
        { type: :evstr },
        { type: :string, literal: '' },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: 'bar' },
        { type: :dstrend },
      ]
      expect(NatalieParser.tokens('"#{1}#{2}"')).must_equal [
        { type: :dstr },
        { type: :string, literal: '' },
        { type: :evstr },
        { type: :fixnum, literal: 1 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :evstr },
        { type: :fixnum, literal: 2 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :dstrend },
      ]
    end

    it 'parses string character escape sequences' do
      # \nnn           octal bit pattern, where nnn is 1-3 octal digits ([0-7])
      expect(NatalieParser.tokens('"\7 \77 \777"')).must_equal [{ type: :dstr }, { type: :string, literal: "\a ? \xFF" }, { type: :dstrend }]
      # \xnn           hexadecimal bit pattern, where nn is 1-2 hexadecimal digits ([0-9a-fA-F])
      expect(NatalieParser.tokens('"\x77 \xaB"')).must_equal [{ type: :dstr }, { type: :string, literal: "w \xAB" }, { type: :dstrend }]
      # \unnnn         Unicode character, where nnnn is exactly 4 hexadecimal digits ([0-9a-fA-F])
      expect(NatalieParser.tokens('"\u7777 \uabcd"')).must_equal [{ type: :dstr }, { type: :string, literal: "\u7777 \uabcd" }, { type: :dstrend }]
      # \u{nnnn ...}   Unicode character(s), where each nnnn is 1-6 hexadecimal digits ([0-9a-fA-F])
      expect(NatalieParser.tokens('"\u{0066 06f 6F}"')).must_equal [{ type: :dstr }, { type: :string, literal: "foo" }, { type: :dstrend }]
      error = expect(-> { NatalieParser.tokens('"\u{0066x}"') }).must_raise(SyntaxError)
      expect(error.message).must_equal "1: invalid Unicode escape"
    end

    # FIXME
    # it 'string interpolation weirdness' do
    #   expect(NatalieParser.tokens('"#{"foo"}"')).must_equal [
    #     { type: :dstr },
    #     { type: :string, literal: '' },
    #     { type: :evstr },
    #     { type: :string, literal: 'foo' },
    #     { type: :"\n" },
    #     { type: :evstrend },
    #     { type: :dstrend },
    #   ]
    #   NatalieParser.tokens('"#{"}"}"')
    #   NatalieParser.tokens('"#{ "#{ "#{ 1 + 1 }" }" }"')
    # end

    it 'tokenizes backticks and %x()' do
      expect(NatalieParser.tokens('`ls`')).must_equal [{ type: :dxstr }, { type: :string, literal: 'ls' }, { type: :dxstrend }]
      expect(NatalieParser.tokens('%x(ls)')).must_equal [{ type: :dxstr }, { type: :string, literal: 'ls' }, { type: :dxstrend }]
      expect(NatalieParser.tokens("%x(ls \#{path})")).must_equal [
        { type: :dxstr },
        { type: :string, literal: 'ls ' },
        { type: :evstr },
        { type: :name, literal: :path },
        { type: :"\n" },
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
        ':"foo\nbar"' => :"foo\nbar",
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
        #':!@' => :!@, # FIXME
        ":'='" => :'=',
        ':%' => :%,
        ':$0' => :$0,
        ':[]' => :[],
        ':[]=' => :[]=,
        ':+@' => :+@,
        ':-@' => :-@,
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
        ':~@' => :~@,
      }.each { |token, symbol| expect(NatalieParser.tokens(token)).must_equal [{ type: :symbol, literal: symbol }] }
    end

    it 'tokenizes arrays' do
      expect(NatalieParser.tokens("['foo']")).must_equal [{ type: :'[' }, { type: :string, literal: 'foo' }, { type: :']' }]
      expect(NatalieParser.tokens("['foo', 1]")).must_equal [
        { type: :'[' },
        { type: :string, literal: 'foo' },
        { type: :',' },
        { type: :fixnum, literal: 1 },
        { type: :']' },
      ]
      expect(NatalieParser.tokens("%w[    foo\n 1\t 2  ]")).must_equal [{ type: :'%w', literal: 'foo 1 2' }]
      expect(NatalieParser.tokens("%w|    foo\n 1\t 2  |")).must_equal [{ type: :'%w', literal: 'foo 1 2' }]
      expect(NatalieParser.tokens("%W[    foo\n 1\t 2  ]")).must_equal [{ type: :'%W', literal: 'foo 1 2' }]
      expect(NatalieParser.tokens("%W|    foo\n 1\t 2  |")).must_equal [{ type: :'%W', literal: 'foo 1 2' }]
      expect(NatalieParser.tokens("%i[    foo\n 1\t 2  ]")).must_equal [{ type: :'%i', literal: 'foo 1 2' }]
      expect(NatalieParser.tokens("%I[    foo\n 1\t 2  ]")).must_equal [{ type: :'%I', literal: 'foo 1 2' }]
    end

    it 'tokenizes hashes' do
      expect(NatalieParser.tokens("{ 'foo' => 1, bar: 2 }")).must_equal [
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

    it 'tokenizes class variables' do
      expect(NatalieParser.tokens('@@foo')).must_equal [{ type: :cvar, literal: :@@foo }]
    end

    it 'tokenizes instance variables' do
      expect(NatalieParser.tokens('@foo')).must_equal [{ type: :ivar, literal: :@foo }]
    end

    it 'tokenizes global variables' do
      expect(NatalieParser.tokens('$foo')).must_equal [{ type: :gvar, literal: :$foo }]
      expect(NatalieParser.tokens('$0')).must_equal [{ type: :gvar, literal: :$0 }]
      %i[$? $! $= $~ $@ $& $` $' $+ $/ $\\ $; $< $> $$ $* $. $: $" $_].each do |sym|
        expect(NatalieParser.tokens(sym.to_s)).must_equal [{ type: :gvar, literal: sym }]
      end
      %i[$-0 $-F $-l $-I $-K $-a $-d $-i $-p $-v $-w].each do |sym|
        expect(NatalieParser.tokens(sym.to_s)).must_equal [{ type: :gvar, literal: sym }]
      end
    end

    it 'tokenizes dots' do
      expect(NatalieParser.tokens('foo.bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
      ]
      expect(NatalieParser.tokens('foo . bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
      ]
    end

    it 'tokenizes newlines' do
      expect(NatalieParser.tokens("foo\nbar")).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
      expect(NatalieParser.tokens("foo \n bar")).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
    end

    it 'ignores newlines that are not significant' do
      expect(NatalieParser.tokens("foo(\n1\n)")).must_equal [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :fixnum, literal: 1 },
        { type: :')' },
      ]
    end

    it 'tokenizes semicolons as newlines' do
      expect(NatalieParser.tokens('foo;bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
      expect(NatalieParser.tokens('foo ; bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
    end

    it 'does not tokenize comments' do
      tokens = NatalieParser.tokens(<<-END)
        foo # comment 1
        # comment 2
        bar
      END
      expect(tokens).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :"\n" },
        { type: :name, literal: :bar },
        { type: :"\n" },
      ]
      NatalieParser.tokens('# only a comment') # does not get stuck in a loop :^)
    end

    it 'tokenizes lambdas' do
      expect(NatalieParser.tokens('-> { }')).must_equal [{ type: :'->' }, { type: :'{' }, { type: :'}' }]
      expect(NatalieParser.tokens('->(x) { }')).must_equal [
        { type: :'->' },
        { type: :'(' },
        { type: :name, literal: :x },
        { type: :')' },
        { type: :'{' },
        { type: :'}' },
      ]
    end

    it 'tokenizes blocks' do
      expect(NatalieParser.tokens("foo do |x, y|\nx\nend")).must_equal [
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
      expect(NatalieParser.tokens('foo { |x, y| x }')).must_equal [
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
      expect(NatalieParser.tokens('def foo()')).must_equal [
        { type: :def },
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :')' },
      ]
      expect(NatalieParser.tokens('def foo?')).must_equal [{ type: :def }, { type: :name, literal: :foo? }]
      expect(NatalieParser.tokens('def foo!')).must_equal [{ type: :def }, { type: :name, literal: :foo! }]
      expect(NatalieParser.tokens('def foo=')).must_equal [{ type: :def }, { type: :name, literal: :foo }, { type: :'=' }]
      expect(NatalieParser.tokens('def self.foo=')).must_equal [
        { type: :def },
        { type: :self },
        { type: :'.' },
        { type: :name, literal: :foo },
        { type: :'=' },
      ]
      expect(NatalieParser.tokens('def /')).must_equal [{ type: :def }, { type: :/ }]
      expect(NatalieParser.tokens('def %(x)')).must_equal [
        { type: :def },
        { type: :% },
        { type: :'(' },
        { type: :name, literal: :x },
        { type: :')' },
      ]
      expect(NatalieParser.tokens('def self.%(x)')).must_equal [
        { type: :def },
        { type: :self },
        { type: :'.' },
        { type: :% },
        { type: :'(' },
        { type: :name, literal: :x },
        { type: :')' },
      ]
      expect(NatalieParser.tokens('foo.bar=')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
        { type: :'=' },
      ]
      expect(NatalieParser.tokens('foo::bar!')).must_equal [
        { type: :name, literal: :foo },
        { type: :'::' },
        { type: :name, literal: :bar! },
      ]
      expect(NatalieParser.tokens('Foo::bar!')).must_equal [
        { type: :constant, literal: :Foo },
        { type: :'::' },
        { type: :name, literal: :bar! },
      ]
      expect(NatalieParser.tokens('bar=1')).must_equal [{ type: :name, literal: :bar }, { type: :'=' }, { type: :fixnum, literal: 1 }]
      expect(NatalieParser.tokens('nil?')).must_equal [{ type: :name, literal: :nil? }]
      expect(NatalieParser.tokens('foo.nil?')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :nil? },
      ]
      expect(NatalieParser.tokens('foo.%(x)')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :% },
        { type: :'(' },
        { type: :name, literal: :x },
        { type: :')' },
      ]
    end

    it 'parses unary method names' do
      expect(NatalieParser.tokens('def -@')).must_equal [{ type: :def }, { type: :name, literal: :-@ }]
      expect(NatalieParser.tokens('def +@')).must_equal [{ type: :def }, { type: :name, literal: :+@ }]
      expect(NatalieParser.tokens('foo.-@')).must_equal [{:type=>:name, :literal=>:foo}, {:type=>:"."}, {:type=>:name, :literal=>:-@}]
      expect(NatalieParser.tokens('foo.+@')).must_equal [{:type=>:name, :literal=>:foo}, {:type=>:"."}, {:type=>:name, :literal=>:+@}]
      expect(NatalieParser.tokens('foo.+@bar')).must_equal [{:type=>:name, :literal=>:foo}, {:type=>:"."}, {:type=>:name, :literal=>:+@}, {:type=>:name, :literal=>:bar}]
      expect(NatalieParser.tokens('foo.+ @bar')).must_equal [{:type=>:name, :literal=>:foo}, {:type=>:"."}, {:type=>:+}, {:type=>:ivar, :literal=>:@bar}]
    end

    it 'tokenizes heredocs' do
      doc1 = <<END
foo = <<FOO
 1
2
FOO
bar
END
      doc2 = <<END
foo(1, <<-FOO, 2)
 1
2
  FOO
bar
END
      expect(NatalieParser.tokens(doc1)).must_equal [
        { type: :name, literal: :foo },
        { type: :'=' },
        { type: :dstr },
        { type: :string, literal: " 1\n2\n" },
        { type: :dstrend },
        { type: :"\n" },
        { type: :name, literal: :bar },
        { type: :"\n" },
      ]
      expect(NatalieParser.tokens(doc2)).must_equal [
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
      expect(NatalieParser.tokens("<<'FOO BAR'\n\#{foo}\nFOO BAR")).must_equal [
        { type: :string, literal: "\#{foo}\n" },
        { type: :"\n" },
      ]
      expect(NatalieParser.tokens(%(<<"FOO BAR"\n\#{foo}\nFOO BAR))).must_equal [
        { type: :dstr},
        { type: :string, literal: "" },
        { type: :evstr },
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: "\n" },
        { type: :dstrend },
        { type: :"\n" },
      ]
      expect(NatalieParser.tokens("<<`FOO BAR`\n\#{foo}\nFOO BAR")).must_equal [
        { type: :dxstr },
        { type: :string, literal: "" },
        { type: :evstr },
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: "\n" },
        { type: :dxstrend },
        { type: :"\n" },
      ]
      expect(NatalieParser.tokens("<<~FOO\n foo\n  bar\n   \nFOO")).must_equal [
        { type: :dstr },
        { type: :string, literal: "foo\n bar\n  \n" },
        { type: :dstrend },
        { type: :"\n" }
      ]
      expect(NatalieParser.tokens("x=<<FOO\nfoo\nFOO")).must_include(type: :dstr)
      expect(NatalieParser.tokens("x*<<FOO\nfoo\nFOO")).must_include(type: :dstr)
    end

    it 'tokenizes left shift (vs heredoc)' do
      # These look kinda like the start of heredocs, but they're not!
      expect(NatalieParser.tokens('x<<y')).must_equal [
        { type: :name, literal: :x },
        { type: :<< },
        { type: :name, literal: :y },
      ]
      expect(NatalieParser.tokens('1<<y')).must_include(type: :<<)
      expect(NatalieParser.tokens('x<<_foo')).must_include(type: :<<)
      expect(NatalieParser.tokens('x<<"foo"')).must_include(type: :<<)
      expect(NatalieParser.tokens('x<<`foo`')).must_include(type: :<<)
      expect(NatalieParser.tokens("x<<'foo'")).must_include(type: :<<)
    end

    # FIXME
    # it 'stores line and column numbers with each token' do
    #   expect(NatalieParser.tokens("foo = 1 + 2 # comment\n# comment\nbar.baz", true)).must_equal [
    #     { type: :name, literal: :foo, line: 0, column: 0 },
    #     { type: :'=', line: 0, column: 4 },
    #     { type: :fixnum, literal: 1, line: 0, column: 6 },
    #     { type: :'+', line: 0, column: 8 },
    #     { type: :fixnum, literal: 2, line: 0, column: 10 },
    #     { type: :"\n", line: 0, column: 21 },
    #     { type: :"\n", line: 1, column: 9 },
    #     { type: :name, literal: :bar, line: 2, column: 0 },
    #     { type: :'.', line: 2, column: 3 },
    #     { type: :name, literal: :baz, line: 2, column: 4 },
    #   ]
    # end
  end
end
