require_relative './test_helper'
require_relative './support/expectations'
require_relative '../lib/natalie_parser/sexp'

%w[RubyParser NatalieParser].each do |parser|
  describe parser do
    if parser == 'NatalieParser'
      def parse(code, path = '(string)')
        NatalieParser.parse(code, path)
      end
      def bare_hash_type
        :bare_hash
      end
    else
      require 'ruby_parser'
      def parse(code, path = '(string)')
        RubyParser.new.parse(code, path)
      rescue Racc::ParseError, RubyParser::SyntaxError => e
        raise SyntaxError, e.message
      end
      def bare_hash_type
        :hash # RubyParser doesn't differentiate!
      end
    end

    def expect_raise_with_message(callable, error, message)
      actual_error = expect(callable).must_raise(error)
      if message.is_a?(Regexp)
        expect(actual_error.message).must_match(message)
      elsif message.include?("\n")
        expect(actual_error.message).must_equal(message)
      else
        expect(actual_error.message.split(/\n/).first).must_equal(message)
      end
    end

    describe '#parse' do
      it 'parses an empty program' do
        if parser == 'NatalieParser'
          expect(parse('')).must_equal s(:block)
        else
          expect(parse('')).must_be_nil
        end
      end

      it 'parses an empty expression' do
        expect(parse('()')).must_equal s(:nil)
      end

      it 'parses a group' do
        expect(parse('x = (1 + 1; 3)')).must_equal s(:lasgn, :x, s(:block, s(:call, s(:lit, 1), :+, s(:lit, 1)), s(:lit, 3)))
        expect(parse("x = (1 + 1\n3)")).must_equal s(:lasgn, :x, s(:block, s(:call, s(:lit, 1), :+, s(:lit, 1)), s(:lit, 3)))
        expect(parse("foo (bar do\n 1\n end).new")).must_equal s(:call, nil, :foo, s(:call, s(:iter, s(:call, nil, :bar), 0, s(:lit, 1)), :new))
      end

      it 'parses line-continuation backslash' do
        expect(parse("foo\\\n1")).must_equal s(:call, nil, :foo, s(:lit, 1))
        expect(-> { parse("foo\\     \n1") }).must_raise SyntaxError
      end

      it 'parses bignums' do
        expect(parse('100000000000000000000')).must_equal s(:lit, 100000000000000000000)
        expect(parse('-100000000000000000000')).must_equal s(:lit, -100000000000000000000)
      end

      it 'parses fixnums' do
        expect(parse('1')).must_equal s(:lit, 1)
        expect(parse(' 1234')).must_equal s(:lit, 1234)
        expect(parse('9223372036854775808')).must_equal s(:lit, 9223372036854775808)
        expect(parse('-9223372036854775808')).must_equal s(:lit, -9223372036854775808)
      end

      it 'parses floats' do
        expect(parse('1.5 ')).must_equal s(:lit, 1.5)
        expect(parse('-1')).must_equal s(:lit, -1)
        expect(parse('-1.5')).must_equal s(:lit, -1.5)
      end

      it 'parses complex and rational numbers' do
        expect(parse('3i')).must_equal s(:lit, Complex(0, 3))
        expect(parse('3r')).must_equal_and_be_same_class s(:lit, Rational(3, 1))
        expect(parse('3ri')).must_equal s(:lit, Complex(0, Rational(3, 1)))
        expect(parse('0d3i')).must_equal s(:lit, Complex(0, 3))
        expect(parse('0d3r')).must_equal_and_be_same_class s(:lit, Rational(3, 1))
        expect(parse('0d3ri')).must_equal s(:lit, Complex(0, Rational(3, 1)))
        expect(parse('0o3i')).must_equal s(:lit, Complex(0, 3))
        expect(parse('0o3r')).must_equal_and_be_same_class s(:lit, Rational(3, 1))
        expect(parse('0o3ri')).must_equal s(:lit, Complex(0, Rational(3, 1)))
        expect(parse('0x3i')).must_equal s(:lit, Complex(0, 3))
        expect(parse('0x3r')).must_equal_and_be_same_class s(:lit, Rational(3, 1))
        expect(parse('0x3ri')).must_equal s(:lit, Complex(0, Rational(3, 1)))
        expect(parse('0b11i')).must_equal s(:lit, Complex(0, 3))
        expect(parse('0b11r')).must_equal_and_be_same_class s(:lit, Rational(3, 1))
        expect(parse('0b11ri')).must_equal s(:lit, Complex(0, Rational(3, 1)))
        expect(parse('1.1i')).must_equal s(:lit, Complex(0, 1.1))
        expect(parse('1.1r')).must_equal_and_be_same_class s(:lit, Rational(11, 10))
        expect(parse('1.1ri')).must_equal s(:lit, Complex(0, Rational(11, 10)))
        expect(parse('100000000000000000000i')).must_equal s(:lit, Complex(0, 100000000000000000000))
        expect(parse('100000000000000000000r')).must_equal_and_be_same_class s(:lit, Rational(100000000000000000000, 1))
        expect(parse('100000000000000000000ri')).must_equal s(:lit, Complex(0, Rational(100000000000000000000, 1)))
      end

      it 'parses unary operators' do
        expect(parse('-foo.odd?')).must_equal s(:call, s(:call, s(:call, nil, :foo), :odd?), :-@)
        expect(parse('-2.odd?')).must_equal s(:call, s(:lit, -2), :odd?)
        expect(parse('+2.even?')).must_equal s(:call, s(:lit, 2), :even?)
        expect(parse('-(2*8)')).must_equal s(:call, s(:call, s(:lit, 2), :*, s(:lit, 8)), :-@)
        expect(parse('+(2*8)')).must_equal s(:call, s(:call, s(:lit, 2), :*, s(:lit, 8)), :+@)
        expect(parse('-2*8')).must_equal s(:call, s(:lit, -2), :*, s(:lit, 8))
        expect(parse('+2*8')).must_equal s(:call, s(:lit, 2), :*, s(:lit, 8))
        expect(parse('+1**2')).must_equal s(:call, s(:lit, 1), :**, s(:lit, 2))
        expect(parse('-1**2')).must_equal s(:call, s(:call, s(:lit, 1), :**, s(:lit, 2)), :-@)
        expect(parse('~1')).must_equal s(:call, s(:lit, 1), :~)
        expect(parse('~foo')).must_equal s(:call, s(:call, nil, :foo), :~)
        expect(parse('foo ~2')).must_equal s(:call, nil, :foo, s(:call, s(:lit, 2), :~))
        expect(parse('2 + ~2')).must_equal s(:call, s(:lit, 2), :+, s(:call, s(:lit, 2), :~))
        expect(parse('foo -123')).must_equal s(:call, nil, :foo, s(:lit, -123))
        expect(parse('foo-123')).must_equal s(:call, s(:call, nil, :foo), :-, s(:lit, 123))
        expect(parse('foo +123')).must_equal s(:call, nil, :foo, s(:lit, 123))
        expect(parse('foo --123')).must_equal s(:call, nil, :foo, s(:call, s(:lit, -123), :-@))
        expect(parse('1 --123')).must_equal s(:call, s(:lit, 1), :-, s(:lit, -123))
        expect(parse('--123')).must_equal s(:call, s(:lit, -123), :-@)
        expect(parse('--9223372036854775808')).must_equal s(:call, s(:lit, -9223372036854775808), :-@)
      end

      it 'parses operator expressions' do
        expect(parse('1 + 3')).must_equal s(:call, s(:lit, 1), :+, s(:lit, 3))
        expect(parse('1+3')).must_equal s(:call, s(:lit, 1), :+, s(:lit, 3))
        expect(parse("1+\n 3")).must_equal s(:call, s(:lit, 1), :+, s(:lit, 3))
        expect(parse('1 - 3')).must_equal s(:call, s(:lit, 1), :-, s(:lit, 3))
        expect(parse('1 * 3')).must_equal s(:call, s(:lit, 1), :*, s(:lit, 3))
        expect(parse('1 / 3')).must_equal s(:call, s(:lit, 1), :/, s(:lit, 3))
        expect(parse('1 * 2 + 3')).must_equal s(:call, s(:call, s(:lit, 1), :*, s(:lit, 2)), :+, s(:lit, 3))
        expect(parse('1 / 2 - 3')).must_equal s(:call, s(:call, s(:lit, 1), :/, s(:lit, 2)), :-, s(:lit, 3))
        expect(parse('1 + 2 * 3')).must_equal s(:call, s(:lit, 1), :+, s(:call, s(:lit, 2), :*, s(:lit, 3)))
        expect(parse('1 - 2 / 3')).must_equal s(:call, s(:lit, 1), :-, s(:call, s(:lit, 2), :/, s(:lit, 3)))
        expect(parse('(1 + 2) * 3')).must_equal s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:lit, 3))
        expect(parse("(\n1 + 2\n) * 3")).must_equal s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:lit, 3))
        expect(parse('(1 - 2) / 3')).must_equal s(:call, s(:call, s(:lit, 1), :-, s(:lit, 2)), :/, s(:lit, 3))
        expect(parse('(1 + 2) * (3 + 4)')).must_equal s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:call, s(:lit, 3), :+, s(:lit, 4)))
        expect(parse('"foo" + "bar"')).must_equal s(:call, s(:str, 'foo'), :+, s(:str, 'bar'))
        expect(parse('1 == 1')).must_equal s(:call, s(:lit, 1), :==, s(:lit, 1))
        expect(parse('a != b')).must_equal s(:call, s(:call, nil, :a), :!=, s(:call, nil, :b))
        expect(parse('Foo === bar')).must_equal s(:call, s(:const, :Foo), :===, s(:call, nil, :bar))
        expect(parse('1 < 2')).must_equal s(:call, s(:lit, 1), :<, s(:lit, 2))
        expect(parse('1 <= 2')).must_equal s(:call, s(:lit, 1), :<=, s(:lit, 2))
        expect(parse('1 > 2')).must_equal s(:call, s(:lit, 1), :>, s(:lit, 2))
        expect(parse('1 >= 2')).must_equal s(:call, s(:lit, 1), :>=, s(:lit, 2))
        expect(parse('5-3')).must_equal s(:call, s(:lit, 5), :-, s(:lit, 3))
        expect(parse('5 -3')).must_equal s(:call, s(:lit, 5), :-, s(:lit, 3))
        expect(parse('1 +1')).must_equal s(:call, s(:lit, 1), :+, s(:lit, 1))
        expect(parse('(1+2)-3 == 0')).must_equal s(:call, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :-, s(:lit, 3)), :==, s(:lit, 0))
        expect(parse('-(1<<33)-1')).must_equal s(:call, s(:call, s(:call, s(:lit, 1), :<<, s(:lit, 33)), :-@), :-, s(:lit, 1))
        expect(parse('((-(1<<33)-1) & 5)')).must_equal s(:call, s(:call, s(:call, s(:call, s(:lit, 1), :<<, s(:lit, 33)), :-@), :-, s(:lit, 1)), :&, s(:lit, 5))
        expect(parse('1 + 2 == 3 * 4')).must_equal s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :==, s(:call, s(:lit, 3), :*, s(:lit, 4)))
        expect(parse('2 ** 10')).must_equal s(:call, s(:lit, 2), :**, s(:lit, 10))
        expect(parse('1 * 2 ** 10 + 3')).must_equal s(:call, s(:call, s(:lit, 1), :*, s(:call, s(:lit, 2), :**, s(:lit, 10))), :+, s(:lit, 3))
        expect(parse('1 & 2 | 3 ^ 4')).must_equal s(:call, s(:call, s(:call, s(:lit, 1), :&, s(:lit, 2)), :|, s(:lit, 3)), :^, s(:lit, 4))
        expect(parse('10 % 3')).must_equal s(:call, s(:lit, 10), :%, s(:lit, 3))
        expect(parse('x << 1')).must_equal s(:call, s(:call, nil, :x), :<<, s(:lit, 1))
        expect(parse('x<<1')).must_equal s(:call, s(:call, nil, :x), :<<, s(:lit, 1))
        expect(parse('x<<y')).must_equal s(:call, s(:call, nil, :x), :<<, s(:call, nil, :y))
        expect(parse('x =~ y')).must_equal s(:call, s(:call, nil, :x), :=~, s(:call, nil, :y))
        expect(parse('x =~ /foo/')).must_equal s(:match3, s(:lit, /foo/), s(:call, nil, :x))
        expect(parse('/foo/ =~ x')).must_equal s(:match2, s(:lit, /foo/), s(:call, nil, :x))
        expect(parse('x !~ y')).must_equal s(:not, s(:call, s(:call, nil, :x), :=~, s(:call, nil, :y)))
        expect(parse('x !~ /foo/')).must_equal s(:not, s(:match3, s(:lit, /foo/), s(:call, nil, :x)))
        expect(parse('/foo/ !~ x')).must_equal s(:not, s(:match2, s(:lit, /foo/), s(:call, nil, :x)))
        expect(parse('foo <=> bar')).must_equal s(:call, s(:call, nil, :foo), :<=>, s(:call, nil, :bar))
        expect(parse('1**2')).must_equal s(:call, s(:lit, 1), :**, s(:lit, 2))
        expect(parse('foo :foo?=>:bar?')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :foo?), s(:lit, :bar?)))
        expect(parse('foo :foo=>:bar')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :foo), s(:lit, :bar)))
        expect(parse(':foo?==:bar?')).must_equal s(:call, s(:lit, :foo?), :==, s(:lit, :bar?))
        if parser == 'NatalieParser'
          # FIXME: These work with NatalieParser, but they shouldn't
          expect(parse('foo?==bar?')).must_equal s(:call, s(:call, nil, :foo?), :==, s(:call, nil, :bar?))
          expect(parse(':foo!==:bar!')).must_equal s(:call, s(:lit, :foo!), :==, s(:lit, :bar!))
          expect(parse('foo!==bar!')).must_equal s(:call, s(:call, nil, :foo!), :==, s(:call, nil, :bar!))
        else
          # MRI and RubyParser both say the '=' is unexpected
          expect(-> { parse('foo?==bar?') }).must_raise SyntaxError
          expect(-> { parse(':foo!==:bar!') }).must_raise SyntaxError
          expect(-> { parse('foo!==bar!') }).must_raise SyntaxError
        end
      end

      it 'parses and/or' do
        expect(parse('1 && 2 || 3 && 4')).must_equal s(:or, s(:and, s(:lit, 1), s(:lit, 2)), s(:and, s(:lit, 3), s(:lit, 4)))
        expect(parse('false and true and false')).must_equal s(:and, s(:false), s(:and, s(:true), s(:false)))
        expect(parse('false or true or false')).must_equal s(:or, s(:false), s(:or, s(:true), s(:false)))
        expect(parse('false && true && false')).must_equal s(:and, s(:false), s(:and, s(:true), s(:false)))
        expect(parse('false || true || false')).must_equal s(:or, s(:false), s(:or, s(:true), s(:false)))
        expect(parse('1 and 2 or 3 and 4')).must_equal s(:and, s(:or, s(:and, s(:lit, 1), s(:lit, 2)), s(:lit, 3)), s(:lit, 4))
        expect(parse('1 or 2 and 3 or 4')).must_equal s(:or, s(:and, s(:or, s(:lit, 1), s(:lit, 2)), s(:lit, 3)), s(:lit, 4))
      end

      it 'parses ! and not' do
        expect(parse('!false')).must_equal s(:call, s(:false), :!)
        expect(parse('not false')).must_equal s(:call, s(:false), :!)
        expect(parse('not foo bar')).must_equal s(:call, s(:call, nil, :foo, s(:call, nil, :bar)), :!)
        expect(parse('!foo.bar(baz)')).must_equal s(:call, s(:call, s(:call, nil, :foo), :bar, s(:call, nil, :baz)), :!)
      end

      it 'raises an error if there is a syntax error' do
        # We choose to more closely match what MRI does vs what ruby_parser raises
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse("1 + 2\n\n)") }, SyntaxError, "(string)#3: syntax error, unexpected ')' (expected: 'end-of-line')")
        else
          expect_raise_with_message(-> { parse("1 + 2\n\n)") }, SyntaxError, "(string):3 :: parse error on value \")\" (tRPAREN)")
        end
      end

      it 'parses strings' do
        expect(parse('""')).must_equal s(:str, '')
        expect(parse('"foo"')).must_equal s(:str, 'foo')
        expect(parse('"this is \"quoted\""')).must_equal s(:str, 'this is "quoted"')
        expect(parse('"other escaped chars \\\\ \n"')).must_equal s(:str, "other escaped chars \\ \n")
        expect(parse("''")).must_equal s(:str, '')
        expect(parse("'foo'")).must_equal s(:str, 'foo')
        expect(parse("'this is \\'quoted\\''")).must_equal s(:str, "this is 'quoted'")
        expect(parse("'other escaped chars \\\\ \\n'")).must_equal s(:str, "other escaped chars \\ \\n")
        expect(parse(%q('foo' 'bar'))).must_equal s(:str, 'foobar')
        expect(parse(%q('foo' "bar"))).must_equal s(:str, 'foobar')
        expect(parse(%q("foo" 'bar'))).must_equal s(:str, 'foobar')
        expect(parse(%q("foo" "bar"))).must_equal s(:str, 'foobar')
        expect(parse(%q(%Q{1 {2} 3}))).must_equal s(:str, "1 {2} 3")
        expect(parse(%q(%Q/1 {2} 3/))).must_equal s(:str, "1 {2} 3")
        expect(parse(%q(%Q[1 [2] 3]))).must_equal s(:str, "1 [2] 3")
        expect(parse(%q(%Q(1 (2) 3)))).must_equal s(:str, "1 (2) 3")
        expect(parse(%q(%{1 {2} 3}))).must_equal s(:str, "1 {2} 3")
        expect(parse(%q(%/1 {2} 3/))).must_equal s(:str, "1 {2} 3")
        expect(parse(%q(%[1 [2] 3]))).must_equal s(:str, "1 [2] 3")
        expect(parse(%q(%(1 (2) 3)))).must_equal s(:str, "1 (2) 3")
        expect(parse(%q(%'1 \' 2'))).must_equal s(:str, "1 ' 2")
        expect(parse(%q(%"1 \" 2"))).must_equal s(:str, '1 " 2')
        expect(parse(%q(%q'1 \' 2'))).must_equal s(:str, "1 ' 2")
        expect(parse(%q(%q"1 \" 2"))).must_equal s(:str, '1 " 2')
        expect(parse(%q(%Q'1 \' 2'))).must_equal s(:str, "1 ' 2")
        expect(parse(%q(%Q"1 \" 2"))).must_equal s(:str, '1 " 2')
        expect(parse("%q(1 (\\x\\') 2)")).must_equal s(:str, "1 (\\x\\') 2")
        expect(parse(%Q("foo\\\nbar"))).must_equal s(:str, "foobar")

        # interpolation
        expect(parse('"#{:foo} bar #{1 + 1}"')).must_equal s(:dstr, '', s(:evstr, s(:lit, :foo)), s(:str, ' bar '), s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))))
        expect(parse('y = "#{1 + 1} 2"')).must_equal s(:lasgn, :y, s(:dstr, '', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, ' 2')))
        expect(parse('x.y = "#{1 + 1} 2"')).must_equal s(:attrasgn, s(:call, nil, :x), :y=, s(:dstr, '', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, ' 2')))
        expect(parse('"#{foo { 1 }}"')).must_equal s(:dstr, "", s(:evstr, s(:iter, s(:call, nil, :foo), 0, s(:lit, 1))))
        expect(parse(%q("#{1}foo#{''}bar"))).must_equal s(:dstr, '', s(:evstr, s(:lit, 1)), s(:str, 'foo'), s(:str, ''), s(:str, 'bar'))
        expect(parse(%Q(\"a\n#\{\n}\"))).must_equal s(:dstr, "a\n", s(:evstr))
        expect(parse(%q("#{'a'}#{b}"))).must_equal s(:dstr, "a", s(:evstr, s(:call, nil, :b)))
        expect(parse(%q("#{'a'}#{'b'}"))).must_equal s(:str, "ab")
        expect(parse(%q("#{'a'} b"))).must_equal s(:str, "a b")
        expect(parse(%q("a #{1} b #{'c'}"))).must_equal s(:dstr, "a ", s(:evstr, s(:lit, 1)), s(:str, " b "), s(:str, "c"))
        expect(parse(%q("#{1+1}foo" "bar"))).must_equal s(:dstr, "", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, "foo"), s(:str, "bar"))
        expect(parse(%q("foo#{0+0}" 'bar#{1+1}'))).must_equal s(:dstr, "foo", s(:evstr, s(:call, s(:lit, 0), :+, s(:lit, 0))), s(:str, "bar\#{1+1}"))
        expect(parse(%q('foo#{0+0}' "bar#{1+1}"))).must_equal s(:dstr, "foo\#{0+0}bar", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))))
        expect(parse(%q('foo' "#{1+1}"))).must_equal s(:dstr, "foo", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))))
        expect(parse(%q("foo#{0+0}" "bar#{1+1}"))).must_equal s(:dstr, "foo", s(:evstr, s(:call, s(:lit, 0), :+, s(:lit, 0))), s(:str, "bar"), s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))))
        expect(parse(%q("#{0+0}foo" "bar#{1+1}"))).must_equal s(:dstr, "", s(:evstr, s(:call, s(:lit, 0), :+, s(:lit, 0))), s(:str, "foo"), s(:str, "bar"), s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))))
        expect(parse(%q("#{0+0}foo" "#{1+1}bar"))).must_equal s(:dstr, "", s(:evstr, s(:call, s(:lit, 0), :+, s(:lit, 0))), s(:str, "foo"), s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, "bar"))
        expect(parse("%{#\{ \"#\{1}\" }}")).must_equal s(:dstr, "", s(:evstr, s(:lit, 1)))
        expect(parse("%{ { #\{ \"#\{1}\" } } }")).must_equal s(:dstr, " { ", s(:evstr, s(:lit, 1)), s(:str, " } "))
        expect(parse('"#{p:a}"')).must_equal s(:dstr, "", s(:evstr, s(:call, nil, :p, s(:lit, :a))))

        # encoding
        expect(parse('"foo"').last.encoding.name).must_equal 'UTF-8'
        expect(parse('"🐮"').last.encoding.name).must_equal 'UTF-8'
        expect(parse('"\xC3"').last.encoding.name).must_equal 'ASCII-8BIT'

        # escapes
        {
          '\a'      => 7,   # bell, ASCII 07h (BEL)
          '\b'      => 8,   # backspace, ASCII 08h (BS)
          '\t'      => 9,   # horizontal tab, ASCII 09h (TAB)
          '\n'      => 10,  # newline (line feed), ASCII 0Ah (LF)
          '\v'      => 11,  # vertical tab, ASCII 0Bh (VT)
          '\f'      => 12,  # form feed, ASCII 0Ch (FF)
          '\r'      => 13,  # carriage return, ASCII 0Dh (CR)
          '\e'      => 27,  # escape, ASCII 1Bh (ESC)
          '\s'      => 32,  # space, ASCII 20h (SPC)
          '\\\\'    => 92,  # backslash, \
        }.each do |escape, value|
          ord = parse(%("#{escape}"))[1].ord
          expect(ord).must_equal value
        end

        # control character, where x is an ASCII printable character
        # 0-30 or 0-31 with three bands
        {
          '\c '     => 0,
          '\c!'     => 1,
          '\c.'     => 14,
          '\c4'     => 20,
          '\c>'     => 30,
          '\c?'     => 127, # special case, delete, ASCII 7Fh (DEL)
          '\C-?'    => 127, # special case, delete, ASCII 7Fh (DEL)
          '\c@'     => 0,
          '\cA'     => 1,
          '\C-X'    => 24,
          '\C-^'    => 30,
          '\C-_'    => 31,
          '\C-`'    => 0,
          '\C-a'    => 1,
          '\C-x'    => 24,
          '\C-~'    => 30,
        }.each do |escape, value|
          ord = parse(%("#{escape}"))[1].ord
          expect(ord).must_equal value
        end

        # meta character, where x is an ASCII printable character
        # +128
        {
          '\M-a'    => 225,
          '\M-A'    => 193,
          '\M-z'    => 250,
          '\M-Z'    => 218,
        }.each do |escape, value|
          ord = parse(%("#{escape}"))[1].bytes.first
          expect(ord).must_equal value
        end

        # meta control character, where x is an ASCII printable character
        # control char num + 128
        {
          '\M-\C-a' => 129,
          '\M-\C-A' => 129,
          '\M-\cX'  => 152,
          '\c\M-y'  => 153,
          '\C-\M-y'  => 153,
        }.each do |escape, value|
        ord = parse(%("#{escape}"))[1].bytes.first.ord
          expect(ord).must_equal value
        end
      end

      it 'throws a SyntaxError for unterminated strings' do
        if parser == 'NatalieParser'
          expect_raise_with_message(
            -> { parse("  \"foo\nbaz") },
            SyntaxError,
            "(string)#1: syntax error, unterminated string meets end of file (expected: '\"')\n" \
            "  \"foo\n" \
            "  ^ starts here, expected closing '\"' somewhere after"
          )
          expect_raise_with_message(
            -> { parse("  %(foo\nbaz") },
            SyntaxError,
            "(string)#1: syntax error, unterminated string meets end of file (expected: ')')\n" \
            "  %(foo\n" \
            "  ^ starts here, expected closing ')' somewhere after"
          )
          expect_raise_with_message(
            -> { parse("  'foo\nbaz") },
            SyntaxError,
            "(string)#1: syntax error, unterminated string meets end of file (expected: \"'\")\n" \
            "  'foo\n" \
            "  ^ starts here, expected closing \"'\" somewhere after"
          )
          expect_raise_with_message(-> { parse("%[foo\nbaz") }, SyntaxError, /expected closing '\]' somewhere after/)
          expect_raise_with_message(-> { parse("%q(foo\nbaz") }, SyntaxError, /expected closing '\)' somewhere after/)
          expect_raise_with_message(-> { parse("%Q(foo\nbaz") }, SyntaxError, /expected closing '\)' somewhere after/)
          expect_raise_with_message(-> { parse("%Q[foo\nbaz") }, SyntaxError, /expected closing '\]' somewhere after/)
          expect_raise_with_message(-> { parse("%Q/foo\nbaz") }, SyntaxError, /expected closing '\/' somewhere after/)
        end
      end

      it 'throws a SyntaxError for unterminated symbols' do
        if parser == 'NatalieParser'
          expect_raise_with_message(
            -> { parse("  :\"foo\nbaz") },
            SyntaxError,
            "(string)#1: syntax error, unterminated symbol meets end of file (expected: '\"')\n" \
            "  :\"foo\n" \
            "  ^ starts here, expected closing '\"' somewhere after"
          )
        end
      end

      it 'throws a SyntaxError for unterminated regexps' do
        if parser == 'NatalieParser'
          expect_raise_with_message(
            -> { parse("  /foo\nbaz") },
            SyntaxError,
            "(string)#1: syntax error, unterminated regexp meets end of file (expected: '/')\n" \
            "  /foo\n" \
            "  ^ starts here, expected closing '/' somewhere after"
          )
          expect_raise_with_message(-> { parse("%r[foo\nbaz") }, SyntaxError, /expected closing '\]' somewhere after/)
          expect_raise_with_message(-> { parse("%r(foo\nbaz") }, SyntaxError, /expected closing '\)' somewhere after/)
          expect_raise_with_message(-> { parse("%r/foo\nbaz") }, SyntaxError, /expected closing '\/' somewhere after/)
        end
      end

      it 'throws a SyntaxError for unterminated shells' do
        if parser == 'NatalieParser'
          expect_raise_with_message(
            -> { parse("  `foo\nbaz") },
            SyntaxError,
            "(string)#1: syntax error, unterminated shell meets end of file (expected: '`')\n" \
            "  `foo\n" \
            "  ^ starts here, expected closing '`' somewhere after"
          )
          expect_raise_with_message(-> { parse("%x[foo\nbaz") }, SyntaxError, /expected closing '\]' somewhere after/)
          expect_raise_with_message(-> { parse("%x(foo\nbaz") }, SyntaxError, /expected closing '\)' somewhere after/)
          expect_raise_with_message(-> { parse("%x/foo\nbaz") }, SyntaxError, /expected closing '\/' somewhere after/)
        end
      end

      it 'throws a SyntaxError for unterminated word arrays' do
        if parser == 'NatalieParser'
          expect_raise_with_message(
            -> { parse("  %w[foo\nbaz") },
            SyntaxError,
            "(string)#1: syntax error, unterminated word array meets end of file (expected: ']')\n" \
            "  %w[foo\n" \
            "  ^ starts here, expected closing ']' somewhere after"
          )
          expect_raise_with_message(-> { parse("%w(foo\nbaz") }, SyntaxError, /expected closing '\)' somewhere after/)
          expect_raise_with_message(-> { parse("%w/foo\nbaz") }, SyntaxError, /expected closing '\/' somewhere after/)
          expect_raise_with_message(-> { parse('%W[ foo#{#1} bar ]') }, SyntaxError, /expected closing delimiter somewhere after/)
        end
      end

      it 'parses symbols' do
        expect(parse(':foo')).must_equal s(:lit, :foo)
        expect(parse(':foo_bar')).must_equal s(:lit, :foo_bar)
        expect(parse(':"foo bar"')).must_equal s(:lit, :'foo bar')
        expect(parse(':"foo #{1+1}"')).must_equal s(:dsym, "foo ", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))))
        expect(parse(':"#{1+1}"')).must_equal s(:dsym, "", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))))
        expect(parse('foo :"bar"')).must_equal s(:call, nil, :foo, s(:lit, :bar))
        expect(parse(':FooBar')).must_equal s(:lit, :FooBar)
        expect(-> { parse('"foo" "bar":') }).must_raise SyntaxError
        expect(-> { parse('"foo": "bar":') }).must_raise SyntaxError
      end

      it 'parses symbols with correct encoding' do
        # sanity check with MRI
        expect(:"\xC3\x9Cber".encoding.name).must_equal 'UTF-8'
        expect(:"\xf0\x9f\x90\xae".encoding.name).must_equal 'UTF-8'

        sym = parse(':foo').last
        expect(sym.encoding.name).must_equal 'US-ASCII'
        expect(sym.length).must_equal 3

        sym = parse(':"\xC3\x9Cber"').last # :Über
        if parser == 'NatalieParser'
          # this is what Ruby does
          expect(sym.encoding.name).must_equal 'UTF-8'
          expect(sym.length).must_equal 4
        else
          expect(sym.encoding.name).must_equal 'ASCII-8BIT'
          expect(sym.length).must_equal 5
        end

        sym = parse(':"\xf0\x9f\x90\xae"').last # :🐮
        if parser == 'NatalieParser'
          # this is what Ruby does
          expect(sym.encoding.name).must_equal 'UTF-8'
          expect(sym.length).must_equal 1
        else
          expect(sym.encoding.name).must_equal 'ASCII-8BIT'
          expect(sym.length).must_equal 4
        end

        sym = parse(':"🐮"').last # "\xf0\x9f\x90\xae"
        expect(sym.encoding.name).must_equal 'UTF-8'
        expect(sym.length).must_equal 1
      end

      it 'parses regexps' do
        expect(parse('/foo/')).must_equal s(:lit, /foo/)
        expect(parse('/foo/i')).must_equal s(:lit, /foo/i)
        expect(parse('/abc/').last.encoding).must_equal Encoding::US_ASCII
        expect(parse('/🐄/').last.encoding).must_equal Encoding::UTF_8
        expect(parse('//mix')).must_equal s(:lit, //mix)
        %w[e s u].each do |flag|
          expect(parse("//#{flag}").last.options).must_equal 16
        end
        expect(parse('/#{1+1}/mixn')).must_equal s(:dregx, '', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), 39)
        expect(parse('/foo #{1+1}/')).must_equal s(:dregx, 'foo ', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))))
        expect(parse('/#{1+1}/')).must_equal s(:dregx, "", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))))
        expect(parse('/^$(.)[.]{1}.*.+.?\^\$\.\(\)\[\]\{\}\w\W\d\D\h\H\s\S\R\*\+\?/')).must_equal s(:lit, /^$(.)[.]{1}.*.+.?\^\$\.\(\)\[\]\{\}\w\W\d\D\h\H\s\S\R\*\+\?/)
        expect(parse("/\\n\\\\n/")).must_equal s(:lit, /\n\\n/)
        expect(parse("/\\/\\* foo \\*\\//")).must_equal s(:lit, Regexp.new("/\\* foo \\*/"))
        expect(parse("/\\&\\a\\b/")).must_equal s(:lit, /\&\a\b/)
        expect(parse(%q(%r/foo\nbar/))).must_equal s(:lit, /foo\nbar/)
        expect(parse(%q(%r'foo\nbar'))).must_equal s(:lit, /foo\nbar/)
        expect(parse(%q(%r"foo\nbar"))).must_equal s(:lit, /foo\nbar/)
        expect(parse("%r(foo(\\n)bar)")).must_equal s(:lit, /foo(\n)bar/)
        expect(parse(%(%r'\\\"'))).must_equal s(:lit, /\"/) # %r'\"'
        expect(parse(%(%r"\\\""))).must_equal s(:lit, /"/) # %r"\""
        expect(parse(%(%r'\\\''))).must_equal s(:lit, /'/) # %r'\''
        expect(parse(%(%r"\\\'"))).must_equal s(:lit, /\'/) # %r"\'"
      end

      it 'parses regexps with leading space preceeded by keywords' do
        expect(parse("if / foo/; end")).must_equal s(:if, s(:match, s(:lit, / foo/)), nil, nil)
        expect(parse("if false; elsif / foo/; end")).must_equal s(:if, s(:false), nil, s(:if, s(:lit, / foo/), nil, nil))
        expect(parse("begin; 1; rescue / foo/; end")).must_equal s(:rescue, s(:lit, 1), s(:resbody, s(:array, s(:lit, / foo/)), nil))
        expect(parse("return / foo/")).must_equal s(:return, s(:lit, / foo/))
        expect(parse("unless / foo/; end")).must_equal s(:if, s(:match, s(:lit, / foo/)), nil, nil)
        expect(parse("case; when / foo/; end")).must_equal s(:case, nil, s(:when, s(:array, s(:lit, / foo/)), nil), nil)
        expect(parse("while / foo/; end")).must_equal s(:while, s(:match, s(:lit, / foo/)), nil, true)
        expect(parse("until / foo/; end")).must_equal s(:until, s(:match, s(:lit, / foo/)), nil, true)
      end

      it 'parses multiple expressions' do
        expect(parse("1 + 2\n3 + 4")).must_equal s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:call, s(:lit, 3), :+, s(:lit, 4)))
        expect(parse("1 + 2;'foo'")).must_equal s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, 'foo'))
      end

      it 'parses colon colon' do
        expect(parse('foo::bar')).must_equal s(:call, s(:call, nil, :foo), :bar)
        expect(parse('foo::Bar')).must_equal s(:colon2, s(:call, nil, :foo), :Bar)
        expect(parse('foo::()')).must_equal s(:call, s(:call, nil, :foo), :call)
        expect(parse('foo::(1, 2)')).must_equal s(:call, s(:call, nil, :foo), :call, s(:lit, 1), s(:lit, 2))
      end

      it 'parses assignment' do
        expect(parse('x = 1')).must_equal s(:lasgn, :x, s(:lit, 1))
        expect(parse('x = 1 + 2')).must_equal s(:lasgn, :x, s(:call, s(:lit, 1), :+, s(:lit, 2)))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('x =') }, SyntaxError, "(string)#1: syntax error, unexpected end-of-input (expected: 'expression')")
          expect_raise_with_message(-> { parse('[1] = 2') }, SyntaxError, "(string)#1: syntax error, unexpected '[' (expected: 'left side of assignment')")
        else
          expect_raise_with_message(-> { parse('x =') }, SyntaxError, '(string):1 :: parse error on value "$" ($end)')
          expect_raise_with_message(-> { parse('[1] = 2') }, SyntaxError, '(string):1 :: parse error on value "=" (tEQL)')
        end
        expect(parse('@foo = 1')).must_equal s(:iasgn, :@foo, s(:lit, 1))
        expect(parse('@@abc_123 = 1')).must_equal s(:cvdecl, :@@abc_123, s(:lit, 1))
        expect(parse('$baz = 1')).must_equal s(:gasgn, :$baz, s(:lit, 1))
        expect(parse('Constant = 1')).must_equal s(:cdecl, :Constant, s(:lit, 1))
        expect(parse('x, y = [1, 2]')).must_equal s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('(x, y) = [1, 2]')).must_equal s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('(x) = [1, 2]') }, SyntaxError, "(string)#1: syntax error, unexpected '=' (expected: 'multiple assignment on left-hand-side')")
          expect_raise_with_message(-> { parse('(x, = [1, 2]') }, SyntaxError, "(string)#1: syntax error, unexpected end-of-input (expected: 'group closing paren')")
          expect_raise_with_message(-> { parse('x,) = [1, 2]') }, SyntaxError, "(string)#1: syntax error, unexpected ')' (expected: 'end-of-line')")
        else
          expect_raise_with_message(-> { parse('(x) = [1, 2]') }, SyntaxError, "(string):1 :: parse error on value \"=\" (tEQL)")
          expect_raise_with_message(-> { parse('(x, = [1, 2]') }, SyntaxError, "(string):1 :: parse error on value \"$\" ($end)")
          expect_raise_with_message(-> { parse('x,) = [1, 2]') }, SyntaxError, "(string):1 :: parse error on value \")\" (tRPAREN)")
        end
        expect(parse('x, = [1, 2]')).must_equal s(:masgn, s(:array, s(:lasgn, :x)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('(x,) = [1, 2]')).must_equal s(:masgn, s(:array, s(:lasgn, :x)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('(x, y, ) = [1, 2]')).must_equal s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('((x, y)) = [1, 2]')).must_equal s(:masgn, s(:array, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)))), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('((x, y),) = [1, 2]')).must_equal s(:masgn, s(:array, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)))), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('(((x, y))) = [1, 2]')).must_equal s(:masgn, s(:array, s(:masgn, s(:array, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)))))), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('(((x, y),)) = [1, 2]')).must_equal s(:masgn, s(:array, s(:masgn, s(:array, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)))))), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('(((x, y)),) = [1, 2]')).must_equal s(:masgn, s(:array, s(:masgn, s(:array, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)))))), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('@x, $y, Z = foo')).must_equal s(:masgn, s(:array, s(:iasgn, :@x), s(:gasgn, :$y), s(:cdecl, :Z)), s(:to_ary, s(:call, nil, :foo)))
        expect(parse('(@x, $y, Z) = foo')).must_equal s(:masgn, s(:array, s(:iasgn, :@x), s(:gasgn, :$y), s(:cdecl, :Z)), s(:to_ary, s(:call, nil, :foo)))
        expect(parse('(a, (b, c)) = [1, [2, 3]]')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:lasgn, :c)))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3)))))
        expect(parse('(a, b), c = [[1, 2], 3]')).must_equal s(:masgn, s(:array, s(:masgn, s(:array, s(:lasgn, :a), s(:lasgn, :b))), s(:lasgn, :c)), s(:to_ary, s(:array, s(:array, s(:lit, 1), s(:lit, 2)), s(:lit, 3))))
        expect(parse('((a, b), c) = [[1, 2], 3]')).must_equal s(:masgn, s(:array, s(:masgn, s(:array, s(:lasgn, :a), s(:lasgn, :b))), s(:lasgn, :c)), s(:to_ary, s(:array, s(:array, s(:lit, 1), s(:lit, 2)), s(:lit, 3))))
        expect(parse('a, (b, c) = [1, [2, 3]]')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:lasgn, :c)))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3)))))
        expect(parse('a, (b, *c) = [1, [2, 3, 4]]')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:splat, s(:lasgn, :c))))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3), s(:lit, 4)))))
        expect(parse('a, (b, *@c) = [1, [2, 3, 4]]')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:splat, s(:iasgn, :@c))))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3), s(:lit, 4)))))
        expect(parse('_, a, *b = [1, 2, 3, 4]')).must_equal s(:masgn, s(:array, s(:lasgn, :_), s(:lasgn, :a), s(:splat, s(:lasgn, :b))), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4))))
        expect(parse('(_, a, *b) = [1, 2, 3, 4]')).must_equal s(:masgn, s(:array, s(:lasgn, :_), s(:lasgn, :a), s(:splat, s(:lasgn, :b))), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4))))
        expect(parse('(a, *) = [1, 2, 3, 4]')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:splat)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4))))
        expect(parse('(a, *, c) = [1, 2, 3, 4]')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:splat), s(:lasgn, :c)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4))))
        expect(parse('x = foo.bar')).must_equal s(:lasgn, :x, s(:call, s(:call, nil, :foo), :bar))
        expect(parse('x = y = 2')).must_equal s(:lasgn, :x, s(:lasgn, :y, s(:lit, 2)))
        expect(parse('x = y = z = 2')).must_equal s(:lasgn, :x, s(:lasgn, :y, s(:lasgn, :z, s(:lit, 2))))
        expect(parse('x = 1, 2')).must_equal s(:lasgn, :x, s(:svalue, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('x, y = 1, 2')).must_equal s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('x, y = 1')).must_equal s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:lit, 1)))
        expect(parse('x = *[1, 2]')).must_equal s(:lasgn, :x, s(:svalue, s(:splat, s(:array, s(:lit, 1), s(:lit, 2)))))
        expect(parse('x, y = *[1, 2]')).must_equal s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:splat, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('* = f')).must_equal s(:masgn, s(:array, s(:splat)), s(:to_ary, s(:call, nil, :f)))
        expect(parse('*, x = f')).must_equal s(:masgn, s(:array, s(:splat), s(:lasgn, :x)), s(:to_ary, s(:call, nil, :f)))
        expect(parse('x, *, y = f')).must_equal s(:masgn, s(:array, s(:lasgn, :x), s(:splat), s(:lasgn, :y)), s(:to_ary, s(:call, nil, :f)))
        expect(parse('x, * = f')).must_equal s(:masgn, s(:array, s(:lasgn, :x), s(:splat)), s(:to_ary, s(:call, nil, :f)))
        expect(parse('::FOO = 1')).must_equal s(:cdecl, s(:colon3, :FOO), s(:lit, 1))
        expect(parse('Foo::BAR = 1')).must_equal s(:cdecl, s(:colon2, s(:const, :Foo), :BAR), s(:lit, 1))
        if parser == 'NatalieParser'
          # We replace :const with :cdecl to be consistent with single-assignment.
          expect(parse('A, B::C = 1, 2')).must_equal s(:masgn, s(:array, s(:cdecl, :A), s(:cdecl, s(:colon2, s(:const, :B), :C))), s(:array, s(:lit, 1), s(:lit, 2)))
          expect(parse('A, ::B = 1, 2')).must_equal s(:masgn, s(:array, s(:cdecl, :A), s(:cdecl, s(:colon3, :B))), s(:array, s(:lit, 1), s(:lit, 2)))
          expect(parse('::A, ::B = 1, 2')).must_equal s(:masgn, s(:array, s(:cdecl, s(:colon3, :A)), s(:cdecl, s(:colon3, :B))), s(:array, s(:lit, 1), s(:lit, 2)))
          expect(parse('A::B, ::C = 1, 2')).must_equal s(:masgn, s(:array, s(:cdecl, s(:colon2, s(:const, :A), :B)), s(:cdecl, s(:colon3, :C))), s(:array, s(:lit, 1), s(:lit, 2)))
        else
          expect(parse('A, B::C = 1, 2')).must_equal s(:masgn, s(:array, s(:cdecl, :A), s(:const, s(:colon2, s(:const, :B), :C))), s(:array, s(:lit, 1), s(:lit, 2)))
          expect(parse('A, ::B = 1, 2')).must_equal s(:masgn, s(:array, s(:cdecl, :A), s(:const, s(:colon3, :B))), s(:array, s(:lit, 1), s(:lit, 2)))
          expect(parse('::A, ::B = 1, 2')).must_equal s(:masgn, s(:array, s(:const, nil, s(:colon3, :A)), s(:const, s(:colon3, :B))), s(:array, s(:lit, 1), s(:lit, 2)))
          expect(parse('A::B, ::C = 1, 2')).must_equal s(:masgn, s(:array, s(:const, s(:colon2, s(:const, :A), :B), nil), s(:const, s(:colon3, :C))), s(:array, s(:lit, 1), s(:lit, 2)))
        end
        expect(parse('a.b, c = 1, 2')).must_equal s(:masgn, s(:array, s(:attrasgn, s(:call, nil, :a), :b=), s(:lasgn, :c)), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('a, b.c = 1, 2')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:attrasgn, s(:call, nil, :b), :c=)), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('a, b.c.d = 1, 2')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:attrasgn, s(:call, s(:call, nil, :b), :c), :d=)), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('*a, b = 1, 2')).must_equal s(:masgn, s(:array, s(:splat, s(:lasgn, :a)), s(:lasgn, :b)), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('*a.b, c = 1, 2')).must_equal s(:masgn, s(:array, s(:splat, s(:attrasgn, s(:call, nil, :a), :b=)), s(:lasgn, :c)), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('a, *b.c = 1, 2')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:splat, s(:attrasgn, s(:call, nil, :b), :c=))), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('a, b[c] = 1, 2')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:attrasgn, s(:call, nil, :b), :[]=, s(:call, nil, :c))), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('a, b[c][d] = 1, 2')).must_equal s(:masgn, s(:array, s(:lasgn, :a), s(:attrasgn, s(:call, s(:call, nil, :b), :[], s(:call, nil, :c)), :[]=, s(:call, nil, :d))), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('h[:foo], h[:bar] = 1, 2')).must_equal s(:masgn, s(:array, s(:attrasgn, s(:call, nil, :h), :[]=, s(:lit, :foo)), s(:attrasgn, s(:call, nil, :h), :[]=, s(:lit, :bar))), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('*a = 1, 2')).must_equal s(:masgn, s(:array, s(:splat, s(:lasgn, :a))), s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('x = 1 && 2 && 3')).must_equal s(:lasgn, :x, s(:and, s(:lit, 1), s(:and, s(:lit, 2), s(:lit, 3))))
        expect(parse('true && false && x = 1')).must_equal s(:and, s(:true), s(:and, s(:false), s(:lasgn, :x, s(:lit, 1))))
        expect(parse('true and false and x = 1')).must_equal s(:and, s(:true), s(:and, s(:false), s(:lasgn, :x, s(:lit, 1))))
        expect(parse('x = 1 && 2 && 3')).must_equal s(:lasgn, :x, s(:and, s(:lit, 1), s(:and, s(:lit, 2), s(:lit, 3))))
        expect(parse('x = 1 and 2 and 3')).must_equal s(:and, s(:lasgn, :x, s(:lit, 1)), s(:and, s(:lit, 2), s(:lit, 3)))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('x, y+z = 1, 2') }, SyntaxError, "(string)#1: syntax error, unexpected '+' (expected: 'left side of assignment')")
          expect_raise_with_message(-> { parse('-x, y = 1, 2') }, SyntaxError, "(string)#1: syntax error, unexpected ',' (expected: 'assignment =')")
          expect_raise_with_message(-> { parse('foo, (bar=1, baz) = buz') }, SyntaxError, "(string)#1: syntax error, unexpected '=' (expected: 'closing paren for multiple assignment')")
        else
          expect_raise_with_message(-> { parse('x, y+z = 1, 2') }, SyntaxError, '(string):1 :: parse error on value "+" (tPLUS)')
          expect_raise_with_message(-> { parse('-x, y = 1, 2') }, SyntaxError, "(string):1 :: parse error on value \",\" (tCOMMA)")
          expect_raise_with_message(-> { parse('foo, (bar=1, baz) = buz') }, SyntaxError, "(string):1 :: parse error on value \"=\" (tEQL)")
        end
        expect(parse('a&.b = 1')).must_equal s(:safe_attrasgn, s(:call, nil, :a), :b=, s(:lit, 1))
        expect(parse('a&.B = 1')).must_equal s(:safe_attrasgn, s(:call, nil, :a), :B=, s(:lit, 1))
      end

      it 'parses proper line number for multiple assignment' do
        result = parse("x,\ny = 1")
        expect(result).must_equal s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:lit, 1)))
        expect(result.sexp_type).must_equal :masgn
        expect(result.line).must_equal 1
        expect(result.column).must_equal 1 if parser == 'NatalieParser'
      end

      it 'parses attr assignment' do
        expect(parse('x.y = 1')).must_equal s(:attrasgn, s(:call, nil, :x), :y=, s(:lit, 1))
        expect(parse('x[y] = 1')).must_equal s(:attrasgn, s(:call, nil, :x), :[]=, s(:call, nil, :y), s(:lit, 1))
        expect(parse('foo[a, b] = :bar')).must_equal s(:attrasgn, s(:call, nil, :foo), :[]=, s(:call, nil, :a), s(:call, nil, :b), s(:lit, :bar))
        expect(parse('foo[] = :bar')).must_equal s(:attrasgn, s(:call, nil, :foo), :[]=, s(:lit, :bar))
        expect(parse('foo.bar=x')).must_equal s(:attrasgn, s(:call, nil, :foo), :bar=, s(:call, nil, :x))
      end

      it 'parses [] as an array vs as a method' do
        expect(parse('foo[1]')).must_equal s(:call, s(:call, nil, :foo), :[], s(:lit, 1))
        expect(parse('foo[1,]')).must_equal s(:call, s(:call, nil, :foo), :[], s(:lit, 1))
        expect(parse('foo[1=>2]')).must_equal s(:call, s(:call, nil, :foo), :[], s(bare_hash_type, s(:lit, 1), s(:lit, 2)))
        expect(parse('foo[1=>2,]')).must_equal s(:call, s(:call, nil, :foo), :[], s(bare_hash_type, s(:lit, 1), s(:lit, 2)))
        expect(parse('foo [1]')).must_equal s(:call, nil, :foo, s(:array, s(:lit, 1)))
        expect(parse('foo = []; foo[1]')).must_equal s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
        expect(parse('foo = [1]; foo[1]')).must_equal s(:block, s(:lasgn, :foo, s(:array, s(:lit, 1))), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
        expect(parse('foo = []; foo [1]')).must_equal s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
        expect(parse('foo = []; foo [1, 2]')).must_equal s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1), s(:lit, 2)))
        expect(parse('foo[a]')).must_equal s(:call, s(:call, nil, :foo), :[], s(:call, nil, :a))
        expect(parse('foo[]')).must_equal s(:call, s(:call, nil, :foo), :[])
        expect(parse('foo []')).must_equal s(:call, nil, :foo, s(:array))
        expect(parse('foo [a, b]')).must_equal s(:call, nil, :foo, s(:array, s(:call, nil, :a), s(:call, nil, :b)))
      end

      it 'parses method definition' do
        expect(parse("def foo\nend")).must_equal s(:defn, :foo, s(:args), s(:nil))
        expect(parse("def Foo\nend")).must_equal s(:defn, :Foo, s(:args), s(:nil))
        expect(parse('def foo;end')).must_equal s(:defn, :foo, s(:args), s(:nil))
        expect(parse("def foo\n1\nend")).must_equal s(:defn, :foo, s(:args), s(:lit, 1))
        expect(parse('def foo;1;end')).must_equal s(:defn, :foo, s(:args), s(:lit, 1))
        expect(parse('def foo();1;end')).must_equal s(:defn, :foo, s(:args), s(:lit, 1))
        expect(parse('def foo() 1 end')).must_equal s(:defn, :foo, s(:args), s(:lit, 1))
        expect(parse("def foo;1;2 + 2;'foo';end")).must_equal s(:defn, :foo, s(:args), s(:lit, 1), s(:call, s(:lit, 2), :+, s(:lit, 2)), s(:str, 'foo'))
        expect(parse("def foo x, y\nend")).must_equal s(:defn, :foo, s(:args, :x, :y), s(:nil))
        expect(parse("def foo x,\ny\nend")).must_equal s(:defn, :foo, s(:args, :x, :y), s(:nil))
        expect(parse("def foo(x, y)\nend")).must_equal s(:defn, :foo, s(:args, :x, :y), s(:nil))
        expect(parse('def foo(x, y);end')).must_equal s(:defn, :foo, s(:args, :x, :y), s(:nil))
        expect(parse("def foo(\nx,\n y\n)\nend")).must_equal s(:defn, :foo, s(:args, :x, :y), s(:nil))
        expect(parse("def foo(x, y)\n1\n2\nend")).must_equal s(:defn, :foo, s(:args, :x, :y), s(:lit, 1), s(:lit, 2))
        expect(parse("def foo n\nn\nend")).must_equal s(:defn, :foo, s(:args, :n), s(:lvar, :n))
        expect(parse("def foo((a, b), c, (d, e))\nend")).must_equal s(:defn, :foo, s(:args, s(:masgn, :a, :b), :c, s(:masgn, :d, :e)), s(:nil))
        expect(parse('def foo!; end')).must_equal s(:defn, :foo!, s(:args), s(:nil))
        expect(parse('def foo?; end')).must_equal s(:defn, :foo?, s(:args), s(:nil))
        expect(parse('def foo=; end')).must_equal s(:defn, :foo=, s(:args), s(:nil))
        expect(parse('def self.foo; end')).must_equal s(:defs, s(:self), :foo, s(:args), s(:nil))
        expect(parse('def self.Foo; end')).must_equal s(:defs, s(:self), :Foo, s(:args), s(:nil))
        expect(parse('def Foo.bar; end')).must_equal s(:defs, s(:const, :Foo), :bar, s(:args), s(:nil))
        expect(parse('def Foo.Bar; end')).must_equal s(:defs, s(:const, :Foo), :Bar, s(:args), s(:nil))
        expect(parse('def self.foo=; end')).must_equal s(:defs, s(:self), :foo=, s(:args), s(:nil))
        expect(parse('def foo.bar=; end')).must_equal s(:defs, s(:call, nil, :foo), :bar=, s(:args), s(:nil))
        expect(parse('def Foo.foo; end')).must_equal s(:defs, s(:const, :Foo), :foo, s(:args), s(:nil))
        expect(parse('foo=o; def foo.bar; end')).must_equal s(:block, s(:lasgn, :foo, s(:call, nil, :o)), s(:defs, s(:lvar, :foo), :bar, s(:args), s(:nil)))
        expect(parse('def foo(*); end')).must_equal s(:defn, :foo, s(:args, :*), s(:nil))
        expect(parse('def foo(*x); end')).must_equal s(:defn, :foo, s(:args, :'*x'), s(:nil))
        expect(parse('def foo(x, *y, z); end')).must_equal s(:defn, :foo, s(:args, :x, :'*y', :z), s(:nil))
        expect(parse('def foo(a, &b); end')).must_equal s(:defn, :foo, s(:args, :a, :'&b'), s(:nil))
        expect(parse('def foo &a; end')).must_equal s(:defn, :foo, s(:args, :"&a"), s(:nil))
        expect(parse('def foo(a = nil, b = foo, c = FOO); end')).must_equal s(:defn, :foo, s(:args, s(:lasgn, :a, s(:nil)), s(:lasgn, :b, s(:call, nil, :foo)), s(:lasgn, :c, s(:const, :FOO))), s(:nil))
        expect(parse('bar def foo() end')).must_equal s(:call, nil, :bar, s(:defn, :foo, s(:args), s(:nil)))
        expect(parse('def (@foo = bar).===(obj); end')).must_equal s(:defs, s(:iasgn, :@foo, s(:call, nil, :bar)), :===, s(:args, :obj), s(:nil))
        expect(parse('def foo(a=b=c={}); c; end')).must_equal s(:defn, :foo, s(:args, s(:lasgn, :a, s(:lasgn, :b, s(:lasgn, :c, s(:hash))))), s(:lvar, :c))

        # args in wrong order
        expect(-> { parse('def foo(a, *b, c = nil) end') }).must_raise SyntaxError
        expect(-> { parse('def foo(a, **b, c = nil) end') }).must_raise SyntaxError
        expect(-> { parse('def foo(a, **b, *c) end') }).must_raise SyntaxError
        expect(-> { parse('def foo(b:, a) end') }).must_raise SyntaxError

        # operators
        expect(parse('def -@; end')).must_equal s(:defn, :-@, s(:args), s(:nil))
        expect(parse('def +@; end')).must_equal s(:defn, :+@, s(:args), s(:nil))
        expect(-> { parse('def +@.foo; end') }).must_raise SyntaxError
        if parser == 'NatalieParser'
          expect(parse('def ~@; end')).must_equal s(:defn, :'~@', s(:args), s(:nil))
        else
          # NOTE: RubyParser removes the '@' -- not sure if that's right, but it's hella inconsistent
          expect(parse('def ~@; end')).must_equal s(:defn, :~, s(:args), s(:nil))
        end
        expect(parse('def !@; end')).must_equal s(:defn, :'!@', s(:args), s(:nil))

        # single-line method defs
        expect(parse('def exec(cmd) = system(cmd)')).must_equal s(:defn, :exec, s(:args, :cmd), s(:call, nil, :system, s(:lvar, :cmd)))
        expect(parse('def foo = bar')).must_equal s(:defn, :foo, s(:args), s(:call, nil, :bar))
        expect(parse('def self.exec(cmd) = system(cmd) rescue nil')).must_equal s(:defs, s(:self), :exec, s(:args, :cmd), s(:rescue, s(:call, nil, :system, s(:lvar, :cmd)), s(:resbody, s(:array), s(:nil))))
        expect(parse("def ==(o) = 42")).must_equal s(:defn, :==, s(:args, :o), s(:lit, 42))
        expect(parse("def self.==(o) = 42")).must_equal s(:defs, s(:self), :==, s(:args, :o), s(:lit, 42))
        expect(-> { parse('def foo a, b = bar') }).must_raise SyntaxError
        expect_raise_with_message(-> { parse("def self.x=(o) = 42") }, SyntaxError, "setter method cannot be defined in an endless method definition")
        expect_raise_with_message(-> { parse("def self.[]=(k, v) = 42") }, SyntaxError, "setter method cannot be defined in an endless method definition")
      end

      it 'parses method definition keyword args' do
        expect(parse('def foo(a, b: :c, d:); end')).must_equal s(:defn, :foo, s(:args, :a, s(:kwarg, :b, s(:lit, :c)), s(:kwarg, :d)), s(:nil))
        expect(parse('def foo bar: 1; end')).must_equal s(:defn, :foo, s(:args, s(:kwarg, :bar, s(:lit, 1))), s(:nil))
        expect(parse('def foo bar:; end')).must_equal s(:defn, :foo, s(:args, s(:kwarg, :bar)), s(:nil))
        expect(parse('def self.foo bar: 1; end')).must_equal s(:defs, s(:self), :foo, s(:args, s(:kwarg, :bar, s(:lit, 1))), s(:nil))
        expect(parse('def foo Bar: 1; end')).must_equal s(:defn, :foo, s(:args, s(:kwarg, :Bar, s(:lit, 1))), s(:nil))
      end

      it 'parses undef' do
        expect(-> { parse('undef') }).must_raise SyntaxError
        expect(parse('undef foo')).must_equal s(:undef, s(:lit, :foo))
        expect(parse('undef :foo')).must_equal s(:undef, s(:lit, :foo))
        expect(parse('undef :"foo"')).must_equal s(:undef, s(:lit, :foo))
        expect(parse('undef Foo')).must_equal s(:undef, s(:lit, :Foo))
        multiple_args_result = parse('undef foo, :bar')
        expect(multiple_args_result).must_equal s(:block, s(:undef, s(:lit, :foo)), s(:undef, s(:lit, :bar)))
      end

      it 'parses operator method definitions' do
        operators = %i[+ - * ** / % == === != =~ !~ > >= < <= <=> & | ^ ~ << >> [] []=]
        operators.each do |operator|
          expect(parse("def #{operator}; end")).must_equal s(:defn, operator, s(:args), s(:nil))
          expect(parse("def #{operator}(x)\nend")).must_equal s(:defn, operator, s(:args, :x), s(:nil))
          expect(parse("def self.#{operator}; end")).must_equal s(:defs, s(:self), operator, s(:args), s(:nil))
        end
      end

      it 'parses method definitions named after a keyword' do
        keywords = %i[
          alias
          and
          begin
          begin
          break
          case
          class
          defined
          def
          do
          else
          elsif
          encoding
          end
          end
          ensure
          false
          file
          for
          if
          in
          line
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
        ].each do |keyword|
          expect(parse("def #{keyword}(x); end")).must_equal s(:defn, keyword, s(:args, :x), s(:nil))
          expect(parse("def self.#{keyword} x; end")).must_equal s(:defs, s(:self), keyword, s(:args, :x), s(:nil))
        end
      end

      it 'parses method calls vs local variable lookup' do
        expect(parse('foo')).must_equal s(:call, nil, :foo)
        expect(parse('foo?')).must_equal s(:call, nil, :foo?)
        expect(parse('foo = 1; foo')).must_equal s(:block, s(:lasgn, :foo, s(:lit, 1)), s(:lvar, :foo))
        expect(parse('foo ||= 1; foo')).must_equal s(:block, s(:op_asgn_or, s(:lvar, :foo), s(:lasgn, :foo, s(:lit, 1))), s(:lvar, :foo))
        expect(parse('foo = 1; def bar; foo; end')).must_equal s(:block, s(:lasgn, :foo, s(:lit, 1)), s(:defn, :bar, s(:args), s(:call, nil, :foo)))
        expect(parse('@foo = 1; foo')).must_equal s(:block, s(:iasgn, :@foo, s(:lit, 1)), s(:call, nil, :foo))
        expect(parse('foo, bar = [1, 2]; foo; bar')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :foo), s(:lasgn, :bar)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2)))), s(:lvar, :foo), s(:lvar, :bar))
        expect(parse('(foo, (bar, baz)) = [1, [2, 3]]; foo; bar; baz')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :foo), s(:masgn, s(:array, s(:lasgn, :bar), s(:lasgn, :baz)))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3))))), s(:lvar, :foo), s(:lvar, :bar), s(:lvar, :baz))
      end

      it 'parses op assignment' do
        expect(parse("a *= 2")).must_equal s(:lasgn, :a, s(:call, s(:lvar, :a), :*, s(:lit, 2)))
        expect(parse("@a *= 2")).must_equal s(:iasgn, :@a, s(:call, s(:ivar, :@a), :*, s(:lit, 2)))
        expect(parse("@@a *= 2")).must_equal s(:cvdecl, :@@a, s(:call, s(:cvar, :@@a), :*, s(:lit, 2)))
        expect(parse("$a *= 2")).must_equal s(:gasgn, :$a, s(:call, s(:gvar, :$a), :*, s(:lit, 2)))
        expect(parse("A *= 2")).must_equal s(:cdecl, :A, s(:call, s(:const, :A), :*, s(:lit, 2)))
        expect(parse("::A *= 2")).must_equal s(:op_asgn, s(:colon3, :A), :*, s(:lit, 2))
        expect(parse("A::B *= 2")).must_equal s(:op_asgn, s(:colon2, s(:const, :A), :B), :*, s(:lit, 2))
        expect(parse("::A *= c")).must_equal s(:op_asgn, s(:colon3, :A), :*, s(:call, nil, :c))
        expect(parse("A::B *= c")).must_equal s(:op_asgn, s(:colon2, s(:const, :A), :B), :*, s(:call, nil, :c))
        expect(parse('x[] *= 2')).must_equal s(:op_asgn1, s(:call, nil, :x), nil, :*, s(:lit, 2))
        expect(parse('x[:y] *= 2')).must_equal s(:op_asgn1, s(:call, nil, :x), s(:arglist, s(:lit, :y)), :*, s(:lit, 2))
        expect(parse('x.y *= 2')).must_equal s(:op_asgn2, s(:call, nil, :x), :y=, :*, s(:lit, 2))
        expect(parse("a ||= c 1")).must_equal s(:op_asgn_or, s(:lvar, :a), s(:lasgn, :a, s(:call, nil, :c, s(:lit, 1))))
        expect(parse('x &&= 1')).must_equal s(:op_asgn_and, s(:lvar, :x), s(:lasgn, :x, s(:lit, 1)))
        expect(parse('x[y] &&= 1')).must_equal s(:op_asgn1, s(:call, nil, :x), s(:arglist, s(:call, nil, :y)), :"&&", s(:lit, 1))
        expect(parse('x ||= 1')).must_equal s(:op_asgn_or, s(:lvar, :x), s(:lasgn, :x, s(:lit, 1)))
        expect(parse('x[y] ||= 1')).must_equal s(:op_asgn1, s(:call, nil, :x), s(:arglist, s(:call, nil, :y)), :"||", s(:lit, 1))
        expect(parse('x += 1')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :+, s(:lit, 1)))
        expect(parse('x -= 1')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :-, s(:lit, 1)))
        expect(parse('x *= 1')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :*, s(:lit, 1)))
        expect(parse('x **= 1')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :**, s(:lit, 1)))
        expect(parse('x /= 1')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :/, s(:lit, 1)))
        expect(parse('x %= 1')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :%, s(:lit, 1)))
        expect(parse('x |= 1')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :|, s(:lit, 1)))
        expect(parse('x &= 1')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :&, s(:lit, 1)))
        expect(parse('x >>= 1')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :>>, s(:lit, 1)))
        expect(parse('x <<= 1')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :<<, s(:lit, 1)))
        expect(parse('x = y += 1')).must_equal s(:lasgn, :x, s(:lasgn, :y, s(:call, s(:lvar, :y), :+, s(:lit, 1))))
        expect(parse("x += y += 1")).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :+, s(:lasgn, :y, s(:call, s(:lvar, :y), :+, s(:lit, 1)))))
        expect(parse("x == y += 1")).must_equal s(:call, s(:call, nil, :x), :==, s(:lasgn, :y, s(:call, s(:lvar, :y), :+, s(:lit, 1))))
        expect(parse("x =~ y += 1")).must_equal s(:call, s(:call, nil, :x), :=~, s(:lasgn, :y, s(:call, s(:lvar, :y), :+, s(:lit, 1))))

        # NOTE: these produce different results in RubyParser, but I don't agree. I'm not sure if it's a bug or just a different choice.
        if parser == 'NatalieParser'
          expect(parse("a.b *= c d")).must_equal s(:op_asgn2, s(:call, nil, :a), :b=, :*, s(:call, nil, :c, s(:call, nil, :d)))
          expect(parse("a&.b *= c d")).must_equal s(:safe_op_asgn2, s(:call, nil, :a), :b=, :*, s(:call, nil, :c, s(:call, nil, :d)))
          expect(parse("::A *= c d")).must_equal s(:op_asgn, s(:colon3, :A), :*, s(:call, nil, :c, s(:call, nil, :d)))
          expect(parse("A::B *= c d")).must_equal s(:op_asgn, s(:colon2, s(:const, :A), :B), :*, s(:call, nil, :c, s(:call, nil, :d)))
          expect(parse("::A &&= 2")).must_equal s(:op_asgn_and, s(:colon3, :A), s(:cdecl, s(:colon3, :A), s(:lit, 2)))
          expect(parse("A::B &&= 2")).must_equal s(:op_asgn_and, s(:colon2, s(:const, :A), :B), s(:cdecl, s(:colon2, s(:const, :A), :B), s(:lit, 2)))
          expect(parse("a.b &&= c 1")).must_equal s(:op_asgn_and, s(:call, s(:call, nil, :a), :b), s(:attrasgn, s(:call, nil, :a), :b=, s(:call, nil, :c, s(:lit, 1))))
          expect(parse("a.b ||= c 1")).must_equal s(:op_asgn_or, s(:call, s(:call, nil, :a), :b), s(:attrasgn, s(:call, nil, :a), :b=, s(:call, nil, :c, s(:lit, 1))))
          expect(parse("a&.b &&= c 1")).must_equal s(:op_asgn_and, s(:safe_call, s(:call, nil, :a), :b), s(:safe_attrasgn, s(:call, nil, :a), :b=, s(:call, nil, :c, s(:lit, 1))))
          expect(parse("a&.b ||= c 1")).must_equal s(:op_asgn_or, s(:safe_call, s(:call, nil, :a), :b), s(:safe_attrasgn, s(:call, nil, :a), :b=, s(:call, nil, :c, s(:lit, 1))))
        end
      end

      it 'parses constants' do
        expect(parse('ARGV')).must_equal s(:const, :ARGV)
        expect(parse('Foo::Bar')).must_equal s(:colon2, s(:const, :Foo), :Bar)
        expect(parse('Foo::Bar::BAZ')).must_equal s(:colon2, s(:colon2, s(:const, :Foo), :Bar), :BAZ)
        expect(parse('x, y = ::Bar')).must_equal s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:colon3, :Bar)))
        expect(parse('Foo::bar')).must_equal s(:call, s(:const, :Foo), :bar)
        expect(parse('Foo::bar = 1 + 2')).must_equal s(:attrasgn, s(:const, :Foo), :bar=, s(:call, s(:lit, 1), :+, s(:lit, 2)))
        expect(parse('Foo::bar x, y')).must_equal s(:call, s(:const, :Foo), :bar, s(:call, nil, :x), s(:call, nil, :y))
        expect(parse('-Foo::BAR')).must_equal s(:call, s(:colon2, s(:const, :Foo), :BAR), :-@)
      end

      it 'parses local variables' do
        expect(parse('ootpüt = 1')).must_equal s(:lasgn, :ootpüt, s(:lit, 1))
      end

      it 'parses global variables' do
        expect(parse('$foo')).must_equal s(:gvar, :$foo)
        expect(parse('$0')).must_equal s(:gvar, :$0)
        expect(parse('$_')).must_equal s(:gvar, :$_)
      end

      it 'parses back refs' do
        expect(parse('$&')).must_equal s(:back_ref, :&)
      end

      it 'parses regexp nth refs' do
        expect(parse('$1')).must_equal s(:nth_ref, 1)
        expect(parse('$9')).must_equal s(:nth_ref, 9)
        expect(parse('$10')).must_equal s(:nth_ref, 10)
        expect(parse('$100')).must_equal s(:nth_ref, 100)
      end

      it 'parses instance variables' do
        expect(parse('@foo')).must_equal s(:ivar, :@foo)
      end

      it 'parses class variables' do
        expect(parse('@@foo')).must_equal s(:cvar, :@@foo)
      end

      it 'parses method calls with parentheses' do
        expect(parse('foo()')).must_equal s(:call, nil, :foo)
        expect(parse("foo ()")).must_equal s(:call, nil, :foo, s(:nil))
        expect(parse('Foo()')).must_equal s(:call, nil, :Foo)
        expect(parse('Foo::Bar()')).must_equal s(:call, s(:const, :Foo), :Bar)
        expect(parse('foo() + bar()')).must_equal s(:call, s(:call, nil, :foo), :+, s(:call, nil, :bar))
        expect(parse("foo(1, 'baz')")).must_equal s(:call, nil, :foo, s(:lit, 1), s(:str, 'baz'))
        expect(parse('foo(a, b)')).must_equal s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b))
        expect(parse('foo.bar(a, b)')).must_equal s(:call, s(:call, nil, :foo), :bar, s(:call, nil, :a), s(:call, nil, :b))
        expect(parse('foo.Bar(a, b)')).must_equal s(:call, s(:call, nil, :foo), :Bar, s(:call, nil, :a), s(:call, nil, :b))
        expect(parse("foo(a,\nb,\n)")).must_equal s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b))
        expect(parse("foo(\n1 + 2  ,\n  'baz'  \n )")).must_equal s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, 'baz'))
        expect(parse('foo(1, a: 2)')).must_equal s(:call, nil, :foo, s(:lit, 1), s(bare_hash_type, s(:lit, :a), s(:lit, 2)))
        expect(parse('foo(1, { a: 2 })')).must_equal s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2)))
        expect(parse('foo(1, { a: 2, :b => 3 })')).must_equal s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2), s(:lit, :b), s(:lit, 3)))
        expect(parse('foo(:a, :b)')).must_equal s(:call, nil, :foo, s(:lit, :a), s(:lit, :b))
        expect(parse('foo(a: 1)')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:lit, 1)))
        expect(parse("foo(0, a: 1, b: 'two')")).must_equal s(:call, nil, :foo, s(:lit, 0), s(bare_hash_type, s(:lit, :a), s(:lit, 1), s(:lit, :b), s(:str, 'two')))
        expect(parse('foo("a": "b")')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:str, "b")))
        expect(parse("foo('a': 'b')")).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:str, "b")))
        expect(parse("foo('a':'b')")).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:str, "b")))
        expect(parse('foo("a#{1+1}": "b")')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:dsym, "a", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1)))), s(:str, "b")))
        expect(parse("foo(bar: 1 && 2)")).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :bar), s(:and, s(:lit, 1), s(:lit, 2))))
        expect(parse("foo(0, 1 => 2, 3 => 4)")).must_equal s(:call, nil, :foo, s(:lit, 0), s(bare_hash_type, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4)))
        expect(parse('foo(a: 1, &b)')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:lit, 1)), s(:block_pass, s(:call, nil, :b)))
        expect(parse('foo(a, *b, c)')).must_equal s(:call, nil, :foo, s(:call, nil, :a), s(:splat, s(:call, nil, :b)), s(:call, nil, :c))
        expect(parse('b=1; foo(a, *b, c)')).must_equal s(:block, s(:lasgn, :b, s(:lit, 1)), s(:call, nil, :foo, s(:call, nil, :a), s(:splat, s(:lvar, :b)), s(:call, nil, :c)))
        expect(parse('foo.()')).must_equal s(:call, s(:call, nil, :foo), :call)
        expect(parse('foo.(1, 2)')).must_equal s(:call, s(:call, nil, :foo), :call, s(:lit, 1), s(:lit, 2))
        expect(parse('foo(a = b, c)')).must_equal s(:call, nil, :foo, s(:lasgn, :a, s(:call, nil, :b)), s(:call, nil, :c))
        expect(parse("foo (1 + 2)")).must_equal s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)))
        expect(parse("a.b (1) {c}")).must_equal s(:iter, s(:call, s(:call, nil, :a), :b, s(:lit, 1)), 0, s(:call, nil, :c))
        expect(parse("a(b + c).d {e}")).must_equal s(:iter, s(:call, s(:call, nil, :a, s(:call, s(:call, nil, :b), :+, s(:call, nil, :c))), :d), 0, s(:call, nil, :e))
        expect(parse("a (b + c).d {e}")).must_equal s(:call, nil, :a, s(:iter, s(:call, s(:call, s(:call, nil, :b), :+, s(:call, nil, :c)), :d), 0, s(:call, nil, :e)))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('foo(') }, SyntaxError, "(string)#1: syntax error, unexpected end-of-input (expected: 'expression')")
          expect_raise_with_message(-> { parse('123(') }, SyntaxError, "(string)#1: syntax error, unexpected '(' (error: 'left-hand-side is not callable')")
        else
          expect_raise_with_message(-> { parse('foo(') }, SyntaxError, '(string):1 :: parse error on value "$" ($end)')
          expect_raise_with_message(-> { parse('123(') }, SyntaxError, "(string):1 :: parse error on value \"(\" (tLPAREN2)")
        end
      end

      it 'parses method calls without parentheses' do
        expect(parse('foo + bar')).must_equal s(:call, s(:call, nil, :foo), :+, s(:call, nil, :bar))
        expect(parse("foo 1, 'baz'")).must_equal s(:call, nil, :foo, s(:lit, 1), s(:str, 'baz'))
        expect(parse('foo 1 + 2')).must_equal s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)))
        expect(parse("foo 1 + 2  ,\n  'baz'")).must_equal s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, 'baz'))
        expect(parse("foo 'foo' + 'bar'  ,\n  2")).must_equal s(:call, nil, :foo, s(:call, s(:str, 'foo'), :+, s(:str, 'bar')), s(:lit, 2))
        expect(parse('foo a, b')).must_equal s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b))
        expect(parse('foo 1, a: 2')).must_equal s(:call, nil, :foo, s(:lit, 1), s(bare_hash_type, s(:lit, :a), s(:lit, 2)))
        expect(parse('foo 1, :a => 2, b: 3')).must_equal s(:call, nil, :foo, s(:lit, 1), s(bare_hash_type, s(:lit, :a), s(:lit, 2), s(:lit, :b), s(:lit, 3)))
        expect(parse('foo 1, { a: 2 }')).must_equal s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2)))
        expect(parse('foo 1, { a: 2 }')).must_equal s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2)))
        expect(parse('foo a: 1')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:lit, 1)))
        expect(parse("foo 0, a: 1, b: 'two'")).must_equal s(:call, nil, :foo, s(:lit, 0), s(bare_hash_type, s(:lit, :a), s(:lit, 1), s(:lit, :b), s(:str, 'two')))
        expect(parse('foo "a": "b"')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:str, "b")))
        expect(parse("foo 'a': 'b'")).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:str, "b")))
        expect(parse("foo 'a':'b'")).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:str, "b")))
        expect(parse('foo "a#{1+1}": "b"')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:dsym, "a", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1)))), s(:str, "b")))
        expect(parse('foo a: 1, &b')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:lit, 1)), s(:block_pass, s(:call, nil, :b)))
        expect(parse('foo :a, :b')).must_equal s(:call, nil, :foo, s(:lit, :a), s(:lit, :b))
        expect(parse('self.class')).must_equal s(:call, s(:self), :class)
        expect(parse('self.begin')).must_equal s(:call, s(:self), :begin)
        expect(parse('self.end')).must_equal s(:call, s(:self), :end)
        expect(parse('describe :enumeratorize, shared: true')).must_equal s(:call, nil, :describe, s(:lit, :enumeratorize), s(bare_hash_type, s(:lit, :shared), s(:true)))
        expect(parse("describe :enumeratorize, shared: true do\nnil\nend")).must_equal s(:iter, s(:call, nil, :describe, s(:lit, :enumeratorize), s(bare_hash_type, s(:lit, :shared), s(:true))), 0, s(:nil))
        expect(parse('foo a = b, c')).must_equal s(:call, nil, :foo, s(:lasgn, :a, s(:call, nil, :b)), s(:call, nil, :c))
        expect(parse('foo a = b if bar')).must_equal s(:if, s(:call, nil, :bar), s(:call, nil, :foo, s(:lasgn, :a, s(:call, nil, :b))), nil)
        expect(parse('foo a && b')).must_equal s(:call, nil, :foo, s(:and, s(:call, nil, :a), s(:call, nil, :b)))
        expect(parse('foo a || b')).must_equal s(:call, nil, :foo, s(:or, s(:call, nil, :a), s(:call, nil, :b)))
        expect(parse('foo a ? b : c')).must_equal s(:call, nil, :foo, s(:if, s(:call, nil, :a), s(:call, nil, :b), s(:call, nil, :c)))
        expect(parse('foo a += 1')).must_equal s(:call, nil, :foo, s(:lasgn, :a, s(:call, s(:lvar, :a), :+, s(:lit, 1))))
        expect(parse('foo a += 1, 2')).must_equal s(:call, nil, :foo, s(:lasgn, :a, s(:call, s(:lvar, :a), :+, s(:lit, 1))), s(:lit, 2))
        expect(parse('foo a and b')).must_equal s(:and, s(:call, nil, :foo, s(:call, nil, :a)), s(:call, nil, :b))
        expect(parse("foo a and b { 1 }")).must_equal s(:and, s(:call, nil, :foo, s(:call, nil, :a)), s(:iter, s(:call, nil, :b), 0, s(:lit, 1)))
        expect(parse("foo a and b do\n1\nend")).must_equal s(:and, s(:call, nil, :foo, s(:call, nil, :a)), s(:iter, s(:call, nil, :b), 0, s(:lit, 1)))
        expect(parse("foo a == b, c")).must_equal s(:call, nil, :foo, s(:call, s(:call, nil, :a), :==, s(:call, nil, :b)), s(:call, nil, :c))
        expect(parse("foo self: a")).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :self), s(:call, nil, :a)))
        expect(parse("foo:bar")).must_equal s(:call, nil, :foo, s(:lit, :bar))
        expect(parse("p foo:bar")).must_equal s(:call, nil, :p, s(bare_hash_type, s(:lit, :foo), s(:call, nil, :bar)))
        expect(parse("p for:bar")).must_equal s(:call, nil, :p, s(bare_hash_type, s(:lit, :for), s(:call, nil, :bar)))
        expect(parse("Foo:bar")).must_equal s(:call, nil, :Foo, s(:lit, :bar))
        expect(parse("p Foo:bar")).must_equal s(:call, nil, :p, s(bare_hash_type, s(:lit, :Foo), s(:call, nil, :bar)))
        expect(parse("p 'foo':'bar'")).must_equal s(:call, nil, :p, s(bare_hash_type, s(:lit, :foo), s(:str, "bar")))
        expect(parse('p "foo":"bar"')).must_equal s(:call, nil, :p, s(bare_hash_type, s(:lit, :foo), s(:str, "bar")))
        expect(parse("p $1")).must_equal s(:call, nil, :p, s(:nth_ref, 1))
        expect(-> { parse("for:bar") }).must_raise SyntaxError # not with a keyword :-)
        expect(parse("Foo::Bar :bar")).must_equal s(:call, s(:const, :Foo), :Bar, s(:lit, :bar))
        expect(parse("Foo::Bar:bar")).must_equal s(:call, s(:const, :Foo), :Bar, s(:lit, :bar))
        expect(parse("p Foo::Bar:bar")).must_equal s(:call, nil, :p, s(:call, s(:const, :Foo), :Bar, s(:lit, :bar)))
        expect(-> { parse("::Bar :bar") }).must_raise SyntaxError # not with colon3
      end

      it 'parses safe calls' do
        expect(parse('foo&.bar')).must_equal s(:safe_call, s(:call, nil, :foo), :bar)
        expect(parse("foo&.\nbar")).must_equal s(:safe_call, s(:call, nil, :foo), :bar)
        expect(-> { parse("foo&\n.bar") }).must_raise SyntaxError
        expect(parse("foo\n&.bar")).must_equal s(:safe_call, s(:call, nil, :foo), :bar)
        expect(parse('foo&.bar 1')).must_equal s(:safe_call, s(:call, nil, :foo), :bar, s(:lit, 1))
        expect(parse('foo&.bar x')).must_equal s(:safe_call, s(:call, nil, :foo), :bar, s(:call, nil, :x))
        expect(parse('foo&.> x')).must_equal s(:safe_call, s(:call, nil, :foo), :>, s(:call, nil, :x))
        expect(parse('foo&.case x')).must_equal s(:safe_call, s(:call, nil, :foo), :case, s(:call, nil, :x))
        expect(parse('foo&.()')).must_equal s(:safe_call, s(:call, nil, :foo), :call)
      end

      it 'parses method calls with a receiver' do
        expect(parse('foo.bar')).must_equal s(:call, s(:call, nil, :foo), :bar)
        expect(parse('foo.bar.baz')).must_equal s(:call, s(:call, s(:call, nil, :foo), :bar), :baz)
        expect(parse('foo.bar 1, 2')).must_equal s(:call, s(:call, nil, :foo), :bar, s(:lit, 1), s(:lit, 2))
        expect(parse('foo.bar(1, 2)')).must_equal s(:call, s(:call, nil, :foo), :bar, s(:lit, 1), s(:lit, 2))
        expect(parse('foo.nil?')).must_equal s(:call, s(:call, nil, :foo), :nil?)
        expect(parse('foo.not?')).must_equal s(:call, s(:call, nil, :foo), :not?)
        expect(parse('foo.baz?')).must_equal s(:call, s(:call, nil, :foo), :baz?)
        expect(parse("foo\n  .bar\n  .baz")).must_equal s(:call, s(:call, s(:call, nil, :foo), :bar), :baz)
        expect(parse("foo.\n  bar.\n  baz")).must_equal s(:call, s(:call, s(:call, nil, :foo), :bar), :baz)
      end

      it 'parses operator method calls' do
        operators = %w[+ - * ** / % == === != =~ !~ > >= < <= <=> & | ^ ~ << >> [] []=]
        operators.each do |operator|
          expect(parse("self.#{operator}")).must_equal s(:call, s(:self), operator.to_sym)
        end
      end

      it 'parses method calls named after a keyword' do
        keywords = %i[
          alias
          and
          begin
          begin
          break
          case
          class
          defined
          def
          do
          else
          elsif
          encoding
          end
          end
          ensure
          false
          file
          for
          if
          in
          line
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
        ].each do |keyword|
          expect(parse("foo.#{keyword}(1)")).must_equal s(:call, s(:call, nil, :foo), keyword, s(:lit, 1))
          expect(parse("foo.#{keyword} 1 ")).must_equal s(:call, s(:call, nil, :foo), keyword, s(:lit, 1))
        end
      end

      it 'parses method calls with explicit hash vs implicit hash (possibly keyword args)' do
        # hash
        expect(parse("foo 1, { a: 'b' }")).must_equal s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:str, "b")))
        expect(parse("foo(1, { a: 'b' })")).must_equal s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:str, "b")))
        expect(parse("foo 1, { :a => 'b' }")).must_equal s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:str, "b")))
        expect(parse("foo(1, { :a => 'b' })")).must_equal s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:str, "b")))
        # bare hash (can be used as keyword args)
        expect(parse("foo 1, a: 'b'")).must_equal s(:call, nil, :foo, s(:lit, 1), s(bare_hash_type, s(:lit, :a), s(:str, "b")))
        expect(parse("foo(1, a: 'b')")).must_equal s(:call, nil, :foo, s(:lit, 1), s(bare_hash_type, s(:lit, :a), s(:str, "b")))
        expect(parse("foo 1, :a => 'b'")).must_equal s(:call, nil, :foo, s(:lit, 1), s(bare_hash_type, s(:lit, :a), s(:str, "b")))
        expect(parse("foo(1, :a => 'b')")).must_equal s(:call, nil, :foo, s(:lit, 1), s(bare_hash_type, s(:lit, :a), s(:str, "b")))
      end

      it 'parses ternary expressions' do
        expect(parse('1 ? 2 : 3')).must_equal s(:if, s(:lit, 1), s(:lit, 2), s(:lit, 3))
        expect(parse("foo ?\nbar + baz\n :\n buz / 2")).must_equal s(:if, s(:call, nil, :foo), s(:call, s(:call, nil, :bar), :+, s(:call, nil, :baz)), s(:call, s(:call, nil, :buz), :/, s(:lit, 2)))
        expect(parse('1 ? 2 : map { |n| n }')).must_equal s(:if, s(:lit, 1), s(:lit, 2), s(:iter, s(:call, nil, :map), s(:args, :n), s(:lvar, :n)))
        expect(parse("1 ? 2 : map do |n|\nn\nend")).must_equal s(:if, s(:lit, 1), s(:lit, 2), s(:iter, s(:call, nil, :map), s(:args, :n), s(:lvar, :n)))
        expect(parse('fib(num ? num.to_i : 25)')).must_equal s(:call, nil, :fib, s(:if, s(:call, nil, :num), s(:call, s(:call, nil, :num), :to_i), s(:lit, 25)))
        expect(parse('foo x < 1 ? x : y')).must_equal s(:call, nil, :foo, s(:if, s(:call, s(:call, nil, :x), :<, s(:lit, 1)), s(:call, nil, :x), s(:call, nil, :y)))
        expect(parse('return x < 1 ? x : y')).must_equal s(:return, s(:if, s(:call, s(:call, nil, :x), :<, s(:lit, 1)), s(:call, nil, :x), s(:call, nil, :y)))
        expect(parse('foo ? bar = 1 : 2')).must_equal s(:if, s(:call, nil, :foo), s(:lasgn, :bar, s(:lit, 1)), s(:lit, 2))
        expect(parse('foo ? 1 : bar = 2')).must_equal s(:if, s(:call, nil, :foo), s(:lit, 1), s(:lasgn, :bar, s(:lit, 2)))
        expect(parse("foo ? 'bar' : 'baz'")).must_equal s(:if, s(:call, nil, :foo), s(:str, "bar"), s(:str, "baz"))
        expect(parse("foo && bar && baz ? 1 : 2")).must_equal s(:if, s(:and, s(:call, nil, :foo), s(:and, s(:call, nil, :bar), s(:call, nil, :baz))), s(:lit, 1), s(:lit, 2))
        expect(parse("a ? '': b")).must_equal s(:if, s(:call, nil, :a), s(:str, ""), s(:call, nil, :b))
        expect(parse('a ? "": b')).must_equal s(:if, s(:call, nil, :a), s(:str, ""), s(:call, nil, :b))
        expect(parse('a=b ? true: false')).must_equal s(:lasgn, :a, s(:if, s(:call, nil, :b), s(:true), s(:false)))
      end

      it 'parses if/elsif/else' do
        expect(parse('if true; end')).must_equal s(:if, s(:true), nil, nil)
        expect(parse('if true; 1; end')).must_equal s(:if, s(:true), s(:lit, 1), nil)
        expect(parse('if true; 1; 2; end')).must_equal s(:if, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), nil)
        expect(parse('if false; 1; else; 2; end')).must_equal s(:if, s(:false), s(:lit, 1), s(:lit, 2))
        expect(parse('if false; 1; elsif 1 + 1 == 2; 2; else; 3; end')).must_equal s(:if, s(:false), s(:lit, 1), s(:if, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 1)), :==, s(:lit, 2)), s(:lit, 2), s(:lit, 3)))
        expect(parse("if false; 1; elsif 1 + 1 == 0; 2; 3; elsif false; 4; elsif foo() == 'bar'; 5; 6; else; 7; end")).must_equal s(:if, s(:false), s(:lit, 1), s(:if, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 1)), :==, s(:lit, 0)), s(:block, s(:lit, 2), s(:lit, 3)), s(:if, s(:false), s(:lit, 4), s(:if, s(:call, s(:call, nil, :foo), :==, s(:str, 'bar')), s(:block, s(:lit, 5), s(:lit, 6)), s(:lit, 7)))))
        expect(parse("if true then 'foo'\nend")).must_equal s(:if, s(:true), s(:str, "foo"), nil)
        expect(parse("if true then 'foo' else 'bar'\nend")).must_equal s(:if, s(:true), s(:str, "foo"), s(:str, "bar"))
      end

      it 'parses unless' do
        expect(parse('unless false; 1; else; 2; end')).must_equal s(:if, s(:false), s(:lit, 2), s(:lit, 1))
      end

      it 'parses while/until' do
        expect(parse('while true; end')).must_equal s(:while, s(:true), nil, true)
        expect(parse('while true; 1; end')).must_equal s(:while, s(:true), s(:lit, 1), true)
        expect(parse('while true do end')).must_equal s(:while, s(:true), nil, true)
        expect(parse('while true do 1; end')).must_equal s(:while, s(:true), s(:lit, 1), true)
        expect(parse('while true; 1; 2; end')).must_equal s(:while, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), true)
        expect(parse('until true; end')).must_equal s(:until, s(:true), nil, true)
        expect(parse('until true; 1; end')).must_equal s(:until, s(:true), s(:lit, 1), true)
        expect(parse('until true do end')).must_equal s(:until, s(:true), nil, true)
        expect(parse('until true do 1; end')).must_equal s(:until, s(:true), s(:lit, 1), true)
        expect(parse('until true; 1; 2; end')).must_equal s(:until, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), true)
        expect(parse('foo while true')).must_equal s(:while, s(:true), s(:call, nil, :foo), true)
        expect(parse('begin; foo; end while true')).must_equal s(:while, s(:true), s(:call, nil, :foo), false)
        expect(parse('1 while true')).must_equal s(:while, s(:true), s(:lit, 1), true)
        expect(parse('begin; 1; end while true')).must_equal s(:while, s(:true), s(:lit, 1), false)
        expect(parse('begin; 1; 2; end while true')).must_equal s(:while, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), false)
        expect(parse('begin; 1; rescue; 2; end while true')).must_equal s(:while, s(:true), s(:rescue, s(:lit, 1), s(:resbody, s(:array), s(:lit, 2))), false)
        expect(parse('begin; 1; ensure; 2; end while true')).must_equal s(:while, s(:true), s(:ensure, s(:lit, 1), s(:lit, 2)), false)
        expect(parse('begin; 1; end until true')).must_equal s(:until, s(:true), s(:lit, 1), false)
        expect(parse('begin; 1; 2; end until true')).must_equal s(:until, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), false)
        expect(parse('begin; 1; rescue; 2; end until true')).must_equal s(:until, s(:true), s(:rescue, s(:lit, 1), s(:resbody, s(:array), s(:lit, 2))), false)
        expect(parse('begin; 1; ensure; 2; end until true')).must_equal s(:until, s(:true), s(:ensure, s(:lit, 1), s(:lit, 2)), false)
        expect(parse('x = 10; x -= 1 until x.zero?')).must_equal s(:block, s(:lasgn, :x, s(:lit, 10)), s(:until, s(:call, s(:lvar, :x), :zero?), s(:lasgn, :x, s(:call, s(:lvar, :x), :-, s(:lit, 1))), true))
        expect(parse('x = 10; x -= 1 while x.positive?')).must_equal s(:block, s(:lasgn, :x, s(:lit, 10)), s(:while, s(:call, s(:lvar, :x), :positive?), s(:lasgn, :x, s(:call, s(:lvar, :x), :-, s(:lit, 1))), true))
      end

      it 'parses for' do
        expect(parse('for foo in bar; end')).must_equal s(:for, s(:call, nil, :bar), s(:lasgn, :foo))
        expect(parse('for foo in bar; 1; end')).must_equal s(:for, s(:call, nil, :bar), s(:lasgn, :foo), s(:lit, 1))
        expect(parse('for foo in bar do end')).must_equal s(:for, s(:call, nil, :bar), s(:lasgn, :foo))
        expect(parse('for foo in bar do 1; end')).must_equal s(:for, s(:call, nil, :bar), s(:lasgn, :foo), s(:lit, 1))
        # FIXME: support do keyword
        # expect(parse('for foo in bar do; end')).must_equal s(:for, s(:call, nil, :bar), s(:lasgn, :foo))
        expect(parse('for a, b in c; end')).must_equal s(:for, s(:call, nil, :c), s(:masgn, s(:array, s(:lasgn, :a), s(:lasgn, :b))))
      end

      it 'parses post-conditional if/unless' do
        expect(parse('true if true')).must_equal s(:if, s(:true), s(:true), nil)
        expect(parse('true unless true')).must_equal s(:if, s(:true), nil, s(:true))
        expect(parse("foo 'hi' if true")).must_equal s(:if, s(:true), s(:call, nil, :foo, s(:str, 'hi')), nil)
        expect(parse("foo.bar 'hi' if true")).must_equal s(:if, s(:true), s(:call, s(:call, nil, :foo), :bar, s(:str, 'hi')), nil)
      end

      it 'parses true/false/nil' do
        expect(parse('true')).must_equal s(:true)
        expect(parse('false')).must_equal s(:false)
        expect(parse('nil')).must_equal s(:nil)
      end

      it 'parses class definition' do
        expect(parse("class Foo\nend")).must_equal s(:class, :Foo, nil)
        expect(parse('class Foo;end')).must_equal s(:class, :Foo, nil)
        expect(parse('class FooBar; 1; 2; end')).must_equal s(:class, :FooBar, nil, s(:lit, 1), s(:lit, 2))
        expect_raise_with_message(-> { parse('class foo;end') }, SyntaxError, 'class/module name must be CONSTANT')
        expect(parse("class Foo < Bar; 3\n 4\n end")).must_equal s(:class, :Foo, s(:const, :Bar), s(:lit, 3), s(:lit, 4))
        expect(parse("class Foo < bar; 3\n 4\n end")).must_equal s(:class, :Foo, s(:call, nil, :bar), s(:lit, 3), s(:lit, 4))
        expect(parse('class Foo::Bar; end')).must_equal s(:class, s(:colon2, s(:const, :Foo), :Bar), nil)
        expect(parse('class foo::Bar; end')).must_equal s(:class, s(:colon2, s(:call, nil, :foo), :Bar), nil)
        expect(parse('class ::Foo; end')).must_equal s(:class, s(:colon3, :Foo), nil)
      end

      it 'parses class << self' do
        expect(parse('class Foo; class << self; end; end')).must_equal s(:class, :Foo, nil, s(:sclass, s(:self)))
        expect(parse('class Foo; class << Bar; 1; end; end')).must_equal s(:class, :Foo, nil, s(:sclass, s(:const, :Bar), s(:lit, 1)))
        expect(parse('class Foo; class << (1 + 1); 1; 2; end; end')).must_equal s(:class, :Foo, nil, s(:sclass, s(:call, s(:lit, 1), :+, s(:lit, 1)), s(:lit, 1), s(:lit, 2)))
      end

      it 'parses module definition' do
        expect(parse("module Foo\nend")).must_equal s(:module, :Foo)
        expect(parse('module Foo;end')).must_equal s(:module, :Foo)
        expect(parse('module FooBar; 1; 2; end')).must_equal s(:module, :FooBar, s(:lit, 1), s(:lit, 2))
        expect_raise_with_message(-> { parse('module foo;end') }, SyntaxError, 'class/module name must be CONSTANT')
        expect(parse('module Foo::Bar; end')).must_equal s(:module, s(:colon2, s(:const, :Foo), :Bar))
        expect(parse('module foo::Bar; end')).must_equal s(:module, s(:colon2, s(:call, nil, :foo), :Bar))
        expect(parse('module ::Foo; end')).must_equal s(:module, s(:colon3, :Foo))
      end

      it 'parses array' do
        expect(parse('[]')).must_equal s(:array)
        expect(parse('[1]')).must_equal s(:array, s(:lit, 1))
        expect(parse('[1,]')).must_equal s(:array, s(:lit, 1))
        expect(parse("['foo']")).must_equal s(:array, s(:str, 'foo'))
        expect(parse('[1, 2, 3]')).must_equal s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3))
        expect(parse('[1, (), 2, 3, ]')).must_equal s(:array, s(:lit, 1), s(:nil), s(:lit, 2), s(:lit, 3))
        expect(parse('[x, y, z]')).must_equal s(:array, s(:call, nil, :x), s(:call, nil, :y), s(:call, nil, :z))
        expect(parse("[\n1 , \n2,\n 3]")).must_equal s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3))
        expect(parse("[\n1 , \n2,\n 3\n]")).must_equal s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3))
        expect(parse("[\n1 , \n2,\n 3,\n]")).must_equal s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3))
        expect(parse('[a = foo, b]')).must_equal s(:array, s(:lasgn, :a, s(:call, nil, :foo)), s(:call, nil, :b))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('[ , 1]') }, SyntaxError, "(string)#1: syntax error, unexpected ',' (expected: 'expression')")
        else
          expect_raise_with_message(-> { parse('[ , 1]') }, SyntaxError, '(string):1 :: parse error on value "," (tCOMMA)')
        end
      end

      it 'parses word array' do
        expect(parse('%w[]')).must_equal s(:array)
        expect(parse('%w|1 2 3|')).must_equal s(:array, s(:str, '1'), s(:str, '2'), s(:str, '3'))
        expect(parse("%w[  1 2\t  3\n \n4 ]")).must_equal s(:array, s(:str, '1'), s(:str, '2'), s(:str, '3'), s(:str, '4'))
        expect(parse("%W[  1 2\t  3\n \n4 ]")).must_equal s(:array, s(:str, '1'), s(:str, '2'), s(:str, '3'), s(:str, '4'))
        expect(parse("%W[\#{1+1}]")).must_equal s(:array, s(:dstr, "", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1)))))
        expect(parse("%w(\#{1+1})")).must_equal s(:array, s(:str, "\#{1+1}"))
        expect(parse('%i[ foo#{1} bar ]')).must_equal s(:array, s(:lit, :"foo\#{1}"), s(:lit, :bar))
        expect(parse('%I[ foo#{1} bar ]')).must_equal s(:array, s(:dsym, "foo", s(:evstr, s(:lit, 1))), s(:lit, :bar))
        expect(parse("%w[1\\ 2]")).must_equal s(:array, s(:str, "1 2"))
        expect(parse("%W[1\\n2]")).must_equal s(:array, s(:str, "1\n2"))
        expect(parse("%w[1\\n2]")).must_equal s(:array, s(:str, "1\\n2"))
        expect(parse("%w[1\\\n2]")).must_equal s(:array, s(:str, "1\n2"))
        expect(parse("%W(1\\t2)")).must_equal s(:array, s(:str, "1\t2"))
      end

      it 'parses hash' do
        expect(parse('{}')).must_equal s(:hash)
        expect(parse('{ 1 => 2 }')).must_equal s(:hash, s(:lit, 1), s(:lit, 2))
        expect(parse('{ 1 => 2, }')).must_equal s(:hash, s(:lit, 1), s(:lit, 2))
        expect(parse("{\n 1 => 2,\n }")).must_equal s(:hash, s(:lit, 1), s(:lit, 2))
        expect(parse('foo :a => 1')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:lit, 1)))
        expect(parse('foo :a=>1')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:lit, 1)))
        expect(parse("{ foo: 'bar' }")).must_equal s(:hash, s(:lit, :foo), s(:str, 'bar'))
        expect(parse("{ 1 => 2, 'foo' => 'bar' }")).must_equal s(:hash, s(:lit, 1), s(:lit, 2), s(:str, 'foo'), s(:str, 'bar'))
        expect(parse("{\n 1 => \n2,\n 'foo' =>\n'bar'\n}")).must_equal s(:hash, s(:lit, 1), s(:lit, 2), s(:str, 'foo'), s(:str, 'bar'))
        expect(parse("{ foo: 'bar', baz: 'buz' }")).must_equal s(:hash, s(:lit, :foo), s(:str, 'bar'), s(:lit, :baz), s(:str, 'buz'))
        expect(parse('{ a => b, c => d }')).must_equal s(:hash, s(:call, nil, :a), s(:call, nil, :b), s(:call, nil, :c), s(:call, nil, :d))
        expect(parse("{ 'a': 'b' }")).must_equal s(:hash, s(:lit, :a), s(:str, "b"))
        expect(parse('{ "a": "b" }')).must_equal s(:hash, s(:lit, :a), s(:str, "b"))
        expect(parse('{ a: -1, b: 2 }')).must_equal s(:hash, s(:lit, :a), s(:lit, -1), s(:lit, :b), s(:lit, 2))
        expect(parse('{ "a#{1+1}": "b" }')).must_equal s(:hash, s(:dsym, "a", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1)))), s(:str, "b"))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('{ , 1 => 2 }') }, SyntaxError, "(string)#1: syntax error, unexpected ',' (expected: 'expression')")
        else
          expect_raise_with_message(-> { parse('{ , 1 => 2 }') }, SyntaxError, '(string):1 :: parse error on value "," (tCOMMA)')
        end
        expect(parse('[1 => 2]')).must_equal s(:array, s(bare_hash_type, s(:lit, 1), s(:lit, 2)))
        expect(parse('[1 => 2,]')).must_equal s(:array, s(bare_hash_type, s(:lit, 1), s(:lit, 2)))
        expect(parse('[0, 1 => 2]')).must_equal s(:array, s(:lit, 0), s(bare_hash_type, s(:lit, 1), s(:lit, 2)))
        expect(parse('[0, foo: "bar"]')).must_equal s(:array, s(:lit, 0), s(bare_hash_type, s(:lit, :foo), s(:str, 'bar')))
        expect(parse('["foo" => :bar, baz: 42]')).must_equal s(:array, s(bare_hash_type, s(:str, "foo"), s(:lit, :bar), s(:lit, :baz), s(:lit, 42)))
        expect(parse('bar[1 => 2]')).must_equal s(:call, s(:call, nil, :bar), :[], s(bare_hash_type, s(:lit, 1), s(:lit, 2)))
        expect(parse('Foo::Bar[1 => 2]')).must_equal s(:call, s(:colon2, s(:const, :Foo), :Bar), :[], s(bare_hash_type, s(:lit, 1), s(:lit, 2)))
        expect(parse('Hash[a: 1, b: 2]')).must_equal s(:call, s(:const, :Hash), :[], s(bare_hash_type, s(:lit, :a), s(:lit, 1), s(:lit, :b), s(:lit, 2)))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('[1 => 2, 3]') }, SyntaxError, "(string)#1: syntax error, unexpected ']' (expected: 'hash rocket')")
          expect_raise_with_message(-> { parse('[0, 1 => 2, 3]') }, SyntaxError, "(string)#1: syntax error, unexpected ']' (expected: 'hash rocket')")
        else
          expect_raise_with_message(-> { parse('[1 => 2, 3]') }, SyntaxError, '(string):1 :: parse error on value "]" (tRBRACK)')
          expect_raise_with_message(-> { parse('[0, 1 => 2, 3]') }, SyntaxError, '(string):1 :: parse error on value "]" (tRBRACK)')
        end
        expect(parse('{a: a = 1, b: 2}')).must_equal s(:hash, s(:lit, :a), s(:lasgn, :a, s(:lit, 1)), s(:lit, :b), s(:lit, 2))
      end

      it 'ignores comments' do
        if parser == 'NatalieParser'
          expect(parse('# comment')).must_equal s(:block)
          expect(parse("# comment\n#comment 2")).must_equal s(:block)
        else
          expect(parse('# comment')).must_be_nil
          expect(parse("# comment\n#comment 2")).must_be_nil
        end
        expect(parse('1 + 1 # comment')).must_equal s(:call, s(:lit, 1), :+, s(:lit, 1))
        node = parse("# comment 1\n# comment 2\n\nfoo\n")
        expect(node.line).must_equal(4)
      end

      it 'parses range' do
        expect(parse('1..10')).must_equal s(:lit, 1..10)
        expect(parse("1..'a'")).must_equal s(:dot2, s(:lit, 1), s(:str, 'a'))
        expect(parse("'a'..1")).must_equal s(:dot2, s(:str, 'a'), s(:lit, 1))
        expect(parse('1...10')).must_equal s(:lit, 1...10)
        expect(parse('1.1..2.2')).must_equal s(:dot2, s(:lit, 1.1), s(:lit, 2.2))
        expect(parse('1.1...2.2')).must_equal s(:dot3, s(:lit, 1.1), s(:lit, 2.2))
        expect(parse("'a'..'z'")).must_equal s(:dot2, s(:str, 'a'), s(:str, 'z'))
        expect(parse("'a'...'z'")).must_equal s(:dot3, s(:str, 'a'), s(:str, 'z'))
        expect(parse("('a')..('z')")).must_equal s(:dot2, s(:str, 'a'), s(:str, 'z'))
        expect(parse("('a')...('z')")).must_equal s(:dot3, s(:str, 'a'), s(:str, 'z'))
        expect(parse('foo..bar')).must_equal s(:dot2, s(:call, nil, :foo), s(:call, nil, :bar))
        expect(parse('foo...bar')).must_equal s(:dot3, s(:call, nil, :foo), s(:call, nil, :bar))
        expect(parse('foo = 1..10')).must_equal s(:lasgn, :foo, s(:lit, 1..10))
        expect(parse('..3')).must_equal s(:dot2, nil, s(:lit, 3))
        expect(parse('(..3)')).must_equal s(:dot2, nil, s(:lit, 3))
        expect(parse('x = ...3')).must_equal s(:lasgn, :x, s(:dot3, nil, s(:lit, 3)))
        expect(parse('4..')).must_equal s(:dot2, s(:lit, 4), nil)
        expect(parse('4..nil')).must_equal s(:dot2, s(:lit, 4), s(:nil))
        expect(parse('(4..)')).must_equal s(:dot2, s(:lit, 4), nil)
        expect(parse("4..\n5")).must_equal s(:lit, 4..5)
        expect(parse("4..\nfoo")).must_equal s(:dot2, s(:lit, 4), s(:call, nil, :foo))
        expect(parse('(4..) * 5')).must_equal s(:call, s(:dot2, s(:lit, 4), nil), :*, s(:lit, 5))
        expect(parse('x = (4..)')).must_equal s(:lasgn, :x, s(:dot2, s(:lit, 4), nil))
        expect(parse("case 1\nwhen 1\n2..\nwhen 2\n3...\nwhen 4\n5..\nend")).must_equal s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:dot2, s(:lit, 2), nil)), s(:when, s(:array, s(:lit, 2)), s(:dot3, s(:lit, 3), nil)), s(:when, s(:array, s(:lit, 4)), s(:dot2, s(:lit, 5), nil)), nil)
        expect(parse("case 1\nwhen 1..2 then 1\nwhen 3.. then 3\nend")).must_equal s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1..2)), s(:lit, 1)), s(:when, s(:array, s(:dot2, s(:lit, 3), nil)), s(:lit, 3)), nil)
        expect(parse("ruby_version_is ''...'3.0' do\nend")).must_equal s(:iter, s(:call, nil, :ruby_version_is, s(:dot3, s(:str, ''), s(:str, '3.0'))), 0)
      end

      it 'parses argument forwarding shorthand' do
        expect(parse("def foo(...); end")).must_equal s(:defn, :foo, s(:args, s(:forward_args)), s(:nil))
        expect(parse("def foo(...); bar(...); end")).must_equal s(:defn, :foo, s(:args, s(:forward_args)), s(:call, nil, :bar, s(:forward_args)))
        expect(parse("def foo(a, ...); bar(a, ...); end")).must_equal s(:defn, :foo, s(:args, :a, s(:forward_args)), s(:call, nil, :bar, s(:lvar, :a), s(:forward_args)))
        expect(parse("-> (...) { bar(...) }")).must_equal s(:iter, s(:lambda), s(:args, s(:forward_args)), s(:call, nil, :bar, s(:forward_args)))
        expect(parse("-> (a, ...) { bar(a, ...) }")).must_equal s(:iter, s(:lambda), s(:args, :a, s(:forward_args)), s(:call, nil, :bar, s(:lvar, :a), s(:forward_args)))
        expect(-> { parse("def foo(..., a); bar(...); end") }).must_raise SyntaxError
        expect(-> { parse("def foo(a); bar(...); end") }).must_raise SyntaxError
        expect(-> { parse("bar(...)") }).must_raise SyntaxError
        expect(-> { parse("foo { |...| bar(...) }") }).must_raise SyntaxError
        expect(-> { parse("foo { |a, ...| bar(a, ...) }") }).must_raise SyntaxError
        expect(-> { parse("def foo(...); a = ...; end") }).must_raise SyntaxError

        # it's a trap! (don't confuse with range)
        expect(parse("def foo(...); bar(1...2); end")).must_equal s(:defn, :foo, s(:args, s(:forward_args)), s(:call, nil, :bar, s(:lit, 1...2)))
        expect(parse("def foo(...); bar(1...); end")).must_equal s(:defn, :foo, s(:args, s(:forward_args)), s(:call, nil, :bar, s(:dot3, s(:lit, 1), nil)))
        expect(parse("def foo(...); bar(...2); end")).must_equal s(:defn, :foo, s(:args, s(:forward_args)), s(:call, nil, :bar, s(:dot3, nil, s(:lit, 2))))
      end

      it 'parses return' do
        expect(parse('return')).must_equal s(:return)
        expect(parse('return foo')).must_equal s(:return, s(:call, nil, :foo))
        expect(parse('return 1, 2')).must_equal s(:return, s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('return foo if true')).must_equal s(:if, s(:true), s(:return, s(:call, nil, :foo)), nil)
        expect(parse('return x = 1')).must_equal s(:return, s(:lasgn, :x, s(:lit, 1)))
        expect(parse('return :x => 1')).must_equal s(:return, s(bare_hash_type, s(:lit, :x), s(:lit, 1)))
        expect(parse('return 1, :x => 2')).must_equal s(:return, s(:array, s(:lit, 1), s(bare_hash_type, s(:lit, :x), s(:lit, 2))))
        expect(parse('return 1, x: 2')).must_equal s(:return, s(:array, s(:lit, 1), s(bare_hash_type, s(:lit, :x), s(:lit, 2))))
        # why?
        expect(-> { parse('return x: 1') }).must_raise SyntaxError
      end

      it 'parses block iter' do
        expect(parse("foo do\nend")).must_equal s(:iter, s(:call, nil, :foo), 0)
        expect(parse("foo do\n1\n2\nend")).must_equal s(:iter, s(:call, nil, :foo), 0, s(:block, s(:lit, 1), s(:lit, 2)))
        expect(parse("foo do |x, y|\nx\ny\nend")).must_equal s(:iter, s(:call, nil, :foo), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y)))
        expect(parse("foo do ||\nend")).must_equal s(:iter, s(:call, nil, :foo), s(:args))
        expect(parse("foo do | |\nend")).must_equal s(:iter, s(:call, nil, :foo), s(:args))
        expect(parse("foo(a, b) do |x, y|\nx\ny\nend")).must_equal s(:iter, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y)))
        expect(parse("super do |x| x end")).must_equal s(:iter, s(:zsuper), s(:args, :x), s(:lvar, :x))
        expect(parse("super() do |x| x end")).must_equal s(:iter, s(:super), s(:args, :x), s(:lvar, :x))
        expect(parse("super(a, b) do |x| x end")).must_equal s(:iter, s(:super, s(:call, nil, :a), s(:call, nil, :b)), s(:args, :x), s(:lvar, :x))
        expect(parse('foo { }')).must_equal s(:iter, s(:call, nil, :foo), 0)
        expect(parse('foo { 1; 2 }')).must_equal s(:iter, s(:call, nil, :foo), 0, s(:block, s(:lit, 1), s(:lit, 2)))
        expect(parse('foo { |x, y| x; y }')).must_equal s(:iter, s(:call, nil, :foo), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y)))
        expect(parse('foo { |x| x }; x')).must_equal s(:block, s(:iter, s(:call, nil, :foo), s(:args, :x), s(:lvar, :x)), s(:call, nil, :x))
        expect(parse('foo { || }')).must_equal s(:iter, s(:call, nil, :foo), s(:args))
        expect(parse('x = 1; foo { x }; x')).must_equal s(:block, s(:lasgn, :x, s(:lit, 1)), s(:iter, s(:call, nil, :foo), 0, s(:lvar, :x)), s(:lvar, :x))
        expect(parse('super { |x| x }; x')).must_equal s(:block, s(:iter, s(:zsuper), s(:args, :x), s(:lvar, :x)), s(:call, nil, :x))
        expect(parse('super() { |x| x }; x')).must_equal s(:block, s(:iter, s(:super), s(:args, :x), s(:lvar, :x)), s(:call, nil, :x))
        expect(parse('super(a) { |x| x }; x')).must_equal s(:block, s(:iter, s(:super, s(:call, nil, :a)), s(:args, :x), s(:lvar, :x)), s(:call, nil, :x))
        expect(parse("foo do\nbar do\nend\nend")).must_equal s(:iter, s(:call, nil, :foo), 0, s(:iter, s(:call, nil, :bar), 0))
        expect(parse("foo do |(x, y), z|\nend")).must_equal s(:iter, s(:call, nil, :foo), s(:args, s(:masgn, :x, :y), :z))
        expect(parse("foo do |(x, (y, z))|\nend")).must_equal s(:iter, s(:call, nil, :foo), s(:args, s(:masgn, :x, s(:masgn, :y, :z))))
        expect(parse('x = foo.bar { |y| y }')).must_equal s(:lasgn, :x, s(:iter, s(:call, s(:call, nil, :foo), :bar), s(:args, :y), s(:lvar, :y)))
        expect(parse('bar { |*| 1 }')).must_equal s(:iter, s(:call, nil, :bar), s(:args, :*), s(:lit, 1))
        expect(parse('bar { |*x| x }')).must_equal s(:iter, s(:call, nil, :bar), s(:args, :'*x'), s(:lvar, :x))
        expect(parse('bar { |x, *y, z| y }')).must_equal s(:iter, s(:call, nil, :bar), s(:args, :x, :'*y', :z), s(:lvar, :y))
        expect(parse('bar { |a = nil, b = foo, c = FOO| b }')).must_equal s(:iter, s(:call, nil, :bar), s(:args, s(:lasgn, :a, s(:nil)), s(:lasgn, :b, s(:call, nil, :foo)), s(:lasgn, :c, s(:const, :FOO))), s(:lvar, :b))
        expect(parse('bar { |a, b: :c, d:| a }')).must_equal s(:iter, s(:call, nil, :bar), s(:args, :a, s(:kwarg, :b, s(:lit, :c)), s(:kwarg, :d)), s(:lvar, :a))
        expect(parse("get 'foo', bar { 'baz' }")).must_equal s(:call, nil, :get, s(:str, 'foo'), s(:iter, s(:call, nil, :bar), 0, s(:str, 'baz')))
        expect(parse("get 'foo', bar do\n'baz'\nend")).must_equal s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz'))
        expect(parse("foo1 = foo do\n'foo'\nend")).must_equal s(:lasgn, :foo1, s(:iter, s(:call, nil, :foo), 0, s(:str, 'foo')))
        expect(parse("@foo = foo do\n'foo'\nend")).must_equal s(:iasgn, :@foo, s(:iter, s(:call, nil, :foo), 0, s(:str, 'foo')))
        expect(parse("@foo ||= foo do\n'foo'\nend")).must_equal s(:op_asgn_or, s(:ivar, :@foo), s(:iasgn, :@foo, s(:iter, s(:call, nil, :foo), 0, s(:str, 'foo'))))
        expect(parse("get 'foo', bar do 'baz'\n end")).must_equal s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz'))
        expect(parse("get 'foo', bar do\n 'baz' end")).must_equal s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz'))
        expect(parse("get 'foo', bar do 'baz' end")).must_equal s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz'))
        expect(parse("foo += bar { |x| x }")).must_equal s(:lasgn, :foo, s(:call, s(:lvar, :foo), :+, s(:iter, s(:call, nil, :bar), s(:args, :x), s(:lvar, :x))))
        expect(parse('bar { |a, | a }')).must_equal s(:iter, s(:call, nil, :bar), s(:args, :a, nil), s(:lvar, :a))
        expect(parse('foo bar { 1 }')).must_equal s(:call, nil, :foo, s(:iter, s(:call, nil, :bar), 0, s(:lit, 1)))
        expect(parse('foo == bar { 1 }')).must_equal s(:call, s(:call, nil, :foo), :==, s(:iter, s(:call, nil, :bar), 0, s(:lit, 1)))
        expect(parse('foo { 1 } == bar { 2 }')).must_equal s(:call, s(:iter, s(:call, nil, :foo), 0, s(:lit, 1)), :==, s(:iter, s(:call, nil, :bar), 0, s(:lit, 2)))
        expect(parse("foo bar do\n 1\n end")).must_equal s(:iter, s(:call, nil, :foo, s(:call, nil, :bar)), 0, s(:lit, 1))
        expect(parse("foo bar, baz do\n 1\n end")).must_equal s(:iter, s(:call, nil, :foo, s(:call, nil, :bar), s(:call, nil, :baz)), 0, s(:lit, 1))
        expect(parse("foo == bar do\n 1\n end")).must_equal s(:call, s(:call, nil, :foo), :==, s(:iter, s(:call, nil, :bar), 0, s(:lit, 1)))
        expect(parse("foo { 1 } == bar do\n 2 \n end")).must_equal s(:call, s(:iter, s(:call, nil, :foo), 0, s(:lit, 1)), :==, s(:iter, s(:call, nil, :bar), 0, s(:lit, 2)))
        expect(parse("foo * bar do\n 1\n end")).must_equal s(:call, s(:call, nil, :foo), :*, s(:iter, s(:call, nil, :bar), 0, s(:lit, 1)))
        expect(parse("foo + bar do\n 1\n end")).must_equal s(:call, s(:call, nil, :foo), :+, s(:iter, s(:call, nil, :bar), 0, s(:lit, 1)))
        expect(parse("foo += bar do\n 1\n end")).must_equal s(:lasgn, :foo, s(:call, s(:lvar, :foo), :+, s(:iter, s(:call, nil, :bar), 0, s(:lit, 1))))
        expect(parse("foo || bar do\n 1\n end")).must_equal s(:or, s(:call, nil, :foo), s(:iter, s(:call, nil, :bar), 0, s(:lit, 1)))
        expect(parse("foo && bar do\n 1\n end")).must_equal s(:and, s(:call, nil, :foo), s(:iter, s(:call, nil, :bar), 0, s(:lit, 1)))
        expect(parse("foo 1 && bar do\n 1\n end")).must_equal s(:iter, s(:call, nil, :foo, s(:and, s(:lit, 1), s(:call, nil, :bar))), 0, s(:lit, 1))
        expect(parse("foo 1, 2 || bar do\n 1\n end")).must_equal s(:iter, s(:call, nil, :foo, s(:lit, 1), s(:or, s(:lit, 2), s(:call, nil, :bar))), 0, s(:lit, 1))
        expect(parse("not foo do\n 1\n end")).must_equal s(:call, s(:iter, s(:call, nil, :foo), 0, s(:lit, 1)), :!)
        expect(parse("foo 1 < 2 do\n3\nend")).must_equal s(:iter, s(:call, nil, :foo, s(:call, s(:lit, 1), :<, s(:lit, 2))), 0, s(:lit, 3))
        expect(parse("-> *a, b { 1 }")).must_equal s(:iter, s(:lambda), s(:args, :"*a", :b), s(:lit, 1))
        expect(parse("-> **foo { 1 }")).must_equal s(:iter, s(:lambda), s(:args, :"**foo"), s(:lit, 1))
        expect(parse("-> &foo { 1 }")).must_equal s(:iter, s(:lambda), s(:args, :"&foo"), s(:lit, 1))
        expect(parse("-> foo = bar { 1 }")).must_equal s(:iter, s(:lambda), s(:args, s(:lasgn, :foo, s(:call, nil, :bar))), s(:lit, 1))
        expect(parse("-> foo: bar { 1 }")).must_equal s(:iter, s(:lambda), s(:args, s(:kwarg, :foo, s(:call, nil, :bar))), s(:lit, 1))
        expect(parse("-> foo: { 1 }")).must_equal s(:iter, s(:lambda), s(:args, s(:kwarg, :foo)), s(:lit, 1))
        expect(parse("!foo { 1 }")).must_equal s(:call, s(:iter, s(:call, nil, :foo), 0, s(:lit, 1)), :!)
        expect(parse("yield foo(arg) do |bar| end")).must_equal s(:yield, s(:iter, s(:call, nil, :foo, s(:call, nil, :arg)), s(:args, :bar)))
        expect(parse("foo(yield(bar)) do |bar| end")).must_equal s(:iter, s(:call, nil, :foo, s(:yield, s(:call, nil, :bar))), s(:args, :bar))
        expect(parse("yield foo arg do |bar| end")).must_equal s(:yield, s(:iter, s(:call, nil, :foo, s(:call, nil, :arg)), s(:args, :bar)))
        expect(parse("foo yield bar do |bar| end")).must_equal s(:iter, s(:call, nil, :foo, s(:yield, s(:call, nil, :bar))), s(:args, :bar))
        expect(parse("f ->() { g do end }")).must_equal s(:call, nil, :f, s(:iter, s(:lambda), s(:args), s(:iter, s(:call, nil, :g), 0)))
        expect(parse("a [ nil, b do end ]")).must_equal s(:call, nil, :a, s(:array, s(:nil), s(:iter, s(:call, nil, :b), 0)))
        expect(parse("private def f\na.b do end\nend")).must_equal s(:call, nil, :private, s(:defn, :f, s(:args), s(:iter, s(:call, s(:call, nil, :a), :b), 0)))
        expect(parse("foo def bar\n x.y do; end\n end")).must_equal s(:call, nil, :foo, s(:defn, :bar, s(:args), s(:iter, s(:call, s(:call, nil, :x), :y), 0)))
        expect(parse("[1,\nfoo do; end,\n 3]")).must_equal s(:array, s(:lit, 1), s(:iter, s(:call, nil, :foo), 0), s(:lit, 3))
        expect(parse("foo(1,\nbar do; end,\n 3)")).must_equal s(:call, nil, :foo, s(:lit, 1), s(:iter, s(:call, nil, :bar), 0), s(:lit, 3))
        expect(parse("foo do\nbar baz([\n])\nend\nfoo do\nend")).must_equal s(:block, s(:iter, s(:call, nil, :foo), 0, s(:call, nil, :bar, s(:call, nil, :baz, s(:array)))), s(:iter, s(:call, nil, :foo), 0))
        expect(parse("foo do\nbar baz([\n[ 1, 2 ],\n])\nend\nfoo do\nend")).must_equal s(:block, s(:iter, s(:call, nil, :foo), 0, s(:call, nil, :bar, s(:call, nil, :baz, s(:array, s(:array, s(:lit, 1), s(:lit, 2)))))), s(:iter, s(:call, nil, :foo), 0))
        expect(-> { parse("foo :a { 1 }") }).must_raise SyntaxError
        expect(-> { parse("1 < 2 { 1 }") }).must_raise SyntaxError
        expect(-> { parse("foo 1 < 2 { 3 }") }).must_raise SyntaxError
        expect(-> { parse("foo 1 | 2 { 3 }") }).must_raise SyntaxError
        expect(-> { parse("foo 1 & 2 { 3 }") }).must_raise SyntaxError
        expect(-> { parse("foo 1 << 2 { 3 }") }).must_raise SyntaxError
        expect(-> { parse("foo 1 + 2 { 3 }") }).must_raise SyntaxError
        expect(-> { parse("foo 1 * 2 { 3 }") }).must_raise SyntaxError
        expect(-> { parse("foo 1 ** 2 { 3 }") }).must_raise SyntaxError
        expect(-> { parse("foo ~1 { 3 }") }).must_raise SyntaxError
        expect_raise_with_message(-> { parse("m.a 1, &b do end") }, SyntaxError, "Both block arg and actual block given.")
      end

      it 'parses numbered block arg shorthand' do
        expect(parse('foo { _1; _2; _3 }')).must_equal s(:iter, s(:call, nil, :foo), 0, s(:block, s(:call, nil, :_1), s(:call, nil, :_2), s(:call, nil, :_3)))
        expect(parse('foo { |x| _1; _2; _3 }')).must_equal s(:iter, s(:call, nil, :foo), s(:args, :x), s(:block, s(:call, nil, :_1), s(:call, nil, :_2), s(:call, nil, :_3)))
        expect(parse('-> { _1; _2; _3 }')).must_equal s(:iter, s(:lambda), 0, s(:block, s(:call, nil, :_1), s(:call, nil, :_2), s(:call, nil, :_3)))
      end

      it 'parses block pass (ampersand operator)' do
        expect(parse('map(&:foo)')).must_equal s(:call, nil, :map, s(:block_pass, s(:lit, :foo)))
        expect(parse('map(&myblock)')).must_equal s(:call, nil, :map, s(:block_pass, s(:call, nil, :myblock)))
        expect(parse('map(&nil)')).must_equal s(:call, nil, :map, s(:block_pass, s(:nil)))
        expect(parse('map(&5.method(:+))')).must_equal s(:call, nil, :map, s(:block_pass, s(:call, s(:lit, 5), :method, s(:lit, :+))))
      end

      it 'parses break, next, super, and yield' do
        expect(parse('break')).must_equal s(:break)
        expect(parse('break()')).must_equal s(:break, s(:nil))
        expect(parse('break 1')).must_equal s(:break, s(:lit, 1))
        expect(parse('break 1, 2')).must_equal s(:break, s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('break([1, 2])')).must_equal s(:break, s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('break if true')).must_equal s(:if, s(:true), s(:break), nil)
        expect(parse('foo { break }')).must_equal s(:iter, s(:call, nil, :foo), 0, s(:break))
        expect(parse('next')).must_equal s(:next)
        expect(parse('next()')).must_equal s(:next, s(:nil))
        expect(parse('next 1')).must_equal s(:next, s(:lit, 1))
        expect(parse('next 1, 2')).must_equal s(:next, s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('next([1, 2])')).must_equal s(:next, s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('next if true')).must_equal s(:if, s(:true), s(:next), nil)
        expect(parse('foo { next }')).must_equal s(:iter, s(:call, nil, :foo), 0, s(:next))
        expect(parse('super')).must_equal s(:zsuper)
        expect(parse('super()')).must_equal s(:super)
        expect(parse('super 1')).must_equal s(:super, s(:lit, 1))
        expect(parse('super 1, 2')).must_equal s(:super, s(:lit, 1), s(:lit, 2))
        expect(parse('super a: b')).must_equal s(:super, s(bare_hash_type, s(:lit, :a), s(:call, nil, :b)))
        expect(parse('super([1, 2])')).must_equal s(:super, s(:array, s(:lit, 1), s(:lit, 2)))
        expect(parse('super if true')).must_equal s(:if, s(:true), s(:zsuper), nil)
        expect(parse('foo { super }')).must_equal s(:iter, s(:call, nil, :foo), 0, s(:zsuper))
        expect(parse('yield')).must_equal s(:yield)
        expect(parse('yield()')).must_equal s(:yield)
        expect(parse('yield 1')).must_equal s(:yield, s(:lit, 1))
        expect(parse('yield 1, 2')).must_equal s(:yield, s(:lit, 1), s(:lit, 2))
        expect(parse('yield(1, 2)')).must_equal s(:yield, s(:lit, 1), s(:lit, 2))
        expect(parse('yield if true')).must_equal s(:if, s(:true), s(:yield), nil)
        expect(parse('x += yield y')).must_equal s(:lasgn, :x, s(:call, s(:lvar, :x), :+, s(:yield, s(:call, nil, :y))))
        expect(parse('while yield == :foo; end')).must_equal s(:while, s(:call, s(:yield), :==, s(:lit, :foo)), nil, true)
        expect(parse('foo { yield }')).must_equal s(:iter, s(:call, nil, :foo), 0, s(:yield))
      end

      it 'parses self' do
        expect(parse('self')).must_equal s(:self)
      end

      it 'parses __FILE__, __LINE__ and __dir__' do
        expect(parse('__FILE__', 'foo/bar.rb')).must_equal s(:str, 'foo/bar.rb')
        expect(parse('__LINE__')).must_equal s(:lit, 1)
        expect(parse('__dir__')).must_equal s(:call, nil, :__dir__)
      end

      it 'parses splat *' do
        expect(parse('def foo(*args); end')).must_equal s(:defn, :foo, s(:args, :'*args'), s(:nil))
        expect(parse('def foo *args; end')).must_equal s(:defn, :foo, s(:args, :'*args'), s(:nil))
        expect(parse('foo(*args)')).must_equal s(:call, nil, :foo, s(:splat, s(:call, nil, :args)))
      end

      it 'parses keyword splat **' do
        expect(parse('def foo(**kwargs); end')).must_equal s(:defn, :foo, s(:args, :'**kwargs'), s(:nil))
        expect(parse('def foo(*args, **kwargs); end')).must_equal s(:defn, :foo, s(:args, :"*args", :"**kwargs"), s(:nil))
        expect(parse('def foo **kwargs; end')).must_equal s(:defn, :foo, s(:args, :'**kwargs'), s(:nil))
        expect(parse('foo(**kwargs)')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:kwsplat, s(:call, nil, :kwargs))))
        expect(parse('foo(a, **kwargs)')).must_equal s(:call, nil, :foo, s(:call, nil, :a), s(bare_hash_type, s(:kwsplat, s(:call, nil, :kwargs))))
        expect(parse('foo(**kwargs, a: 2)')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:kwsplat, s(:call, nil, :kwargs)), s(:lit, :a), s(:lit, 2)))
        expect(parse('foo(a: 1, **kwargs, b: 2)')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:lit, 1), s(:kwsplat, s(:call, nil, :kwargs)), s(:lit, :b), s(:lit, 2)))
        expect(parse('foo(a: :b, **kwargs)')).must_equal s(:call, nil, :foo, s(bare_hash_type, s(:lit, :a), s(:lit, :b), s(:kwsplat, s(:call, nil, :kwargs))))
        expect(-> { parse('foo { |nil| }') }).must_raise SyntaxError
        expect(-> { parse('foo { |*nil| }') }).must_raise SyntaxError
        expect(parse('foo { |**nil| }')).must_equal s(:iter, s(:call, nil, :foo), s(:args, :"**nil"))
      end

      it 'parses stabby proc' do
        expect(parse('-> { puts 1 }')).must_equal s(:iter, s(:lambda), 0, s(:call, nil, :puts, s(:lit, 1)))
        expect(parse('-> () { puts 1 }')).must_equal s(:iter, s(:lambda), s(:args), s(:call, nil, :puts, s(:lit, 1)))
        expect(parse('-> x { puts x }')).must_equal s(:iter, s(:lambda), s(:args, :x), s(:call, nil, :puts, s(:lvar, :x)))
        expect(parse('-> x, y { puts x, y }')).must_equal s(:iter, s(:lambda), s(:args, :x, :y), s(:call, nil, :puts, s(:lvar, :x), s(:lvar, :y)))
        expect(parse('-> (x, y) { x; y }')).must_equal s(:iter, s(:lambda), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y)))
        expect(-> { parse('->') }).must_raise(SyntaxError)
        expect(parse('foo -> { x } do y end')).must_equal s(:iter, s(:call, nil, :foo, s(:iter, s(:lambda), 0, s(:call, nil, :x))), 0, s(:call, nil, :y))
        expect(parse('foo = -> x { x }')).must_equal s(:lasgn, :foo, s(:iter, s(:lambda), s(:args, :x), s(:lvar, :x)))
        expect(parse('foo(1, &-> a, b { c })')).must_equal s(:call, nil, :foo, s(:lit, 1), s(:block_pass, s(:iter, s(:lambda), s(:args, :a, :b), s(:call, nil, :c))))
        expect(parse('foo(1, &(-> a, b { c }))')).must_equal s(:call, nil, :foo, s(:lit, 1), s(:block_pass, s(:iter, s(:lambda), s(:args, :a, :b), s(:call, nil, :c))))
        expect(parse('f ->() {}')).must_equal s(:call, nil, :f, s(:iter, s(:lambda), s(:args)))
        expect(parse('f ->() do end')).must_equal s(:call, nil, :f, s(:iter, s(:lambda), s(:args)))
      end

      it 'parses block-local and proc-local shadow variables' do
        expect(parse('-> (a; x) { x }')).must_equal s(:iter, s(:lambda), s(:args, :a, s(:shadow, :x)), s(:lvar, :x))
        expect(parse('-> (; x) { x }')).must_equal s(:iter, s(:lambda), s(:args, s(:shadow, :x)), s(:lvar, :x))
        expect(parse('lambda { |a; x| x }')).must_equal s(:iter, s(:call, nil, :lambda), s(:args, :a, s(:shadow, :x)), s(:lvar, :x))
        expect(parse('lambda { |; x| x }')).must_equal s(:iter, s(:call, nil, :lambda), s(:args, s(:shadow, :x)), s(:lvar, :x))
        expect(parse('foo { |a; x, y| y }')).must_equal s(:iter, s(:call, nil, :foo), s(:args, :a, s(:shadow, :x, :y)), s(:lvar, :y))

        # just a newline does not make a shadow variable
        expect(parse("-> (\nx) { x }")).must_equal s(:iter, s(:lambda), s(:args, :x), s(:lvar, :x))
        expect(parse("lambda { |\nx| x }")).must_equal s(:iter, s(:call, nil, :lambda), s(:args, :x), s(:lvar, :x))
        expect(-> { parse("-> (a\nx) { x }") }).must_raise SyntaxError
        expect(-> { parse("lambda { |a\nx| x }") }).must_raise SyntaxError

        # cannot have them in a method
        expect(-> { parse('def foo(a; x) end') }).must_raise SyntaxError
      end

      it 'parses case/when/else' do
        expect(parse("case 1\nwhen 1\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a)), nil)
        expect(parse("case 1\nwhen 1\n:a\n:b\n:c\nend")).must_equal s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a), s(:lit, :b), s(:lit, :c)), nil)
        expect(parse("case 1\nwhen 1 then :a\nend")).must_equal s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a)), nil)
        expect(parse("case 1\nwhen 1\n:a\n:b\nwhen 2, 3\n:c\nelse\n:d\nend")).must_equal s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a), s(:lit, :b)), s(:when, s(:array, s(:lit, 2), s(:lit, 3)), s(:lit, :c)), s(:lit, :d))
        expect(parse("case 1\nwhen 1 then :a\n:b\nwhen 2, 3 then :c\nelse :d\nend")).must_equal s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a), s(:lit, :b)), s(:when, s(:array, s(:lit, 2), s(:lit, 3)), s(:lit, :c)), s(:lit, :d))
        expect(parse("case 1\nwhen 1 then end")).must_equal s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), nil), nil)
        expect(parse("case\nwhen true\n:a\nelse\n:b\nend")).must_equal s(:case, nil, s(:when, s(:array, s(:true)), s(:lit, :a)), s(:lit, :b))
        expect(parse("case\nwhen true then :a\nelse :b\nend")).must_equal s(:case, nil, s(:when, s(:array, s(:true)), s(:lit, :a)), s(:lit, :b))
        expect(parse("case\nfoo; when 1; end")).must_equal s(:case, s(:call, nil, :foo), s(:when, s(:array, s(:lit, 1)), nil), nil)
        expect(parse("case; when 1; end")).must_equal s(:case, nil, s(:when, s(:array, s(:lit, 1)), nil), nil)
        expect(-> { parse("case 1\nelse\n:else\nend") }).must_raise SyntaxError
        expect(-> { parse("case 1\nwhen 1\n1\nelse\n:else\nwhen 2\n2\nend") }).must_raise SyntaxError
        expect(-> { parse("case 1\nelse\n:else\nwhen 2\n2\nend") }).must_raise SyntaxError
      end

      it 'parses case/in/else' do
        # nil
        expect(parse("case 1\nin nil\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:nil), nil), nil)
        expect(parse("case 1\nin nil, nil\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:nil), s(:nil)), nil), nil)
        expect(parse("case 1\nin nil, nil, nil\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:nil), s(:nil), s(:nil)), nil), nil)

        # string
        expect(parse("case 1\nin \"x\"\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:str, "x"), nil), nil)
        expect(parse("case 1\nin 'x'\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:str, "x"), nil), nil)

        # range
        expect(parse("case 1\nin 'a'..'z'\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:dot2, s(:str, 'a'), s(:str, 'z')), nil), nil)
        expect(parse("case 1\nin 'a'..'z' then 1\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:dot2, s(:str, "a"), s(:str, "z")), s(:lit, 1)), nil)
        if parser == 'NatalieParser'
          # NOTE: I like this better -- it's more consistent with normal ranges
          expect(parse("case 1\nin 1..2\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lit, 1..2), nil), nil)
          expect(parse("case 1\nin 1...2\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lit, 1...2), nil), nil)
          expect(parse("case 1\nin -1..2\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lit, -1..2), nil), nil)
        else
          expect(parse("case 1\nin 1..2\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:dot2, s(:lit, 1), s(:lit, 2)), nil), nil)
          expect(parse("case 1\nin 1...2\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:dot3, s(:lit, 1), s(:lit, 2)), nil), nil)
          expect(parse("case 1\nin -1..2\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:dot2, s(:lit, -1), s(:lit, 2)), nil), nil)
        end
        expect(parse("case 1\nin ...2\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:dot3, nil, s(:lit, 2)), nil), nil)
        expect(parse("case 1\nin (1...)\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:dot3, s(:lit, 1), nil), nil), nil)
        expect(parse("case 1\nin 1.. then 1\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:dot2, s(:lit, 1), nil), s(:lit, 1)), nil)

        # variable
        expect(parse("case 1\nin x\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lvar, :x), s(:lit, :a)), nil)
        expect(parse("case 1\nin x then :a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lvar, :x), s(:lit, :a)), nil)

        # splat
        expect(parse("case 1\nin *x then :a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, :"*x"), s(:lit, :a)), nil)
        expect(parse("case 1\nin :x, *y\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lit, :x), :"*y"), s(:lit, :a)), nil)
        expect(parse("case 1\nin [:x, *y]\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lit, :x), :"*y"), s(:lit, :a)), nil)

        # or
        expect(parse("case 1\nin x | y\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:or, s(:lvar, :x), s(:lvar, :y)), s(:lit, :a)), nil)

        # constant
        expect(parse("case 1\nin X\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:const, :X), s(:lit, :a)), nil)

        # pinned variable
        if parser == 'NatalieParser'
          # pinned variables not supported in ruby_parser yet
          expect(parse("case 1\nin [^a, a]\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:pin, s(:lvar, :a)), s(:lvar, :a)), s(:lit, :a)), nil)
        end

        # array pattern
        expect(parse("case 1\nin x, :y\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lvar, :x), s(:lit, :y)), s(:lit, :a)), nil)
        expect(parse("case 1\nin x, [:y]\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lvar, :x), s(:array_pat, nil, s(:lit, :y))), s(:lit, :a)), nil)
        expect(parse("case 1\nin [x], :y\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:array_pat, nil, s(:lvar, :x)), s(:lit, :y)), s(:lit, :a)), nil)
        expect(parse("case 1\nin []\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat), s(:lit, :a)), nil)
        expect(parse("case 1\nin [ ]\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat), nil), nil)
        expect(parse("case 1\nin [:x, x]\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lit, :x), s(:lvar, :x)), s(:lit, :a)), nil)
        expect(parse("case 1\nin [a, a]\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lvar, :a), s(:lvar, :a)), s(:lit, :a)), nil)
        expect(parse("case 1\nin [1, x]\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lit, 1), s(:lvar, :x)), s(:lit, :a)), nil)
        expect(parse("case 1\nin [1.2, x]\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lit, 1.2), s(:lvar, :x)), s(:lit, :a)), nil)
        expect(parse("case 1\nin ['one', x]\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:str, 'one'), s(:lvar, :x)), s(:lit, :a)), nil)
        expect(parse("case 1\nin [1, 2] => a\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lasgn, :a, s(:array_pat, nil, s(:lit, 1), s(:lit, 2))), s(:lit, :a)), nil)
        expect(parse("case 1\nin [1, 2,] => a\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lasgn, :a, s(:array_pat, nil, s(:lit, 1), s(:lit, 2), :*)), s(:lit, :a)), nil)
        expect(parse("case 1\nin [1 => a] => b\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lasgn, :b, s(:array_pat, nil, s(:lasgn, :a, s(:lit, 1)))), s(:lit, :a)), nil)

        # hash pattern
        expect(parse("case 1\nin {}\n:a\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil), s(:lit, :a)), nil)
        expect(parse("case 1\nin { x: x }\nx\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:lit, :x), s(:lvar, :x)), s(:call, nil, :x)), nil)
        expect(parse("case 1\nin { x: x, y: y }\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:lit, :x), s(:lvar, :x), s(:lit, :y), s(:lvar, :y)), nil), nil)
        expect(parse("case 1\nin { 'x': x, 'y': y }\nx\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:lit, :x), s(:lvar, :x), s(:lit, :y), s(:lvar, :y)), s(:call, nil, :x)), nil)
        expect(parse("case 1\nin { \"x\": x }\nx\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:lit, :x), s(:lvar, :x)), s(:call, nil, :x)), nil)
        expect(parse("case 1\nin { x: }\nx\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:lit, :x), nil), s(:lvar, :x)), nil)
        expect(parse("case 1\nin { x:, y: }\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:lit, :x), nil, s(:lit, :y), nil), nil), nil)
        expect(parse("case 1\nin { x: [:a, a] => b } => y\nx\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lasgn, :y, s(:hash_pat, nil, s(:lit, :x), s(:lasgn, :b, s(:array_pat, nil, s(:lit, :a), s(:lvar, :a))))), s(:call, nil, :x)), nil)
        expect(parse("case 1\nin { x:, **foo }\nx\nfoo\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:lit, :x), nil, s(:kwrest, :"**foo")), s(:lvar, :x), s(:lvar, :foo)), nil)
        expect(parse("case 1\nin { x:, ** }\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:lit, :x), nil, s(:kwrest, :**)), nil), nil)
        expect(parse("case 1\nin **foo\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:kwrest, :"**foo")), nil), nil)
        expect(parse("case 1\nin **nil\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:kwrest, :"**nil")), nil), nil)

        # parens only allow a single item?
        expect(parse("case 1\nin (1)\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lit, 1), nil), nil)
        expect(parse("case 1\nin (:x)\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:lit, :x), nil), nil)
        expect(parse("case 1\nin ([:x])\nend")).must_equal s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lit, :x)), nil), nil)
        expect(-> { parse("case 1\nin (:x, :y)\nend") }).must_raise SyntaxError
      end

      it 'parses begin/rescue/else/ensure' do
        expect(parse('begin;1;2;rescue;3;4;end')).must_equal s(:rescue, s(:block, s(:lit, 1), s(:lit, 2)), s(:resbody, s(:array), s(:lit, 3), s(:lit, 4)))
        expect(parse('begin;1;rescue => e;e;end')).must_equal s(:rescue, s(:lit, 1), s(:resbody, s(:array, s(:lasgn, :e, s(:gvar, :$!))), s(:lvar, :e)))
        expect(parse('begin;rescue SyntaxError, NoMethodError => e;3;end')).must_equal s(:rescue, s(:resbody, s(:array, s(:const, :SyntaxError), s(:const, :NoMethodError), s(:lasgn, :e, s(:gvar, :$!))), s(:lit, 3)))
        expect(parse('begin;rescue SyntaxError;3;else;4;5;end')).must_equal s(:rescue, s(:resbody, s(:array, s(:const, :SyntaxError)), s(:lit, 3)), s(:block, s(:lit, 4), s(:lit, 5)))
        expect(parse("begin\n0\nensure\n:a\n:b\nend")).must_equal s(:ensure, s(:lit, 0), s(:block, s(:lit, :a), s(:lit, :b)))
        expect(parse('begin;0;rescue;:a;else;:c;ensure;:d;:e;end')).must_equal s(:ensure, s(:rescue, s(:lit, 0), s(:resbody, s(:array), s(:lit, :a)), s(:lit, :c)), s(:block, s(:lit, :d), s(:lit, :e)))
        expect(parse('def foo;0;rescue;:a;else;:c;ensure;:d;:e;end')).must_equal s(:defn, :foo, s(:args), s(:ensure, s(:rescue, s(:lit, 0), s(:resbody, s(:array), s(:lit, :a)), s(:lit, :c)), s(:block, s(:lit, :d), s(:lit, :e))))
        expect(parse('def foo;0;ensure;:a;:e;end')).must_equal s(:defn, :foo, s(:args), s(:ensure, s(:lit, 0), s(:block, s(:lit, :a), s(:lit, :e))))
        expect(parse('begin;0;rescue foo(1), bar(2);1;end')).must_equal s(:rescue, s(:lit, 0), s(:resbody, s(:array, s(:call, nil, :foo, s(:lit, 1)), s(:call, nil, :bar, s(:lit, 2))), s(:lit, 1)))
        expect(parse('begin;0;ensure;1;end')).must_equal s(:ensure, s(:lit, 0), s(:lit, 1))
        expect(parse('x ||= begin;0;rescue;1;end')).must_equal s(:op_asgn_or, s(:lvar, :x), s(:lasgn, :x, s(:rescue, s(:lit, 0), s(:resbody, s(:array), s(:lit, 1)))))
        expect(parse('x = begin;1;2;end')).must_equal s(:lasgn, :x, s(:block, s(:lit, 1), s(:lit, 2)))
        expect(parse("foo do;rescue;1;else;2;ensure;3;end")).must_equal s(:iter, s(:call, nil, :foo), 0, s(:ensure, s(:rescue, s(:resbody, s(:array), s(:lit, 1)), s(:lit, 2)), s(:lit, 3)))
        expect(parse("foo do\n\nrescue\n1\nelse;2\nensure\n3\nend")).must_equal s(:iter, s(:call, nil, :foo), 0, s(:ensure, s(:rescue, s(:resbody, s(:array), s(:lit, 1)), s(:lit, 2)), s(:lit, 3)))
        expect(parse("class Foo;rescue;1;else;2;ensure;3;end")).must_equal s(:class, :Foo, nil, s(:ensure, s(:rescue, s(:resbody, s(:array), s(:lit, 1)), s(:lit, 2)), s(:lit, 3)))
        expect(parse("module Foo;rescue;1;else;2;ensure;3;end")).must_equal s(:module, :Foo, s(:ensure, s(:rescue, s(:resbody, s(:array), s(:lit, 1)), s(:lit, 2)), s(:lit, 3)))
        expect(parse("h[k]=begin\n42\nend")).must_equal s(:attrasgn, s(:call, nil, :h), :[]=, s(:call, nil, :k), s(:lit, 42))
        expect(parse("a begin\nb.c do end\nend")).must_equal s(:call, nil, :a, s(:iter, s(:call, s(:call, nil, :b), :c), 0))
      end

      it 'parses inline rescue' do
        expect(parse('foo rescue bar')).must_equal s(:rescue, s(:call, nil, :foo), s(:resbody, s(:array), s(:call, nil, :bar)))
        expect(parse('foo(1) { 2 } rescue [1, 2]')).must_equal s(:rescue, s(:iter, s(:call, nil, :foo, s(:lit, 1)), 0, s(:lit, 2)), s(:resbody, s(:array), s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse("a = b and c = d(1) rescue 2")).must_equal s(:and, s(:lasgn, :a, s(:call, nil, :b)), s(:lasgn, :c, s(:rescue, s(:call, nil, :d, s(:lit, 1)), s(:resbody, s(:array), s(:lit, 2)))))
        expect(parse("a = b(1) rescue 2")).must_equal s(:lasgn, :a, s(:rescue, s(:call, nil, :b, s(:lit, 1)), s(:resbody, s(:array), s(:lit, 2))))
      end

      it 'parses backticks and %x()' do
        expect(parse('`ls`')).must_equal s(:xstr, 'ls')
        expect(parse('%x(ls)')).must_equal s(:xstr, 'ls')
        expect(parse("%x(ls \#{path})")).must_equal s(:dxstr, 'ls ', s(:evstr, s(:call, nil, :path)))
        expect(parse(%q(`#{1+1} #{2+2}`.foo(1).strip))).must_equal s(:call, s(:call, s(:dxstr, "", s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, " "), s(:evstr, s(:call, s(:lit, 2), :+, s(:lit, 2)))), :foo, s(:lit, 1)), :strip)
      end

      it 'parses alias' do
        expect(parse('alias foo bar')).must_equal s(:alias, s(:lit, :foo), s(:lit, :bar))
        expect(parse('alias :foo :bar')).must_equal s(:alias, s(:lit, :foo), s(:lit, :bar))
        expect(parse('alias :"foo" :"bar"')).must_equal s(:alias, s(:lit, :foo), s(:lit, :bar))
        expect(parse("alias write <<\ndef foo; end")).must_equal s(:block, s(:alias, s(:lit, :write), s(:lit, :<<)), s(:defn, :foo, s(:args), s(:nil)))
        expect(parse("alias << write\ndef foo; end")).must_equal s(:block, s(:alias, s(:lit, :<<), s(:lit, :write)), s(:defn, :foo, s(:args), s(:nil)))
        expect(parse("alias yield <<\ndef foo; end")).must_equal s(:block, s(:alias, s(:lit, :yield), s(:lit, :<<)), s(:defn, :foo, s(:args), s(:nil)))
        expect(parse("alias << yield\ndef foo; end")).must_equal s(:block, s(:alias, s(:lit, :<<), s(:lit, :yield)), s(:defn, :foo, s(:args), s(:nil)))
      end

      it 'parses defined?' do
        expect(parse('defined? foo')).must_equal s(:defined, s(:call, nil, :foo))
        expect(parse('defined?(:foo)')).must_equal s(:defined, s(:lit, :foo))
        expect(parse('defined?(:foo) != "constant"')).must_equal s(:call, s(:defined, s(:lit, :foo)), :!=, s(:str, "constant"))
      end

      it 'parses logical and/or with block' do
        result = parse('x = foo.bar || ->(i) { baz(i) }')
        expect(result).must_equal s(:lasgn, :x, s(:or, s(:call, s(:call, nil, :foo), :bar), s(:iter, s(:lambda), s(:args, :i), s(:call, nil, :baz, s(:lvar, :i)))))
        result = parse('x = foo.bar || ->(i) do baz(i) end')
        expect(result).must_equal s(:lasgn, :x, s(:or, s(:call, s(:call, nil, :foo), :bar), s(:iter, s(:lambda), s(:args, :i), s(:call, nil, :baz, s(:lvar, :i)))))
        result = parse('x = foo.bar && ->(i) { baz(i) }')
        expect(result).must_equal s(:lasgn, :x, s(:and, s(:call, s(:call, nil, :foo), :bar), s(:iter, s(:lambda), s(:args, :i), s(:call, nil, :baz, s(:lvar, :i)))))
        result = parse('x = foo.bar && ->(i) do baz(i) end')
        expect(result).must_equal s(:lasgn, :x, s(:and, s(:call, s(:call, nil, :foo), :bar), s(:iter, s(:lambda), s(:args, :i), s(:call, nil, :baz, s(:lvar, :i)))))
      end

      it 'parses redo' do
        expect(parse('redo')).must_equal s(:redo)
      end

      it 'parses retry' do
        expect(parse('retry')).must_equal s(:retry)
      end

      it 'parses heredocs' do
        # empty heredoc
        expect(parse("<<FOO\nFOO")).must_equal s(:str, "")

        # underscore in delimiter
        doc1 = <<~END
        foo = <<FOO_BAR
         1
        2
        FOO_BAR
        END
        expect(parse(doc1)).must_equal s(:lasgn, :foo, s(:str, " 1\n2\n"))

        # passed as an argument
        doc2 = <<~END
        foo(1, <<-foo, 2)
         1
        2
          foo
        END
        expect(parse(doc2)).must_equal s(:call, nil, :foo, s(:lit, 1), s(:str, " 1\n2\n"), s(:lit, 2))

        # interpolation
        doc3 = <<~'END'
        <<FOO
          #{1+1}
        FOO
        END
        expect(parse(doc3)).must_equal s(:dstr, '  ', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, "\n"))

        # delimiter appears inside body but not by itself
        doc4 = <<~END
        <<-BAR
        FOOBAR
          BAR
        END
        expect(parse(doc4)).must_equal s(:str, "FOOBAR\n")

        # unicode delimiter
        doc5 = <<~END
        <<OOTPÜT
        hello unicode
        OOTPÜT
        END
        expect(parse(doc5)).must_equal s(:str, "hello unicode\n")

        # just a sanity check!
        ruby_only_counts_literal_tabs_as_indentation = <<~"EOF"
        \tfoo
        \tbar
        EOF
        expect(ruby_only_counts_literal_tabs_as_indentation).must_equal "\tfoo\n\tbar\n"

        doc6 = <<~'END'
        a = <<~"EOF"
          \tbackslash tabs are ignored
          \tbackslash tabs don't count
        EOF
        END
        if parser == 'NatalieParser'
          expect(parse(doc6)).must_equal s(:lasgn, :a, s(:str, "\tbackslash tabs are ignored\n\tbackslash tabs don't count\n"))
        else
          # NOTE: this seems like a bug in RubyParser
          expect(parse(doc6)).must_equal s(:lasgn, :a, s(:str, "backslash tabs are ignored\nbackslash tabs don't count\n"))
        end

        # line continuation
        doc7 = <<~'END'
          <<-EOF
          foo \
          bar
          EOF
        END
        expect(parse(doc7)).must_equal s(:str, "foo bar\n")

        # line number of evaluated parts
        doc8 = <<~'END'
          # skip a line
          <<-EOF
          1
            #{42}
          EOF
        END
        result = parse(doc8)
        expect(result).must_equal s(:dstr, "1\n  ", s(:evstr, s(:lit, 42)), s(:str, "\n"))
        expect(result.line).must_equal(2)
        expect(result[2].line).must_equal(4)

        # inside array
        doc9 = <<~END
          [<<-EOF,
            foo
            EOF
            1,
            2]
        END
        expect(parse(doc9)).must_equal s(:array, s(:str, "  foo\n"), s(:lit, 1), s(:lit, 2))

        # FIXME: heredoc inside heredoc interpolation
        #doc10 = "[<<A,\n\#{<<B}\nb\nB\na\nA\n0]"
        #expect(parse(doc10)).must_equal s(:array, s(:str, "b\n\na\n"), s(:lit, 0))
      end

      it 'parses =begin =end doc blocks' do
        {
          class: :class,
          def: :defn,
          module: :module
        }.each do |keyword, sexp_type|
          doc = <<-END.gsub(/^\s+/, '')
            =begin
            embedded doc
            =end
            #{keyword} Foo
            end
          END
          node = parse(doc)
          expect(node.sexp_type).must_equal sexp_type
          expect(node.comments).must_equal("=begin\nembedded doc\n=end\n", "missing doc block on #{sexp_type}")
        end
      end

      it 'parses BEGIN and END' do
        expect(parse('BEGIN { }')).must_equal s(:iter, s(:preexe), 0)
        expect(parse('BEGIN { 1 }')).must_equal s(:iter, s(:preexe), 0, s(:lit, 1))
        expect(parse('BEGIN { 1; 2 }')).must_equal s(:iter, s(:preexe), 0, s(:block, s(:lit, 1), s(:lit, 2)))
        expect(parse('END { }')).must_equal s(:iter, s(:postexe), 0)
        expect(parse('END { 1 }')).must_equal s(:iter, s(:postexe), 0, s(:lit, 1))
        expect(parse('END { 1; 2 }')).must_equal s(:iter, s(:postexe), 0, s(:block, s(:lit, 1), s(:lit, 2)))
        expect(parse('class Foo; END { 1 }; end')).must_equal s(:class, :Foo, nil, s(:iter, s(:postexe), 0, s(:lit, 1)))

        expect_raise_with_message(-> { parse('class Foo; BEGIN { 1 }; end') }, SyntaxError, 'BEGIN is permitted only at toplevel')
        expect(-> { parse('BEGIN 1') }).must_raise SyntaxError
        expect(-> { parse('BEGIN do 1 end') }).must_raise SyntaxError
        expect(-> { parse('END 1') }).must_raise SyntaxError
        expect(-> { parse('END do 1 end') }).must_raise SyntaxError
      end

      it 'parses __ENCODING__' do
        expect(parse('__ENCODING__')).must_equal s(:colon2, s(:const, :Encoding), :UTF_8)
        expect(-> { parse('__ENCODING__ 1') }).must_raise SyntaxError
        expect(-> { parse('__ENCODING__ do; end') }).must_raise SyntaxError
      end

      it 'parses regular comment doc blocks' do
        {
          class: :class,
          def: :defn,
          module: :module
        }.each do |keyword, sexp_type|
          doc = "# embedded doc\n" \
                "\n" \
                "# line 2\n" \
                "\n" \
                "#{keyword} Foo\n" \
                "end"
          node = parse(doc)
          expect(node.sexp_type).must_equal sexp_type
          expect(node.comments).must_equal("# embedded doc\n\n# line 2\n\n", "missing doc block on #{sexp_type}")

          # indented
          doc = "    # embedded doc\n" \
                "\n" \
                "    # line 2\n" \
                "\n" \
                "    #{keyword} Foo\n" \
                "    end"
          node = parse(doc)
          expect(node.sexp_type).must_equal sexp_type
          expect(node.comments).must_equal("# embedded doc\n\n# line 2\n\n", "missing doc block on #{sexp_type}")
        end

        # ignores inline comments
        nodes = parse("foo # ignored\nbar")[1..-1]
        expect(nodes.map(&:comments)).must_equal [nil, nil]

        # does not include previous ignored comments
        doc = <<-CODE
          def foo
            # foo logic
          end

          # bar stuff
          def bar
            # bar logic
          end
        CODE
        node = parse(doc)
        bar = node[2]
        expect(bar.comments).must_equal "# bar stuff\n"
      end

      it 'tracks file and line/column number' do
        ast = parse("1 +\n\n    2", 'foo.rb')
        expect(ast.file).must_equal('foo.rb')
        expect(ast.line).must_equal(1)
        expect(ast.column).must_equal(3) if parser == 'NatalieParser'

        two = ast.last
        expect(two).must_equal s(:lit, 2)
        expect(two.file).must_equal('foo.rb')
        expect(two.line).must_equal(3)
        expect(two.column).must_equal(5) if parser == 'NatalieParser'

        expect(ast.file).must_be_same_as(two.file)
      end

      it 'does not panic on certain errors' do
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('foo sel$f: a') }, SyntaxError, "(string)#1: syntax error, unexpected ':' (expected: 'expression')")
        end
      end

      it 'does not wrap everything in a block' do
        expect(parse('1')).must_equal s(:lit, 1)
      end

      it 'ignores UTF-8 BOM (any other BOM will error)' do
        expect(parse("\xEF\xBB\xBFfoo")).must_equal s(:call, nil, :foo)
      end
    end
  end
end
