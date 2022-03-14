# skip-ruby

require_relative './test_helper'

describe 'Parser' do
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
      ].each { |keyword| expect(Parser.tokens(keyword)).must_equal [{ type: keyword.to_sym }] }
      expect(Parser.tokens('defx = 1')).must_equal [
        { type: :name, literal: :defx },
        { type: :'=' },
        { type: :integer, literal: 1 },
      ]
    end

    it 'tokenizes division and regexp' do
      expect(Parser.tokens('1/2')).must_equal [{ type: :integer, literal: 1 }, { type: :'/' }, { type: :integer, literal: 2 }]
      expect(Parser.tokens('1 / 2')).must_equal [{ type: :integer, literal: 1 }, { type: :'/' }, { type: :integer, literal: 2 }]
      expect(Parser.tokens('1 / 2 / 3')).must_equal [
        { type: :integer, literal: 1 },
        { type: :'/' },
        { type: :integer, literal: 2 },
        { type: :'/' },
        { type: :integer, literal: 3 },
      ]
      expect(Parser.tokens('foo / 2')).must_equal [
        { type: :name, literal: :foo },
        { type: :'/' },
        { type: :integer, literal: 2 },
      ]
      expect(Parser.tokens('foo /2/')).must_equal [
        { type: :name, literal: :foo },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
      ]
      expect(Parser.tokens('foo/2')).must_equal [{ type: :name, literal: :foo }, { type: :'/' }, { type: :integer, literal: 2 }]
      expect(Parser.tokens('foo( /2/ )')).must_equal [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
        { type: :')' },
      ]
      expect(Parser.tokens('foo 1,/2/')).must_equal [
        { type: :name, literal: :foo },
        { type: :integer, literal: 1 },
        { type: :',' },
        { type: :dregx },
        { type: :string, literal: '2' },
        { type: :dregxend },
      ]
    end

    it 'tokenizes regexps' do
      expect(Parser.tokens('//mix')).must_equal [{ type: :dregx }, { type: :dregxend, options: 'mix' }]
      expect(Parser.tokens('/foo/i')).must_equal [
        { type: :dregx },
        { type: :string, literal: 'foo' },
        { type: :dregxend, options: 'i' },
      ]
      expect(Parser.tokens('/foo/')).must_equal [{ type: :dregx }, { type: :string, literal: 'foo' }, { type: :dregxend }]
      expect(Parser.tokens('/\/\*\/\n/')).must_equal [
        { type: :dregx },
        { type: :string, literal: "/\\*/\\n" }, # eliminates unneeded \\
        { type: :dregxend },
      ]
      expect(Parser.tokens('/foo #{1+1} bar/')).must_equal [
        { type: :dregx },
        { type: :string, literal: 'foo ' },
        { type: :evstr },
        { type: :integer, literal: 1 },
        { type: :'+' },
        { type: :integer, literal: 1 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: ' bar' },
        { type: :dregxend },
      ]
      expect(Parser.tokens('foo =~ /=$/')).must_equal [
        { type: :name, literal: :foo },
        { type: :'=~' },
        { type: :dregx },
        { type: :string, literal: '=$' },
        { type: :dregxend },
      ]
      expect(Parser.tokens('/^$(.)[.]{1}.*.+.?\^\$\.\(\)\[\]\{\}\w\W\d\D\h\H\s\S\R\*\+\?/')).must_equal [
        { type: :dregx },
        { type: :string, literal: "^$(.)[.]{1}.*.+.?\\^\\$\\.\\(\\)\\[\\]\\{\\}\\w\\W\\d\\D\\h\\H\\s\\S\\R\\*\\+\\?" },
        { type: :dregxend },
      ]
      expect(Parser.tokens("/\\n\\\\n/")).must_equal [
        { type: :dregx },
        { type: :string, literal: "\\n\\\\n" },
        { type: :dregxend },
      ]
      expect(Parser.tokens("/\\&\\a\\b/")).must_equal [
        { type: :dregx },
        { type: :string, literal: "\\&\\a\\b" },
        { type: :dregxend },
      ]
      expect(Parser.tokens("%r{a/b/c}")).must_equal [
        { type: :dregx },
        { type: :string, literal: "a/b/c" },
        { type: :dregxend },
      ]
      expect(Parser.tokens("%r(a/b/c)")).must_equal [
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
      expect(Parser.tokens(operators.join(' '))).must_equal operators.map { |o| { type: o.to_sym } }
    end

    it 'tokenizes numbers' do
      expect(Parser.tokens('1 123 +1 -456 - 0 100_000_000 0d5 0D6 0o10 0O11 0xff 0XFF 0b110 0B111')).must_equal [
        { type: :integer, literal: 1 },
        { type: :integer, literal: 123 },
        { type: :'+' },
        { type: :integer, literal: 1 },
        { type: :'-' },
        { type: :integer, literal: 456 },
        { type: :'-' },
        { type: :integer, literal: 0 },
        { type: :integer, literal: 100_000_000 },
        { type: :integer, literal: 5 }, # 0d5
        { type: :integer, literal: 6 }, # 0D6
        { type: :integer, literal: 8 }, # 0o10
        { type: :integer, literal: 9 }, # 0O11
        { type: :integer, literal: 255 }, # 0xff
        { type: :integer, literal: 255 }, # 0XFF
        { type: :integer, literal: 6 }, # 0b110
        { type: :integer, literal: 7 }, # 0B111
      ]
      expect(Parser.tokens('1, (123)')).must_equal [
        { type: :integer, literal: 1 },
        { type: :',' },
        { type: :'(' },
        { type: :integer, literal: 123 },
        { type: :')' },
      ]
      expect(-> { Parser.tokens('1x') }).must_raise(SyntaxError)
      expect(Parser.tokens('1.234')).must_equal [{ type: :float, literal: 1.234 }]
      expect(Parser.tokens('-1.234')).must_equal [{ type: :'-' }, { type: :float, literal: 1.234 }]
      expect(Parser.tokens('0.1')).must_equal [{ type: :float, literal: 0.1 }]
      expect(Parser.tokens('-0.1')).must_equal [{ type: :'-' }, { type: :float, literal: 0.1 }]
      expect(Parser.tokens('123_456.00')).must_equal [{ type: :float, literal: 123456.0 }]
      expect(-> { Parser.tokens('0.1a') }).must_raise(SyntaxError, "1: syntax error, unexpected 'a'")
      expect(-> { Parser.tokens('0bb') }).must_raise(SyntaxError, "1: syntax error, unexpected 'b'")
      expect(-> { Parser.tokens('0dc') }).must_raise(SyntaxError, "1: syntax error, unexpected 'c'")
      expect(-> { Parser.tokens('0d2d') }).must_raise(SyntaxError, "1: syntax error, unexpected 'd'")
      expect(-> { Parser.tokens('0o2e') }).must_raise(SyntaxError, "1: syntax error, unexpected 'e'")
      expect(-> { Parser.tokens('0x2z') }).must_raise(SyntaxError, "1: syntax error, unexpected 'z'")
    end

    it 'tokenizes strings' do
      expect(Parser.tokens('"foo"')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(Parser.tokens('"this is \"quoted\""')).must_equal [
        { type: :dstr },
        { type: :string, literal: "this is \"quoted\"" },
        { type: :dstrend },
      ]
      expect(Parser.tokens("'foo'")).must_equal [{ type: :string, literal: 'foo' }]
      expect(Parser.tokens("'this is \\'quoted\\''")).must_equal [{ type: :string, literal: "this is 'quoted'" }]
      expect(Parser.tokens('"\t\n"')).must_equal [{ type: :dstr }, { type: :string, literal: "\t\n" }, { type: :dstrend }]
      expect(Parser.tokens("'other escaped chars \\\\ \\n'")).must_equal [
        { type: :string, literal: "other escaped chars \\ \\n" },
      ]
      expect(Parser.tokens('%(foo)')).must_equal [{ type: :string, literal: 'foo' }]
      expect(Parser.tokens('%[foo]')).must_equal [{ type: :string, literal: 'foo' }]
      expect(Parser.tokens('%/foo/')).must_equal [{ type: :string, literal: 'foo' }]
      expect(Parser.tokens('%|foo|')).must_equal [{ type: :string, literal: 'foo' }]
      expect(Parser.tokens('%q(foo)')).must_equal [{ type: :string, literal: 'foo' }]
      expect(Parser.tokens('%Q(foo)')).must_equal [{ type: :dstr }, { type: :string, literal: 'foo' }, { type: :dstrend }]
      expect(Parser.tokens('"#{:foo} bar #{1 + 1}"')).must_equal [
        { type: :dstr },
        { type: :string, literal: '' },
        { type: :evstr },
        { type: :symbol, literal: :foo },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: ' bar ' },
        { type: :evstr },
        { type: :integer, literal: 1 },
        { type: :'+' },
        { type: :integer, literal: 1 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :dstrend },
      ]
      expect(Parser.tokens(%("foo\#{''}bar"))).must_equal [
        { type: :dstr },
        { type: :string, literal: 'foo' },
        { type: :evstr },
        { type: :string, literal: '' },
        { type: :"\n" },
        { type: :evstrend },
        { type: :string, literal: 'bar' },
        { type: :dstrend },
      ]
      expect(Parser.tokens('"#{1}#{2}"')).must_equal [
        { type: :dstr },
        { type: :string, literal: '' },
        { type: :evstr },
        { type: :integer, literal: 1 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :evstr },
        { type: :integer, literal: 2 },
        { type: :"\n" },
        { type: :evstrend },
        { type: :dstrend },
      ]
    end

    # FIXME
    # it 'string interpolation weirdness' do
    #   expect(Parser.tokens('"#{"foo"}"')).must_equal [
    #     { type: :dstr },
    #     { type: :string, literal: '' },
    #     { type: :evstr },
    #     { type: :string, literal: 'foo' },
    #     { type: :"\n" },
    #     { type: :evstrend },
    #     { type: :dstrend },
    #   ]
    #   Parser.tokens('"#{"}"}"')
    #   Parser.tokens('"#{ "#{ "#{ 1 + 1 }" }" }"')
    # end

    it 'tokenizes backticks and %x()' do
      expect(Parser.tokens('`ls`')).must_equal [{ type: :dxstr }, { type: :string, literal: 'ls' }, { type: :dxstrend }]
      expect(Parser.tokens('%x(ls)')).must_equal [{ type: :dxstr }, { type: :string, literal: 'ls' }, { type: :dxstrend }]
      expect(Parser.tokens("%x(ls \#{path})")).must_equal [
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
      }.each { |token, symbol| expect(Parser.tokens(token)).must_equal [{ type: :symbol, literal: symbol }] }
    end

    it 'tokenizes arrays' do
      expect(Parser.tokens("['foo']")).must_equal [{ type: :'[' }, { type: :string, literal: 'foo' }, { type: :']' }]
      expect(Parser.tokens("['foo', 1]")).must_equal [
        { type: :'[' },
        { type: :string, literal: 'foo' },
        { type: :',' },
        { type: :integer, literal: 1 },
        { type: :']' },
      ]
      expect(Parser.tokens("%w[    foo\n 1\t 2  ]")).must_equal [{ type: :'%w', literal: 'foo 1 2' }]
      expect(Parser.tokens("%w|    foo\n 1\t 2  |")).must_equal [{ type: :'%w', literal: 'foo 1 2' }]
      expect(Parser.tokens("%W[    foo\n 1\t 2  ]")).must_equal [{ type: :'%W', literal: 'foo 1 2' }]
      expect(Parser.tokens("%W|    foo\n 1\t 2  |")).must_equal [{ type: :'%W', literal: 'foo 1 2' }]
      expect(Parser.tokens("%i[    foo\n 1\t 2  ]")).must_equal [{ type: :'%i', literal: 'foo 1 2' }]
      expect(Parser.tokens("%I[    foo\n 1\t 2  ]")).must_equal [{ type: :'%I', literal: 'foo 1 2' }]
    end

    it 'tokenizes hashes' do
      expect(Parser.tokens("{ 'foo' => 1, bar: 2 }")).must_equal [
        { type: :'{' },
        { type: :string, literal: 'foo' },
        { type: :'=>' },
        { type: :integer, literal: 1 },
        { type: :',' },
        { type: :symbol_key, literal: :bar },
        { type: :integer, literal: 2 },
        { type: :'}' },
      ]
    end

    it 'tokenizes class variables' do
      expect(Parser.tokens('@@foo')).must_equal [{ type: :cvar, literal: :@@foo }]
    end

    it 'tokenizes instance variables' do
      expect(Parser.tokens('@foo')).must_equal [{ type: :ivar, literal: :@foo }]
    end

    it 'tokenizes global variables' do
      expect(Parser.tokens('$foo')).must_equal [{ type: :gvar, literal: :$foo }]
      expect(Parser.tokens('$0')).must_equal [{ type: :gvar, literal: :$0 }]
      expect(Parser.tokens('$?')).must_equal [{ type: :gvar, literal: :$? }]
    end

    it 'tokenizes dots' do
      expect(Parser.tokens('foo.bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
      ]
      expect(Parser.tokens('foo . bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
      ]
    end

    it 'tokenizes newlines' do
      expect(Parser.tokens("foo\nbar")).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
      expect(Parser.tokens("foo \n bar")).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
    end

    it 'ignores newlines that are not significant' do
      expect(Parser.tokens("foo(\n1\n)")).must_equal [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :integer, literal: 1 },
        { type: :')' },
      ]
    end

    it 'tokenizes semicolons as newlines' do
      expect(Parser.tokens('foo;bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
      expect(Parser.tokens('foo ; bar')).must_equal [
        { type: :name, literal: :foo },
        { type: :"\n" },
        { type: :name, literal: :bar },
      ]
    end

    it 'does not tokenize comments' do
      tokens = Parser.tokens(<<-END)
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
      Parser.tokens('# only a comment') # does not get stuck in a loop :^)
    end

    it 'tokenizes lambdas' do
      expect(Parser.tokens('-> { }')).must_equal [{ type: :'->' }, { type: :'{' }, { type: :'}' }]
      expect(Parser.tokens('->(x) { }')).must_equal [
        { type: :'->' },
        { type: :'(' },
        { type: :name, literal: :x },
        { type: :')' },
        { type: :'{' },
        { type: :'}' },
      ]
    end

    it 'tokenizes blocks' do
      expect(Parser.tokens("foo do |x, y|\nx\nend")).must_equal [
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
      expect(Parser.tokens('foo { |x, y| x }')).must_equal [
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
      expect(Parser.tokens('def foo()')).must_equal [
        { type: :def },
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :')' },
      ]
      expect(Parser.tokens('def foo?')).must_equal [{ type: :def }, { type: :name, literal: :foo? }]
      expect(Parser.tokens('def foo!')).must_equal [{ type: :def }, { type: :name, literal: :foo! }]
      expect(Parser.tokens('def foo=')).must_equal [{ type: :def }, { type: :name, literal: :foo }, { type: :'=' }]
      expect(Parser.tokens('def self.foo=')).must_equal [
        { type: :def },
        { type: :self },
        { type: :'.' },
        { type: :name, literal: :foo },
        { type: :'=' },
      ]
      expect(Parser.tokens('def /')).must_equal [{ type: :def }, { type: :/ }]
      expect(Parser.tokens('foo.bar=')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :bar },
        { type: :'=' },
      ]
      expect(Parser.tokens('foo::bar!')).must_equal [
        { type: :name, literal: :foo },
        { type: :'::' },
        { type: :name, literal: :bar! },
      ]
      expect(Parser.tokens('Foo::bar!')).must_equal [
        { type: :constant, literal: :Foo },
        { type: :'::' },
        { type: :name, literal: :bar! },
      ]
      expect(Parser.tokens('bar=1')).must_equal [{ type: :name, literal: :bar }, { type: :'=' }, { type: :integer, literal: 1 }]
      expect(Parser.tokens('nil?')).must_equal [{ type: :name, literal: :nil? }]
      expect(Parser.tokens('foo.nil?')).must_equal [
        { type: :name, literal: :foo },
        { type: :'.' },
        { type: :name, literal: :nil? },
      ]
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
      expect(Parser.tokens(doc1)).must_equal [
        { type: :name, literal: :foo },
        { type: :'=' },
        { type: :dstr },
        { type: :string, literal: " 1\n2\n" },
        { type: :dstrend },
        { type: :"\n" },
        { type: :name, literal: :bar },
        { type: :"\n" },
      ]
      expect(Parser.tokens(doc2)).must_equal [
        { type: :name, literal: :foo },
        { type: :'(' },
        { type: :integer, literal: 1 },
        { type: :',' },
        { type: :dstr },
        { type: :string, literal: " 1\n2\n" },
        { type: :dstrend },
        { type: :',' },
        { type: :integer, literal: 2 },
        { type: :')' },
        { type: :"\n" },
        { type: :name, literal: :bar },
        { type: :"\n" },
      ]
      expect(Parser.tokens("<<'FOO BAR'\n\#{foo}\nFOO BAR")).must_equal [
        { type: :string, literal: "\#{foo}\n"},
        { type: :"\n"},
      ]
      expect(Parser.tokens(%(<<"FOO BAR"\n\#{foo}\nFOO BAR))).must_equal [
        { type: :dstr},
        { type: :string, literal: ""},
        { type: :evstr},
        { type: :name, literal: :foo},
        { type: :"\n"},
        { type: :evstrend},
        { type: :string, literal: "\n"},
        { type: :dstrend},
        { type: :"\n"},
      ]
      expect(Parser.tokens("<<`FOO BAR`\n\#{foo}\nFOO BAR")).must_equal [
        { type: :dxstr},
        { type: :string, literal: ""},
        { type: :evstr},
        { type: :name, literal: :foo},
        { type: :"\n"},
        { type: :evstrend},
        { type: :string, literal: "\n"},
        { type: :dxstrend},
        { type: :"\n"},
      ]
      expect(Parser.tokens("<<~FOO\n foo\n  bar\n   \nFOO")).must_equal [
        { type: :dstr},
        { type: :string, literal: "foo\n bar\n  \n"},
        { type: :dstrend},
        { type: :"\n"}
      ]
    end

    # FIXME
    # it 'stores line and column numbers with each token' do
    #   expect(Parser.tokens("foo = 1 + 2 # comment\n# comment\nbar.baz", true)).must_equal [
    #     { type: :name, literal: :foo, line: 0, column: 0 },
    #     { type: :'=', line: 0, column: 4 },
    #     { type: :integer, literal: 1, line: 0, column: 6 },
    #     { type: :'+', line: 0, column: 8 },
    #     { type: :integer, literal: 2, line: 0, column: 10 },
    #     { type: :"\n", line: 0, column: 21 },
    #     { type: :"\n", line: 1, column: 9 },
    #     { type: :name, literal: :bar, line: 2, column: 0 },
    #     { type: :'.', line: 2, column: 3 },
    #     { type: :name, literal: :baz, line: 2, column: 4 },
    #   ]
    # end
  end
end
