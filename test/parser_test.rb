require_relative './test_helper'

%w[RubyParser NatalieParser].each do |parser|
  describe parser do
    if parser == 'NatalieParser'
      def parse(code, path = '(string)')
        NatalieParser.parse(code, path)
      end
    else
      require 'ruby_parser'
      def parse(code, path = '(string)')
        node = RubyParser.new.parse(code, path)
        if node.nil?
          s(:block)
        elsif node.first == :block
          node
        else
          node.new(:block, node)
        end
      rescue Racc::ParseError, RubyParser::SyntaxError => e
        raise SyntaxError, e.message
      end
    end

    def expect_raise_with_message(callable, error, message)
      actual_error = expect(callable).must_raise(error)
      expect(actual_error.message).must_equal(message)
    end

    describe '#parse' do
      it 'parses an empty program' do
        expect(parse('')).must_equal s(:block)
      end

      it 'parses numbers' do
        expect(parse('1')).must_equal s(:block, s(:lit, 1))
        expect(parse(' 1234')).must_equal s(:block, s(:lit, 1234))
        expect(parse('1.5 ')).must_equal s(:block, s(:lit, 1.5))
        expect(parse('-1')).must_equal s(:block, s(:lit, -1))
        expect(parse('-1.5')).must_equal s(:block, s(:lit, -1.5))
      end

      it 'parses unary operators' do
        expect(parse('-foo.odd?')).must_equal s(:block, s(:call, s(:call, s(:call, nil, :foo), :odd?), :-@))
        expect(parse('-2.odd?')).must_equal s(:block, s(:call, s(:lit, -2), :odd?))
        expect(parse('+2.even?')).must_equal s(:block, s(:call, s(:lit, 2), :even?))
        expect(parse('-(2*8)')).must_equal s(:block, s(:call, s(:call, s(:lit, 2), :*, s(:lit, 8)), :-@))
        expect(parse('+(2*8)')).must_equal s(:block, s(:call, s(:call, s(:lit, 2), :*, s(:lit, 8)), :+@))
        expect(parse('-2*8')).must_equal s(:block, s(:call, s(:lit, -2), :*, s(:lit, 8)))
        expect(parse('+2*8')).must_equal s(:block, s(:call, s(:lit, 2), :*, s(:lit, 8)))
        expect(parse('+1**2')).must_equal s(:block, s(:call, s(:lit, 1), :**, s(:lit, 2)))
        expect(parse('-1**2')).must_equal s(:block, s(:call, s(:call, s(:lit, 1), :**, s(:lit, 2)), :-@))
        expect(parse('~1')).must_equal s(:block, s(:call, s(:lit, 1), :~))
        expect(parse('~foo')).must_equal s(:block, s(:call, s(:call, nil, :foo), :~))
      end

      it 'parses operator expressions' do
        expect(parse('1 + 3')).must_equal s(:block, s(:call, s(:lit, 1), :+, s(:lit, 3)))
        expect(parse('1+3')).must_equal s(:block, s(:call, s(:lit, 1), :+, s(:lit, 3)))
        expect(parse("1+\n 3")).must_equal s(:block, s(:call, s(:lit, 1), :+, s(:lit, 3)))
        expect(parse('1 - 3')).must_equal s(:block, s(:call, s(:lit, 1), :-, s(:lit, 3)))
        expect(parse('1 * 3')).must_equal s(:block, s(:call, s(:lit, 1), :*, s(:lit, 3)))
        expect(parse('1 / 3')).must_equal s(:block, s(:call, s(:lit, 1), :/, s(:lit, 3)))
        expect(parse('1 * 2 + 3')).must_equal s(:block, s(:call, s(:call, s(:lit, 1), :*, s(:lit, 2)), :+, s(:lit, 3)))
        expect(parse('1 / 2 - 3')).must_equal s(:block, s(:call, s(:call, s(:lit, 1), :/, s(:lit, 2)), :-, s(:lit, 3)))
        expect(parse('1 + 2 * 3')).must_equal s(:block, s(:call, s(:lit, 1), :+, s(:call, s(:lit, 2), :*, s(:lit, 3))))
        expect(parse('1 - 2 / 3')).must_equal s(:block, s(:call, s(:lit, 1), :-, s(:call, s(:lit, 2), :/, s(:lit, 3))))
        expect(parse('(1 + 2) * 3')).must_equal s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:lit, 3)))
        expect(parse("(\n1 + 2\n) * 3")).must_equal s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:lit, 3)))
        expect(parse('(1 - 2) / 3')).must_equal s(:block, s(:call, s(:call, s(:lit, 1), :-, s(:lit, 2)), :/, s(:lit, 3)))
        expect(parse('(1 + 2) * (3 + 4)')).must_equal s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:call, s(:lit, 3), :+, s(:lit, 4))))
        expect(parse('"foo" + "bar"')).must_equal s(:block, s(:call, s(:str, 'foo'), :+, s(:str, 'bar')))
        expect(parse('1 == 1')).must_equal s(:block, s(:call, s(:lit, 1), :==, s(:lit, 1)))
        expect(parse('a != b')).must_equal s(:block, s(:call, s(:call, nil, :a), :!=, s(:call, nil, :b)))
        expect(parse('Foo === bar')).must_equal s(:block, s(:call, s(:const, :Foo), :===, s(:call, nil, :bar)))
        expect(parse('1 < 2')).must_equal s(:block, s(:call, s(:lit, 1), :<, s(:lit, 2)))
        expect(parse('1 <= 2')).must_equal s(:block, s(:call, s(:lit, 1), :<=, s(:lit, 2)))
        expect(parse('1 > 2')).must_equal s(:block, s(:call, s(:lit, 1), :>, s(:lit, 2)))
        expect(parse('1 >= 2')).must_equal s(:block, s(:call, s(:lit, 1), :>=, s(:lit, 2)))
        expect(parse('5-3')).must_equal s(:block, s(:call, s(:lit, 5), :-, s(:lit, 3)))
        expect(parse('5 -3')).must_equal s(:block, s(:call, s(:lit, 5), :-, s(:lit, 3)))
        expect(parse('1 +1')).must_equal s(:block, s(:call, s(:lit, 1), :+, s(:lit, 1)))
        expect(parse('(1+2)-3 == 0')).must_equal s(:block, s(:call, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :-, s(:lit, 3)), :==, s(:lit, 0)))
        expect(parse('2 ** 10')).must_equal s(:block, s(:call, s(:lit, 2), :**, s(:lit, 10)))
        expect(parse('1 * 2 ** 10 + 3')).must_equal s(:block, s(:call, s(:call, s(:lit, 1), :*, s(:call, s(:lit, 2), :**, s(:lit, 10))), :+, s(:lit, 3)))
        expect(parse('1 & 2 | 3 ^ 4')).must_equal s(:block, s(:call, s(:call, s(:call, s(:lit, 1), :&, s(:lit, 2)), :|, s(:lit, 3)), :^, s(:lit, 4)))
        expect(parse('10 % 3')).must_equal s(:block, s(:call, s(:lit, 10), :%, s(:lit, 3)))
        expect(parse('x << 1')).must_equal s(:block, s(:call, s(:call, nil, :x), :<<, s(:lit, 1)))
        expect(parse('x =~ y')).must_equal s(:block, s(:call, s(:call, nil, :x), :=~, s(:call, nil, :y)))
        expect(parse('x =~ /foo/')).must_equal s(:block, s(:match3, s(:lit, /foo/), s(:call, nil, :x)))
        expect(parse('/foo/ =~ x')).must_equal s(:block, s(:match2, s(:lit, /foo/), s(:call, nil, :x)))
        expect(parse('x !~ y')).must_equal s(:block, s(:not, s(:call, s(:call, nil, :x), :=~, s(:call, nil, :y))))
        expect(parse('x !~ /foo/')).must_equal s(:block, s(:not, s(:match3, s(:lit, /foo/), s(:call, nil, :x))))
        expect(parse('/foo/ !~ x')).must_equal s(:block, s(:not, s(:match2, s(:lit, /foo/), s(:call, nil, :x))))
        expect(parse('x &&= 1')).must_equal s(:block, s(:op_asgn_and, s(:lvar, :x), s(:lasgn, :x, s(:lit, 1))))
        expect(parse('x ||= 1')).must_equal s(:block, s(:op_asgn_or, s(:lvar, :x), s(:lasgn, :x, s(:lit, 1))))
        expect(parse('x += 1')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :+, s(:lit, 1))))
        expect(parse('x -= 1')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :-, s(:lit, 1))))
        expect(parse('x *= 1')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :*, s(:lit, 1))))
        expect(parse('x **= 1')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :**, s(:lit, 1))))
        expect(parse('x /= 1')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :/, s(:lit, 1))))
        expect(parse('x %= 1')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :%, s(:lit, 1))))
        expect(parse('x |= 1')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :|, s(:lit, 1))))
        expect(parse('x &= 1')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :&, s(:lit, 1))))
        expect(parse('x >>= 1')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :>>, s(:lit, 1))))
        expect(parse('x <<= 1')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :<<, s(:lit, 1))))
        expect(parse('n = x += 1')).must_equal s(:block, s(:lasgn, :n, s(:lasgn, :x, s(:call, s(:lvar, :x), :+, s(:lit, 1)))))
        expect(parse('x.y += 1')).must_equal s(:block, s(:op_asgn2, s(:call, nil, :x), :y=, :+, s(:lit, 1)))
        expect(parse('x[:y] += 1')).must_equal s(:block, s(:op_asgn1, s(:call, nil, :x), s(:arglist, s(:lit, :y)), :+, s(:lit, 1)))
        expect(parse('foo&.bar')).must_equal s(:block, s(:safe_call, s(:call, nil, :foo), :bar))
        expect(parse('foo&.bar 1')).must_equal s(:block, s(:safe_call, s(:call, nil, :foo), :bar, s(:lit, 1)))
        expect(parse('foo&.bar x')).must_equal s(:block, s(:safe_call, s(:call, nil, :foo), :bar, s(:call, nil, :x)))
        expect(parse('foo <=> bar')).must_equal s(:block, s(:call, s(:call, nil, :foo), :<=>, s(:call, nil, :bar)))
        expect(parse('1**2')).must_equal s(:block, s(:call, s(:lit, 1), :**, s(:lit, 2)))
      end

      it 'parses and/or' do
        expect(parse('1 && 2 || 3 && 4')).must_equal s(:block, s(:or, s(:and, s(:lit, 1), s(:lit, 2)), s(:and, s(:lit, 3), s(:lit, 4))))
        expect(parse('false and true and false')).must_equal s(:block, s(:and, s(:false), s(:and, s(:true), s(:false))))
        expect(parse('false or true or false')).must_equal s(:block, s(:or, s(:false), s(:or, s(:true), s(:false))))
        expect(parse('false && true && false')).must_equal s(:block, s(:and, s(:false), s(:and, s(:true), s(:false))))
        expect(parse('false || true || false')).must_equal s(:block, s(:or, s(:false), s(:or, s(:true), s(:false))))
        expect(parse('1 and 2 or 3 and 4')).must_equal s(:block, s(:and, s(:or, s(:and, s(:lit, 1), s(:lit, 2)), s(:lit, 3)), s(:lit, 4)))
        expect(parse('1 or 2 and 3 or 4')).must_equal s(:block, s(:or, s(:and, s(:or, s(:lit, 1), s(:lit, 2)), s(:lit, 3)), s(:lit, 4)))
      end

      it 'parses ! and not' do
        expect(parse('!false')).must_equal s(:block, s(:call, s(:false), :!))
        expect(parse('not false')).must_equal s(:block, s(:call, s(:false), :!))
        expect(parse('!foo.bar(baz)')).must_equal s(:block, s(:call, s(:call, s(:call, nil, :foo), :bar, s(:call, nil, :baz)), :!))
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
        expect(parse('""')).must_equal s(:block, s(:str, ''))
        expect(parse('"foo"')).must_equal s(:block, s(:str, 'foo'))
        expect(parse('"this is \"quoted\""')).must_equal s(:block, s(:str, 'this is "quoted"'))
        expect(parse('"other escaped chars \\\\ \n"')).must_equal s(:block, s(:str, "other escaped chars \\ \n"))
        expect(parse("''")).must_equal s(:block, s(:str, ''))
        expect(parse("'foo'")).must_equal s(:block, s(:str, 'foo'))
        expect(parse("'this is \\'quoted\\''")).must_equal s(:block, s(:str, "this is 'quoted'"))
        expect(parse("'other escaped chars \\\\ \\n'")).must_equal s(:block, s(:str, "other escaped chars \\ \\n"))
        expect(parse('"#{:foo} bar #{1 + 1}"')).must_equal s(:block, s(:dstr, '', s(:evstr, s(:lit, :foo)), s(:str, ' bar '), s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1)))))
        expect(parse('y = "#{1 + 1} 2"')).must_equal s(:block, s(:lasgn, :y, s(:dstr, '', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, ' 2'))))
        expect(parse('x.y = "#{1 + 1} 2"')).must_equal s(:block, s(:attrasgn, s(:call, nil, :x), :y=, s(:dstr, '', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, ' 2'))))
        expect(parse(%("\#{1}foo\#{''}bar"))).must_equal s(:block, s(:dstr, '', s(:evstr, s(:lit, 1)), s(:str, 'foo'), s(:str, ''), s(:str, 'bar')))
      end

      it 'parses symbols' do
        expect(parse(':foo')).must_equal s(:block, s(:lit, :foo))
        expect(parse(':foo_bar')).must_equal s(:block, s(:lit, :foo_bar))
        expect(parse(':"foo bar"')).must_equal s(:block, s(:lit, :'foo bar'))
        expect(parse(':FooBar')).must_equal s(:block, s(:lit, :FooBar))
      end

      it 'parses regexps' do
        expect(parse('/foo/')).must_equal s(:block, s(:lit, /foo/))
        expect(parse('/foo/i')).must_equal s(:block, s(:lit, /foo/i))
        expect(parse('//mix')).must_equal s(:block, s(:lit, //mix))
        %w[e s u].each do |flag|
          expect(parse("//#{flag}").last.last.options).must_equal 16
        end
        expect(parse('/#{1+1}/mixn')).must_equal s(:block, s(:dregx, '', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), 39))
        expect(parse('/foo #{1+1}/')).must_equal s(:block, s(:dregx, 'foo ', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1)))))
        expect(parse('/^$(.)[.]{1}.*.+.?\^\$\.\(\)\[\]\{\}\w\W\d\D\h\H\s\S\R\*\+\?/')).must_equal s(:block, s(:lit, /^$(.)[.]{1}.*.+.?\^\$\.\(\)\[\]\{\}\w\W\d\D\h\H\s\S\R\*\+\?/))
        expect(parse("/\\n\\\\n/")).must_equal s(:block, s(:lit, /\n\\n/))
        expect(parse("/\\/\\* foo \\*\\//")).must_equal s(:block, s(:lit, Regexp.new("/\\* foo \\*/")))
        expect(parse("/\\&\\a\\b/")).must_equal s(:block, s(:lit, /\&\a\b/))
      end

      it 'parses multiple expressions' do
        expect(parse("1 + 2\n3 + 4")).must_equal s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:call, s(:lit, 3), :+, s(:lit, 4)))
        expect(parse("1 + 2;'foo'")).must_equal s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, 'foo'))
      end

      it 'parses assignment' do
        expect(parse('x = 1')).must_equal s(:block, s(:lasgn, :x, s(:lit, 1)))
        expect(parse('x = 1 + 2')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lit, 1), :+, s(:lit, 2))))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('x =') }, SyntaxError, "(string)#1: syntax error, unexpected end-of-input (expected: 'expression')")
          expect_raise_with_message(-> { parse('[1] = 2') }, SyntaxError, "(string)#1: syntax error, unexpected '[' (expected: 'left side of assignment')")
        else
          expect_raise_with_message(-> { parse('x =') }, SyntaxError, '(string):1 :: parse error on value "$" ($end)')
          expect_raise_with_message(-> { parse('[1] = 2') }, SyntaxError, '(string):1 :: parse error on value "=" (tEQL)')
        end
        expect(parse('@foo = 1')).must_equal s(:block, s(:iasgn, :@foo, s(:lit, 1)))
        expect(parse('@@abc_123 = 1')).must_equal s(:block, s(:cvdecl, :@@abc_123, s(:lit, 1)))
        expect(parse('$baz = 1')).must_equal s(:block, s(:gasgn, :$baz, s(:lit, 1)))
        expect(parse('Constant = 1')).must_equal s(:block, s(:cdecl, :Constant, s(:lit, 1)))
        expect(parse('x, y = [1, 2]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2)))))
        expect(parse('(x, y) = [1, 2]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2)))))
        expect(parse('@x, $y, Z = foo')).must_equal s(:block, s(:masgn, s(:array, s(:iasgn, :@x), s(:gasgn, :$y), s(:cdecl, :Z)), s(:to_ary, s(:call, nil, :foo))))
        expect(parse('(@x, $y, Z) = foo')).must_equal s(:block, s(:masgn, s(:array, s(:iasgn, :@x), s(:gasgn, :$y), s(:cdecl, :Z)), s(:to_ary, s(:call, nil, :foo))))
        expect(parse('(a, (b, c)) = [1, [2, 3]]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:lasgn, :c)))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3))))))
        expect(parse('(a, b), c = [[1, 2], 3]')).must_equal s(:block, s(:masgn, s(:array, s(:masgn, s(:array, s(:lasgn, :a), s(:lasgn, :b))), s(:lasgn, :c)), s(:to_ary, s(:array, s(:array, s(:lit, 1), s(:lit, 2)), s(:lit, 3)))))
        expect(parse('((a, b), c) = [[1, 2], 3]')).must_equal s(:block, s(:masgn, s(:array, s(:masgn, s(:array, s(:lasgn, :a), s(:lasgn, :b))), s(:lasgn, :c)), s(:to_ary, s(:array, s(:array, s(:lit, 1), s(:lit, 2)), s(:lit, 3)))))
        expect(parse('a, (b, c) = [1, [2, 3]]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:lasgn, :c)))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3))))))
        expect(parse('a, (b, *c) = [1, [2, 3, 4]]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:splat, s(:lasgn, :c))))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3), s(:lit, 4))))))
        expect(parse('a, (b, *@c) = [1, [2, 3, 4]]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :a), s(:masgn, s(:array, s(:lasgn, :b), s(:splat, s(:iasgn, :@c))))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3), s(:lit, 4))))))
        expect(parse('_, a, *b = [1, 2, 3, 4]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :_), s(:lasgn, :a), s(:splat, s(:lasgn, :b))), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4)))))
        expect(parse('(_, a, *b) = [1, 2, 3, 4]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :_), s(:lasgn, :a), s(:splat, s(:lasgn, :b))), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4)))))
        expect(parse('(a, *) = [1, 2, 3, 4]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :a), s(:splat)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4)))))
        expect(parse('(a, *, c) = [1, 2, 3, 4]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :a), s(:splat), s(:lasgn, :c)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4)))))
        expect(parse('x = foo.bar')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:call, nil, :foo), :bar)))
        expect(parse('x = y = 2')).must_equal s(:block, s(:lasgn, :x, s(:lasgn, :y, s(:lit, 2))))
        expect(parse('x = y = z = 2')).must_equal s(:block, s(:lasgn, :x, s(:lasgn, :y, s(:lasgn, :z, s(:lit, 2)))))
        expect(parse('x = 1, 2')).must_equal s(:block, s(:lasgn, :x, s(:svalue, s(:array, s(:lit, 1), s(:lit, 2)))))
        expect(parse('x, y = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('x, y = 1')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:lit, 1))))
        expect(parse('x = *[1, 2]')).must_equal s(:block, s(:lasgn, :x, s(:svalue, s(:splat, s(:array, s(:lit, 1), s(:lit, 2))))))
        expect(parse('x, y = *[1, 2]')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:splat, s(:array, s(:lit, 1), s(:lit, 2)))))
        expect(parse('::FOO = 1')).must_equal s(:block, s(:cdecl, s(:colon3, :FOO), s(:lit, 1)))
        expect(parse('Foo::BAR = 1')).must_equal s(:block, s(:cdecl, s(:colon2, s(:const, :Foo), :BAR), s(:lit, 1)))
        if parser == 'NatalieParser'
          # We replace :const with :cdecl to be consistent with single-assignment.
          expect(parse('A, B::C = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:cdecl, :A), s(:cdecl, s(:colon2, s(:const, :B), :C))), s(:array, s(:lit, 1), s(:lit, 2))))
          expect(parse('A, ::B = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:cdecl, :A), s(:cdecl, s(:colon3, :B))), s(:array, s(:lit, 1), s(:lit, 2))))
        else
          expect(parse('A, B::C = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:cdecl, :A), s(:const, s(:colon2, s(:const, :B), :C))), s(:array, s(:lit, 1), s(:lit, 2))))
          expect(parse('A, ::B = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:cdecl, :A), s(:const, s(:colon3, :B))), s(:array, s(:lit, 1), s(:lit, 2))))
        end
        expect(parse('a.b, c = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:attrasgn, s(:call, nil, :a), :b=), s(:lasgn, :c)), s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('a, b.c = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :a), s(:attrasgn, s(:call, nil, :b), :c=)), s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('*a, b = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:splat, s(:lasgn, :a)), s(:lasgn, :b)), s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('*a.b, c = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:splat, s(:attrasgn, s(:call, nil, :a), :b=)), s(:lasgn, :c)), s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('a, *b.c = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :a), s(:splat, s(:attrasgn, s(:call, nil, :b), :c=))), s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('*a = 1, 2')).must_equal s(:block, s(:masgn, s(:array, s(:splat, s(:lasgn, :a))), s(:array, s(:lit, 1), s(:lit, 2))))
      end

      it 'parses attr assignment' do
        expect(parse('x.y = 1')).must_equal s(:block, s(:attrasgn, s(:call, nil, :x), :y=, s(:lit, 1)))
        expect(parse('x[y] = 1')).must_equal s(:block, s(:attrasgn, s(:call, nil, :x), :[]=, s(:call, nil, :y), s(:lit, 1)))
        expect(parse('foo[a, b] = :bar')).must_equal s(:block, s(:attrasgn, s(:call, nil, :foo), :[]=, s(:call, nil, :a), s(:call, nil, :b), s(:lit, :bar)))
        expect(parse('foo[] = :bar')).must_equal s(:block, s(:attrasgn, s(:call, nil, :foo), :[]=, s(:lit, :bar)))
        expect(parse('foo.bar=x')).must_equal s(:block, s(:attrasgn, s(:call, nil, :foo), :bar=, s(:call, nil, :x)))
      end

      it 'parses [] as an array vs as a method' do
        expect(parse('foo[1]')).must_equal s(:block, s(:call, s(:call, nil, :foo), :[], s(:lit, 1)))
        expect(parse('foo [1]')).must_equal s(:block, s(:call, nil, :foo, s(:array, s(:lit, 1))))
        expect(parse('foo = []; foo[1]')).must_equal s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
        expect(parse('foo = [1]; foo[1]')).must_equal s(:block, s(:lasgn, :foo, s(:array, s(:lit, 1))), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
        expect(parse('foo = []; foo [1]')).must_equal s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1)))
        expect(parse('foo = []; foo [1, 2]')).must_equal s(:block, s(:lasgn, :foo, s(:array)), s(:call, s(:lvar, :foo), :[], s(:lit, 1), s(:lit, 2)))
        expect(parse('foo[a]')).must_equal s(:block, s(:call, s(:call, nil, :foo), :[], s(:call, nil, :a)))
        expect(parse('foo[]')).must_equal s(:block, s(:call, s(:call, nil, :foo), :[]))
        expect(parse('foo []')).must_equal s(:block, s(:call, nil, :foo, s(:array)))
        expect(parse('foo [a, b]')).must_equal s(:block, s(:call, nil, :foo, s(:array, s(:call, nil, :a), s(:call, nil, :b))))
      end

      it 'parses method definition' do
        expect(parse("def foo\nend")).must_equal s(:block, s(:defn, :foo, s(:args), s(:nil)))
        expect(parse('def foo;end')).must_equal s(:block, s(:defn, :foo, s(:args), s(:nil)))
        expect(parse("def foo\n1\nend")).must_equal s(:block, s(:defn, :foo, s(:args), s(:lit, 1)))
        expect(parse('def foo;1;end')).must_equal s(:block, s(:defn, :foo, s(:args), s(:lit, 1)))
        expect(parse('def foo();1;end')).must_equal s(:block, s(:defn, :foo, s(:args), s(:lit, 1)))
        expect(parse('def foo() 1 end')).must_equal s(:block, s(:defn, :foo, s(:args), s(:lit, 1)))
        expect(parse("def foo;1;2 + 2;'foo';end")).must_equal s(:block, s(:defn, :foo, s(:args), s(:lit, 1), s(:call, s(:lit, 2), :+, s(:lit, 2)), s(:str, 'foo')))
        expect(parse("def foo x, y\nend")).must_equal s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
        expect(parse("def foo x,\ny\nend")).must_equal s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
        expect(parse("def foo(x, y)\nend")).must_equal s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
        expect(parse('def foo(x, y);end')).must_equal s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
        expect(parse("def foo(\nx,\n y\n)\nend")).must_equal s(:block, s(:defn, :foo, s(:args, :x, :y), s(:nil)))
        expect(parse("def foo(x, y)\n1\n2\nend")).must_equal s(:block, s(:defn, :foo, s(:args, :x, :y), s(:lit, 1), s(:lit, 2)))
        expect(parse("def foo n\nn\nend")).must_equal s(:block, s(:defn, :foo, s(:args, :n), s(:lvar, :n)))
        expect(parse("def foo((a, b), c, (d, e))\nend")).must_equal s(:block, s(:defn, :foo, s(:args, s(:masgn, :a, :b), :c, s(:masgn, :d, :e)), s(:nil)))
        expect(parse('def foo!; end')).must_equal s(:block, s(:defn, :foo!, s(:args), s(:nil)))
        expect(parse('def foo?; end')).must_equal s(:block, s(:defn, :foo?, s(:args), s(:nil)))
        expect(parse('def foo=; end')).must_equal s(:block, s(:defn, :foo=, s(:args), s(:nil)))
        expect(parse('def self.foo=; end')).must_equal s(:block, s(:defs, s(:self), :foo=, s(:args), s(:nil)))
        expect(parse('def foo.bar=; end')).must_equal s(:block, s(:defs, s(:call, nil, :foo), :bar=, s(:args), s(:nil)))
        expect(parse('def Foo.foo; end')).must_equal s(:block, s(:defs, s(:const, :Foo), :foo, s(:args), s(:nil)))
        expect(parse('foo=o; def foo.bar; end')).must_equal s(:block, s(:lasgn, :foo, s(:call, nil, :o)), s(:defs, s(:lvar, :foo), :bar, s(:args), s(:nil)))
        expect(parse('def foo(*); end')).must_equal s(:block, s(:defn, :foo, s(:args, :*), s(:nil)))
        expect(parse('def foo(*x); end')).must_equal s(:block, s(:defn, :foo, s(:args, :'*x'), s(:nil)))
        expect(parse('def foo(x, *y, z); end')).must_equal s(:block, s(:defn, :foo, s(:args, :x, :'*y', :z), s(:nil)))
        expect(parse('def foo(a, &b); end')).must_equal s(:block, s(:defn, :foo, s(:args, :a, :'&b'), s(:nil)))
        expect(parse('def foo(a = nil, b = foo, c = FOO); end')).must_equal s(:block, s(:defn, :foo, s(:args, s(:lasgn, :a, s(:nil)), s(:lasgn, :b, s(:call, nil, :foo)), s(:lasgn, :c, s(:const, :FOO))), s(:nil)))
        expect(parse('def foo(a, b: :c, d:); end')).must_equal s(:block, s(:defn, :foo, s(:args, :a, s(:kwarg, :b, s(:lit, :c)), s(:kwarg, :d)), s(:nil)))
        expect(parse('bar def foo() end')).must_equal s(:block, s(:call, nil, :bar, s(:defn, :foo, s(:args), s(:nil))))
      end

      it 'parses operator method definitions' do
        operators = %i[+ - * ** / % == === != =~ !~ > >= < <= <=> & | ^ ~ << >> [] []=]
        operators.each do |operator|
          expect(parse("def #{operator}; end")).must_equal s(:block, s(:defn, operator, s(:args), s(:nil)))
          expect(parse("def #{operator}(x)\nend")).must_equal s(:block, s(:defn, operator, s(:args, :x), s(:nil)))
          expect(parse("def self.#{operator}; end")).must_equal s(:block, s(:defs, s(:self), operator, s(:args), s(:nil)))
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
          expect(parse("def #{keyword}(x); end")).must_equal s(:block, s(:defn, keyword, s(:args, :x), s(:nil)))
          expect(parse("def self.#{keyword} x; end")).must_equal s(:block, s(:defs, s(:self), keyword, s(:args, :x), s(:nil)))
        end
      end

      it 'parses method calls vs local variable lookup' do
        expect(parse('foo')).must_equal s(:block, s(:call, nil, :foo))
        expect(parse('foo?')).must_equal s(:block, s(:call, nil, :foo?))
        expect(parse('foo = 1; foo')).must_equal s(:block, s(:lasgn, :foo, s(:lit, 1)), s(:lvar, :foo))
        expect(parse('foo ||= 1; foo')).must_equal s(:block, s(:op_asgn_or, s(:lvar, :foo), s(:lasgn, :foo, s(:lit, 1))), s(:lvar, :foo))
        expect(parse('foo = 1; def bar; foo; end')).must_equal s(:block, s(:lasgn, :foo, s(:lit, 1)), s(:defn, :bar, s(:args), s(:call, nil, :foo)))
        expect(parse('@foo = 1; foo')).must_equal s(:block, s(:iasgn, :@foo, s(:lit, 1)), s(:call, nil, :foo))
        expect(parse('foo, bar = [1, 2]; foo; bar')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :foo), s(:lasgn, :bar)), s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2)))), s(:lvar, :foo), s(:lvar, :bar))
        expect(parse('(foo, (bar, baz)) = [1, [2, 3]]; foo; bar; baz')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :foo), s(:masgn, s(:array, s(:lasgn, :bar), s(:lasgn, :baz)))), s(:to_ary, s(:array, s(:lit, 1), s(:array, s(:lit, 2), s(:lit, 3))))), s(:lvar, :foo), s(:lvar, :bar), s(:lvar, :baz))
      end

      it 'parses constants' do
        expect(parse('ARGV')).must_equal s(:block, s(:const, :ARGV))
        expect(parse('Foo::Bar')).must_equal s(:block, s(:colon2, s(:const, :Foo), :Bar))
        expect(parse('Foo::Bar::BAZ')).must_equal s(:block, s(:colon2, s(:colon2, s(:const, :Foo), :Bar), :BAZ))
        expect(parse('x, y = ::Bar')).must_equal s(:block, s(:masgn, s(:array, s(:lasgn, :x), s(:lasgn, :y)), s(:to_ary, s(:colon3, :Bar))))
        expect(parse('Foo::bar')).must_equal s(:block, s(:call, s(:const, :Foo), :bar))
        expect(parse('Foo::bar = 1 + 2')).must_equal s(:block, s(:attrasgn, s(:const, :Foo), :bar=, s(:call, s(:lit, 1), :+, s(:lit, 2))))
        expect(parse('Foo::bar x, y')).must_equal s(:block, s(:call, s(:const, :Foo), :bar, s(:call, nil, :x), s(:call, nil, :y)))
        expect(parse('-Foo::BAR')).must_equal s(:block, s(:call, s(:colon2, s(:const, :Foo), :BAR), :-@))
      end

      it 'parses global variables' do
        expect(parse('$foo')).must_equal s(:block, s(:gvar, :$foo))
        expect(parse('$0')).must_equal s(:block, s(:gvar, :$0))
      end

      it 'parses regexp nth refs' do
        expect(parse('$1')).must_equal s(:block, s(:nth_ref, 1))
        expect(parse('$9')).must_equal s(:block, s(:nth_ref, 9))
        expect(parse('$10')).must_equal s(:block, s(:nth_ref, 10))
        expect(parse('$100')).must_equal s(:block, s(:nth_ref, 100))
      end

      it 'parses instance variables' do
        expect(parse('@foo')).must_equal s(:block, s(:ivar, :@foo))
      end

      it 'parses class variables' do
        expect(parse('@@foo')).must_equal s(:block, s(:cvar, :@@foo))
      end

      it 'parses method calls with parentheses' do
        expect(parse('foo()')).must_equal s(:block, s(:call, nil, :foo))
        expect(parse('foo() + bar()')).must_equal s(:block, s(:call, s(:call, nil, :foo), :+, s(:call, nil, :bar)))
        expect(parse("foo(1, 'baz')")).must_equal s(:block, s(:call, nil, :foo, s(:lit, 1), s(:str, 'baz')))
        expect(parse('foo(a, b)')).must_equal s(:block, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)))
        expect(parse('foo(a, b)')).must_equal s(:block, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)))
        expect(parse("foo(a,\nb,\n)")).must_equal s(:block, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)))
        expect(parse("foo(\n1 + 2  ,\n  'baz'  \n )")).must_equal s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, 'baz')))
        expect(parse('foo(1, a: 2)')).must_equal s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2))))
        expect(parse('foo(1, { a: 2 })')).must_equal s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2))))
        expect(parse('foo(1, { a: 2, :b => 3 })')).must_equal s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2), s(:lit, :b), s(:lit, 3))))
        expect(parse('foo(:a, :b)')).must_equal s(:block, s(:call, nil, :foo, s(:lit, :a), s(:lit, :b)))
        expect(parse('foo(a: 1)')).must_equal s(:block, s(:call, nil, :foo, s(:hash, s(:lit, :a), s(:lit, 1))))
        expect(parse("foo(0, a: 1, b: 'two')")).must_equal s(:block, s(:call, nil, :foo, s(:lit, 0), s(:hash, s(:lit, :a), s(:lit, 1), s(:lit, :b), s(:str, 'two'))))
        expect(parse("foo(0, 1 => 2, 3 => 4)")).must_equal s(:block, s(:call, nil, :foo, s(:lit, 0), s(:hash, s(:lit, 1), s(:lit, 2), s(:lit, 3), s(:lit, 4))))
        expect(parse('foo(a, *b, c)')).must_equal s(:block, s(:call, nil, :foo, s(:call, nil, :a), s(:splat, s(:call, nil, :b)), s(:call, nil, :c)))
        expect(parse('b=1; foo(a, *b, c)')).must_equal s(:block, s(:lasgn, :b, s(:lit, 1)), s(:call, nil, :foo, s(:call, nil, :a), s(:splat, s(:lvar, :b)), s(:call, nil, :c)))
        expect(parse('foo.()')).must_equal s(:block, s(:call, s(:call, nil, :foo), :call))
        expect(parse('foo.(1, 2)')).must_equal s(:block, s(:call, s(:call, nil, :foo), :call, s(:lit, 1), s(:lit, 2)))
        expect(parse('foo(a = b, c)')).must_equal s(:block, s(:call, nil, :foo, s(:lasgn, :a, s(:call, nil, :b)), s(:call, nil, :c)))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('foo(') }, SyntaxError, "(string)#1: syntax error, unexpected end-of-input (expected: 'expression')")
        else
          expect_raise_with_message(-> { parse('foo(') }, SyntaxError, '(string):1 :: parse error on value "$" ($end)')
        end
      end

      it 'parses method calls without parentheses' do
        expect(parse('foo + bar')).must_equal s(:block, s(:call, s(:call, nil, :foo), :+, s(:call, nil, :bar)))
        expect(parse("foo 1, 'baz'")).must_equal s(:block, s(:call, nil, :foo, s(:lit, 1), s(:str, 'baz')))
        expect(parse('foo 1 + 2')).must_equal s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2))))
        expect(parse("foo 1 + 2  ,\n  'baz'")).must_equal s(:block, s(:call, nil, :foo, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, 'baz')))
        expect(parse("foo 'foo' + 'bar'  ,\n  2")).must_equal s(:block, s(:call, nil, :foo, s(:call, s(:str, 'foo'), :+, s(:str, 'bar')), s(:lit, 2)))
        expect(parse('foo a, b')).must_equal s(:block, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)))
        expect(parse('foo 1, a: 2')).must_equal s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2))))
        expect(parse('foo 1, :a => 2, b: 3')).must_equal s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2), s(:lit, :b), s(:lit, 3))))
        expect(parse('foo 1, { a: 2 }')).must_equal s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2))))
        expect(parse('foo 1, { a: 2 }')).must_equal s(:block, s(:call, nil, :foo, s(:lit, 1), s(:hash, s(:lit, :a), s(:lit, 2))))
        expect(parse('foo a: 1')).must_equal s(:block, s(:call, nil, :foo, s(:hash, s(:lit, :a), s(:lit, 1))))
        expect(parse("foo 0, a: 1, b: 'two'")).must_equal s(:block, s(:call, nil, :foo, s(:lit, 0), s(:hash, s(:lit, :a), s(:lit, 1), s(:lit, :b), s(:str, 'two'))))
        expect(parse('foo :a, :b')).must_equal s(:block, s(:call, nil, :foo, s(:lit, :a), s(:lit, :b)))
        expect(parse('self.class')).must_equal s(:block, s(:call, s(:self), :class))
        expect(parse('self.begin')).must_equal s(:block, s(:call, s(:self), :begin))
        expect(parse('self.end')).must_equal s(:block, s(:call, s(:self), :end))
        expect(parse('describe :enumeratorize, shared: true')).must_equal s(:block, s(:call, nil, :describe, s(:lit, :enumeratorize), s(:hash, s(:lit, :shared), s(:true))))
        expect(parse("describe :enumeratorize, shared: true do\nnil\nend")).must_equal s(:block, s(:iter, s(:call, nil, :describe, s(:lit, :enumeratorize), s(:hash, s(:lit, :shared), s(:true))), 0, s(:nil)))
        expect(parse('foo a = b, c')).must_equal s(:block, s(:call, nil, :foo, s(:lasgn, :a, s(:call, nil, :b)), s(:call, nil, :c)))
      end

      it 'parses operator method calls' do
        operators = %w[+ - * ** / % == === != =~ !~ > >= < <= <=> & | ^ ~ << >> [] []=]
        operators.each do |operator|
          expect(parse("self.#{operator}")).must_equal s(:block, s(:call, s(:self), operator.to_sym))
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
          expect(parse("foo.#{keyword}(1)")).must_equal s(:block, s(:call, s(:call, nil, :foo), keyword, s(:lit, 1)))
          expect(parse("foo.#{keyword} 1 ")).must_equal s(:block, s(:call, s(:call, nil, :foo), keyword, s(:lit, 1)))
        end
      end

      it 'parses method calls with a receiver' do
        expect(parse('foo.bar')).must_equal s(:block, s(:call, s(:call, nil, :foo), :bar))
        expect(parse('foo.bar.baz')).must_equal s(:block, s(:call, s(:call, s(:call, nil, :foo), :bar), :baz))
        expect(parse('foo.bar 1, 2')).must_equal s(:block, s(:call, s(:call, nil, :foo), :bar, s(:lit, 1), s(:lit, 2)))
        expect(parse('foo.bar(1, 2)')).must_equal s(:block, s(:call, s(:call, nil, :foo), :bar, s(:lit, 1), s(:lit, 2)))
        expect(parse('foo.nil?')).must_equal s(:block, s(:call, s(:call, nil, :foo), :nil?))
        expect(parse('foo.not?')).must_equal s(:block, s(:call, s(:call, nil, :foo), :not?))
        expect(parse('foo.baz?')).must_equal s(:block, s(:call, s(:call, nil, :foo), :baz?))
        expect(parse("foo\n  .bar\n  .baz")).must_equal s(:block, s(:call, s(:call, s(:call, nil, :foo), :bar), :baz))
        expect(parse("foo.\n  bar.\n  baz")).must_equal s(:block, s(:call, s(:call, s(:call, nil, :foo), :bar), :baz))
      end

      it 'parses ternary expressions' do
        expect(parse('1 ? 2 : 3')).must_equal s(:block, s(:if, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
        expect(parse("foo ?\nbar + baz\n :\n buz / 2")).must_equal s(:block, s(:if, s(:call, nil, :foo), s(:call, s(:call, nil, :bar), :+, s(:call, nil, :baz)), s(:call, s(:call, nil, :buz), :/, s(:lit, 2))))
        expect(parse('1 ? 2 : map { |n| n }')).must_equal s(:block, s(:if, s(:lit, 1), s(:lit, 2), s(:iter, s(:call, nil, :map), s(:args, :n), s(:lvar, :n))))
        expect(parse("1 ? 2 : map do |n|\nn\nend")).must_equal s(:block, s(:if, s(:lit, 1), s(:lit, 2), s(:iter, s(:call, nil, :map), s(:args, :n), s(:lvar, :n))))
        expect(parse('fib(num ? num.to_i : 25)')).must_equal s(:block, s(:call, nil, :fib, s(:if, s(:call, nil, :num), s(:call, s(:call, nil, :num), :to_i), s(:lit, 25))))
        # FIXME: precedence problem
        #expect(parse('foo x < 1 ? x : y')).must_equal s(:block, s(:call, nil, :foo, s(:if, s(:call, s(:call, nil, :x), :<, s(:lit, 1)), s(:call, nil, :x), s(:call, nil, :y))))
        #expect(parse('return x < 1 ? x : y')).must_equal s(:block, s(:return, s(:if, s(:call, s(:call, nil, :x), :<, s(:lit, 1)), s(:call, nil, :x), s(:call, nil, :y))))
      end

      it 'parses if/elsif/else' do
        expect(parse('if true; 1; end')).must_equal s(:block, s(:if, s(:true), s(:lit, 1), nil))
        expect(parse('if true; 1; 2; end')).must_equal s(:block, s(:if, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), nil))
        expect(parse('if false; 1; else; 2; end')).must_equal s(:block, s(:if, s(:false), s(:lit, 1), s(:lit, 2)))
        expect(parse('if false; 1; elsif 1 + 1 == 2; 2; else; 3; end')).must_equal s(:block, s(:if, s(:false), s(:lit, 1), s(:if, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 1)), :==, s(:lit, 2)), s(:lit, 2), s(:lit, 3))))
        expect(parse("if false; 1; elsif 1 + 1 == 0; 2; 3; elsif false; 4; elsif foo() == 'bar'; 5; 6; else; 7; end")).must_equal s(:block, s(:if, s(:false), s(:lit, 1), s(:if, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 1)), :==, s(:lit, 0)), s(:block, s(:lit, 2), s(:lit, 3)), s(:if, s(:false), s(:lit, 4), s(:if, s(:call, s(:call, nil, :foo), :==, s(:str, 'bar')), s(:block, s(:lit, 5), s(:lit, 6)), s(:lit, 7))))))
      end

      it 'parses unless' do
        expect(parse('unless false; 1; else; 2; end')).must_equal s(:block, s(:if, s(:false), s(:lit, 2), s(:lit, 1)))
      end

      it 'parses while/until' do
        expect(parse('while true; end')).must_equal s(:block, s(:while, s(:true), nil, true))
        expect(parse('while true; 1; end')).must_equal s(:block, s(:while, s(:true), s(:lit, 1), true))
        expect(parse('while true; 1; 2; end')).must_equal s(:block, s(:while, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), true))
        expect(parse('until true; end')).must_equal s(:block, s(:until, s(:true), nil, true))
        expect(parse('until true; 1; end')).must_equal s(:block, s(:until, s(:true), s(:lit, 1), true))
        expect(parse('until true; 1; 2; end')).must_equal s(:block, s(:until, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), true))
        expect(parse('begin; 1; end while true')).must_equal s(:block, s(:while, s(:true), s(:lit, 1), false))
        expect(parse('begin; 1; 2; end while true')).must_equal s(:block, s(:while, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), false))
        expect(parse('begin; 1; rescue; 2; end while true')).must_equal s(:block, s(:while, s(:true), s(:rescue, s(:lit, 1), s(:resbody, s(:array), s(:lit, 2))), false))
        expect(parse('begin; 1; ensure; 2; end while true')).must_equal s(:block, s(:while, s(:true), s(:ensure, s(:lit, 1), s(:lit, 2)), false))
        expect(parse('begin; 1; end until true')).must_equal s(:block, s(:until, s(:true), s(:lit, 1), false))
        expect(parse('begin; 1; 2; end until true')).must_equal s(:block, s(:until, s(:true), s(:block, s(:lit, 1), s(:lit, 2)), false))
        expect(parse('begin; 1; rescue; 2; end until true')).must_equal s(:block, s(:until, s(:true), s(:rescue, s(:lit, 1), s(:resbody, s(:array), s(:lit, 2))), false))
        expect(parse('begin; 1; ensure; 2; end until true')).must_equal s(:block, s(:until, s(:true), s(:ensure, s(:lit, 1), s(:lit, 2)), false))
        expect(parse('x = 10; x -= 1 until x.zero?')).must_equal s(:block, s(:lasgn, :x, s(:lit, 10)), s(:until, s(:call, s(:lvar, :x), :zero?), s(:lasgn, :x, s(:call, s(:lvar, :x), :-, s(:lit, 1))), true))
        expect(parse('x = 10; x -= 1 while x.positive?')).must_equal s(:block, s(:lasgn, :x, s(:lit, 10)), s(:while, s(:call, s(:lvar, :x), :positive?), s(:lasgn, :x, s(:call, s(:lvar, :x), :-, s(:lit, 1))), true))
      end

      it 'parses post-conditional if/unless' do
        expect(parse('true if true')).must_equal s(:block, s(:if, s(:true), s(:true), nil))
        expect(parse('true unless true')).must_equal s(:block, s(:if, s(:true), nil, s(:true)))
        expect(parse("foo 'hi' if true")).must_equal s(:block, s(:if, s(:true), s(:call, nil, :foo, s(:str, 'hi')), nil))
        expect(parse("foo.bar 'hi' if true")).must_equal s(:block, s(:if, s(:true), s(:call, s(:call, nil, :foo), :bar, s(:str, 'hi')), nil))
      end

      it 'parses true/false/nil' do
        expect(parse('true')).must_equal s(:block, s(:true))
        expect(parse('false')).must_equal s(:block, s(:false))
        expect(parse('nil')).must_equal s(:block, s(:nil))
      end

      it 'parses class definition' do
        expect(parse("class Foo\nend")).must_equal s(:block, s(:class, :Foo, nil))
        expect(parse('class Foo;end')).must_equal s(:block, s(:class, :Foo, nil))
        expect(parse('class FooBar; 1; 2; end')).must_equal s(:block, s(:class, :FooBar, nil, s(:lit, 1), s(:lit, 2)))
        expect_raise_with_message(-> { parse('class foo;end') }, SyntaxError, 'class/module name must be CONSTANT')
        expect(parse("class Foo < Bar; 3\n 4\n end")).must_equal s(:block, s(:class, :Foo, s(:const, :Bar), s(:lit, 3), s(:lit, 4)))
        expect(parse("class Foo < bar; 3\n 4\n end")).must_equal s(:block, s(:class, :Foo, s(:call, nil, :bar), s(:lit, 3), s(:lit, 4)))
        expect(parse('class Foo::Bar; end')).must_equal s(:block, s(:class, s(:colon2, s(:const, :Foo), :Bar), nil))
        expect(parse('class ::Foo; end')).must_equal s(:block, s(:class, s(:colon3, :Foo), nil))
      end

      it 'parses class << self' do
        expect(parse('class Foo; class << self; end; end')).must_equal s(:block, s(:class, :Foo, nil, s(:sclass, s(:self))))
        expect(parse('class Foo; class << Bar; 1; end; end')).must_equal s(:block, s(:class, :Foo, nil, s(:sclass, s(:const, :Bar), s(:lit, 1))))
        expect(parse('class Foo; class << (1 + 1); 1; 2; end; end')).must_equal s(:block, s(:class, :Foo, nil, s(:sclass, s(:call, s(:lit, 1), :+, s(:lit, 1)), s(:lit, 1), s(:lit, 2))))
      end

      it 'parses module definition' do
        expect(parse("module Foo\nend")).must_equal s(:block, s(:module, :Foo))
        expect(parse('module Foo;end')).must_equal s(:block, s(:module, :Foo))
        expect(parse('module FooBar; 1; 2; end')).must_equal s(:block, s(:module, :FooBar, s(:lit, 1), s(:lit, 2)))
        expect_raise_with_message(-> { parse('module foo;end') }, SyntaxError, 'class/module name must be CONSTANT')
        expect(parse('module Foo::Bar; end')).must_equal s(:block, s(:module, s(:colon2, s(:const, :Foo), :Bar)))
        expect(parse('module ::Foo; end')).must_equal s(:block, s(:module, s(:colon3, :Foo)))
      end

      it 'parses array' do
        expect(parse('[]')).must_equal s(:block, s(:array))
        expect(parse('[1]')).must_equal s(:block, s(:array, s(:lit, 1)))
        expect(parse('[1,]')).must_equal s(:block, s(:array, s(:lit, 1)))
        expect(parse("['foo']")).must_equal s(:block, s(:array, s(:str, 'foo')))
        expect(parse('[1, 2, 3]')).must_equal s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
        expect(parse('[1, 2, 3, ]')).must_equal s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
        expect(parse('[x, y, z]')).must_equal s(:block, s(:array, s(:call, nil, :x), s(:call, nil, :y), s(:call, nil, :z)))
        expect(parse("[\n1 , \n2,\n 3]")).must_equal s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
        expect(parse("[\n1 , \n2,\n 3\n]")).must_equal s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
        expect(parse("[\n1 , \n2,\n 3,\n]")).must_equal s(:block, s(:array, s(:lit, 1), s(:lit, 2), s(:lit, 3)))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('[ , 1]') }, SyntaxError, "(string)#1: syntax error, unexpected ',' (expected: 'expression')")
        else
          expect_raise_with_message(-> { parse('[ , 1]') }, SyntaxError, '(string):1 :: parse error on value "," (tCOMMA)')
        end
      end

      it 'parses word array' do
        expect(parse('%w[]')).must_equal s(:block, s(:array))
        expect(parse('%w|1 2 3|')).must_equal s(:block, s(:array, s(:str, '1'), s(:str, '2'), s(:str, '3')))
        expect(parse("%w[  1 2\t  3\n \n4 ]")).must_equal s(:block, s(:array, s(:str, '1'), s(:str, '2'), s(:str, '3'), s(:str, '4')))
        expect(parse("%W[  1 2\t  3\n \n4 ]")).must_equal s(:block, s(:array, s(:str, '1'), s(:str, '2'), s(:str, '3'), s(:str, '4')))
        expect(parse('%i[ foo bar ]')).must_equal s(:block, s(:array, s(:lit, :foo), s(:lit, :bar)))
        expect(parse('%I[ foo bar ]')).must_equal s(:block, s(:array, s(:lit, :foo), s(:lit, :bar)))
      end

      it 'parses hash' do
        expect(parse('{}')).must_equal s(:block, s(:hash))
        expect(parse('{ 1 => 2 }')).must_equal s(:block, s(:hash, s(:lit, 1), s(:lit, 2)))
        expect(parse('{ 1 => 2, }')).must_equal s(:block, s(:hash, s(:lit, 1), s(:lit, 2)))
        expect(parse("{\n 1 => 2,\n }")).must_equal s(:block, s(:hash, s(:lit, 1), s(:lit, 2)))
        expect(parse("{ foo: 'bar' }")).must_equal s(:block, s(:hash, s(:lit, :foo), s(:str, 'bar')))
        expect(parse("{ 1 => 2, 'foo' => 'bar' }")).must_equal s(:block, s(:hash, s(:lit, 1), s(:lit, 2), s(:str, 'foo'), s(:str, 'bar')))
        expect(parse("{\n 1 => \n2,\n 'foo' =>\n'bar'\n}")).must_equal s(:block, s(:hash, s(:lit, 1), s(:lit, 2), s(:str, 'foo'), s(:str, 'bar')))
        expect(parse("{ foo: 'bar', baz: 'buz' }")).must_equal s(:block, s(:hash, s(:lit, :foo), s(:str, 'bar'), s(:lit, :baz), s(:str, 'buz')))
        expect(parse('{ a => b, c => d }')).must_equal s(:block, s(:hash, s(:call, nil, :a), s(:call, nil, :b), s(:call, nil, :c), s(:call, nil, :d)))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('{ , 1 => 2 }') }, SyntaxError, "(string)#1: syntax error, unexpected ',' (expected: 'expression')")
        else
          expect_raise_with_message(-> { parse('{ , 1 => 2 }') }, SyntaxError, '(string):1 :: parse error on value "," (tCOMMA)')
        end
        expect(parse('[1 => 2]')).must_equal s(:block, s(:array, s(:hash, s(:lit, 1), s(:lit, 2))))
        expect(parse('[0, 1 => 2]')).must_equal s(:block, s(:array, s(:lit, 0), s(:hash, s(:lit, 1), s(:lit, 2))))
        expect(parse('[0, foo: "bar"]')).must_equal s(:block, s(:array, s(:lit, 0), s(:hash, s(:lit, :foo), s(:str, 'bar'))))
        expect(parse('bar[1 => 2]')).must_equal s(:block, s(:call, s(:call, nil, :bar), :[], s(:hash, s(:lit, 1), s(:lit, 2))))
        expect(parse('Foo::Bar[1 => 2]')).must_equal s(:block, s(:call, s(:colon2, s(:const, :Foo), :Bar), :[], s(:hash, s(:lit, 1), s(:lit, 2))))
        if parser == 'NatalieParser'
          expect_raise_with_message(-> { parse('[1 => 2, 3]') }, SyntaxError, "(string)#1: syntax error, unexpected ']' (expected: 'hash rocket')")
          expect_raise_with_message(-> { parse('[0, 1 => 2, 3]') }, SyntaxError, "(string)#1: syntax error, unexpected ']' (expected: 'hash rocket')")
        else
          expect_raise_with_message(-> { parse('[1 => 2, 3]') }, SyntaxError, '(string):1 :: parse error on value "]" (tRBRACK)')
          expect_raise_with_message(-> { parse('[0, 1 => 2, 3]') }, SyntaxError, '(string):1 :: parse error on value "]" (tRBRACK)')
        end
      end

      it 'ignores comments' do
        expect(parse('# comment')).must_equal s(:block)
        expect(parse("# comment\n#comment 2")).must_equal s(:block)
        expect(parse('1 + 1 # comment')).must_equal s(:block, s(:call, s(:lit, 1), :+, s(:lit, 1)))
      end

      it 'parses range' do
        expect(parse('1..10')).must_equal s(:block, s(:lit, 1..10))
        expect(parse("1..'a'")).must_equal s(:block, s(:dot2, s(:lit, 1), s(:str, 'a')))
        expect(parse("'a'..1")).must_equal s(:block, s(:dot2, s(:str, 'a'), s(:lit, 1)))
        expect(parse('1...10')).must_equal s(:block, s(:lit, 1...10))
        expect(parse('1.1..2.2')).must_equal s(:block, s(:dot2, s(:lit, 1.1), s(:lit, 2.2)))
        expect(parse('1.1...2.2')).must_equal s(:block, s(:dot3, s(:lit, 1.1), s(:lit, 2.2)))
        expect(parse("'a'..'z'")).must_equal s(:block, s(:dot2, s(:str, 'a'), s(:str, 'z')))
        expect(parse("'a'...'z'")).must_equal s(:block, s(:dot3, s(:str, 'a'), s(:str, 'z')))
        expect(parse("('a')..('z')")).must_equal s(:block, s(:dot2, s(:str, 'a'), s(:str, 'z')))
        expect(parse("('a')...('z')")).must_equal s(:block, s(:dot3, s(:str, 'a'), s(:str, 'z')))
        expect(parse('foo..bar')).must_equal s(:block, s(:dot2, s(:call, nil, :foo), s(:call, nil, :bar)))
        expect(parse('foo...bar')).must_equal s(:block, s(:dot3, s(:call, nil, :foo), s(:call, nil, :bar)))
        expect(parse('foo = 1..10')).must_equal s(:block, s(:lasgn, :foo, s(:lit, 1..10)))
        expect(parse('..3')).must_equal s(:block, s(:dot2, nil, s(:lit, 3)))
        expect(parse('(..3)')).must_equal s(:block, s(:dot2, nil, s(:lit, 3)))
        expect(parse('x = ...3')).must_equal s(:block, s(:lasgn, :x, s(:dot3, nil, s(:lit, 3))))
        expect(parse('4..')).must_equal s(:block, s(:dot2, s(:lit, 4), nil))
        expect(parse('(4..)')).must_equal s(:block, s(:dot2, s(:lit, 4), nil))
        expect(parse("4..\n5")).must_equal s(:block, s(:lit, 4..5))
        expect(parse("4..\nfoo")).must_equal s(:block, s(:dot2, s(:lit, 4), s(:call, nil, :foo)))
        expect(parse('(4..) * 5')).must_equal s(:block, s(:call, s(:dot2, s(:lit, 4), nil), :*, s(:lit, 5)))
        expect(parse('x = (4..)')).must_equal s(:block, s(:lasgn, :x, s(:dot2, s(:lit, 4), nil)))
        expect(parse("ruby_version_is ''...'3.0' do\nend")).must_equal s(:block, s(:iter, s(:call, nil, :ruby_version_is, s(:dot3, s(:str, ''), s(:str, '3.0'))), 0))
      end

      it 'parses return' do
        expect(parse('return')).must_equal s(:block, s(:return))
        expect(parse('return foo')).must_equal s(:block, s(:return, s(:call, nil, :foo)))
        expect(parse('return 1, 2')).must_equal s(:block, s(:return, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('return foo if true')).must_equal s(:block, s(:if, s(:true), s(:return, s(:call, nil, :foo)), nil))
      end

      it 'parses block' do
        expect(parse("foo do\nend")).must_equal s(:block, s(:iter, s(:call, nil, :foo), 0))
        expect(parse("foo do\n1\n2\nend")).must_equal s(:block, s(:iter, s(:call, nil, :foo), 0, s(:block, s(:lit, 1), s(:lit, 2))))
        expect(parse("foo do |x, y|\nx\ny\nend")).must_equal s(:block, s(:iter, s(:call, nil, :foo), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
        expect(parse("foo(a, b) do |x, y|\nx\ny\nend")).must_equal s(:block, s(:iter, s(:call, nil, :foo, s(:call, nil, :a), s(:call, nil, :b)), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
        expect(parse("super do |x| x end")).must_equal s(:block, s(:iter, s(:zsuper), s(:args, :x), s(:lvar, :x)))
        expect(parse("super() do |x| x end")).must_equal s(:block, s(:iter, s(:super), s(:args, :x), s(:lvar, :x)))
        expect(parse("super(a, b) do |x| x end")).must_equal s(:block, s(:iter, s(:super, s(:call, nil, :a), s(:call, nil, :b)), s(:args, :x), s(:lvar, :x)))
        expect(parse('foo { }')).must_equal s(:block, s(:iter, s(:call, nil, :foo), 0))
        expect(parse('foo { 1; 2 }')).must_equal s(:block, s(:iter, s(:call, nil, :foo), 0, s(:block, s(:lit, 1), s(:lit, 2))))
        expect(parse('foo { |x, y| x; y }')).must_equal s(:block, s(:iter, s(:call, nil, :foo), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
        expect(parse('foo { |x| x }; x')).must_equal s(:block, s(:iter, s(:call, nil, :foo), s(:args, :x), s(:lvar, :x)), s(:call, nil, :x))
        expect(parse('x = 1; foo { x }; x')).must_equal s(:block, s(:lasgn, :x, s(:lit, 1)), s(:iter, s(:call, nil, :foo), 0, s(:lvar, :x)), s(:lvar, :x))
        expect(parse('super { |x| x }; x')).must_equal s(:block, s(:iter, s(:zsuper), s(:args, :x), s(:lvar, :x)), s(:call, nil, :x))
        expect(parse('super() { |x| x }; x')).must_equal s(:block, s(:iter, s(:super), s(:args, :x), s(:lvar, :x)), s(:call, nil, :x))
        expect(parse('super(a) { |x| x }; x')).must_equal s(:block, s(:iter, s(:super, s(:call, nil, :a)), s(:args, :x), s(:lvar, :x)), s(:call, nil, :x))
        expect(parse("foo do\nbar do\nend\nend")).must_equal s(:block, s(:iter, s(:call, nil, :foo), 0, s(:iter, s(:call, nil, :bar), 0)))
        expect(parse("foo do |(x, y), z|\nend")).must_equal s(:block, s(:iter, s(:call, nil, :foo), s(:args, s(:masgn, :x, :y), :z)))
        expect(parse("foo do |(x, (y, z))|\nend")).must_equal s(:block, s(:iter, s(:call, nil, :foo), s(:args, s(:masgn, :x, s(:masgn, :y, :z)))))
        expect(parse('x = foo.bar { |y| y }')).must_equal s(:block, s(:lasgn, :x, s(:iter, s(:call, s(:call, nil, :foo), :bar), s(:args, :y), s(:lvar, :y))))
        expect(parse('bar { |*| 1 }')).must_equal s(:block, s(:iter, s(:call, nil, :bar), s(:args, :*), s(:lit, 1)))
        expect(parse('bar { |*x| x }')).must_equal s(:block, s(:iter, s(:call, nil, :bar), s(:args, :'*x'), s(:lvar, :x)))
        expect(parse('bar { |x, *y, z| y }')).must_equal s(:block, s(:iter, s(:call, nil, :bar), s(:args, :x, :'*y', :z), s(:lvar, :y)))
        expect(parse('bar { |a = nil, b = foo, c = FOO| b }')).must_equal s(:block, s(:iter, s(:call, nil, :bar), s(:args, s(:lasgn, :a, s(:nil)), s(:lasgn, :b, s(:call, nil, :foo)), s(:lasgn, :c, s(:const, :FOO))), s(:lvar, :b)))
        expect(parse('bar { |a, b: :c, d:| a }')).must_equal s(:block, s(:iter, s(:call, nil, :bar), s(:args, :a, s(:kwarg, :b, s(:lit, :c)), s(:kwarg, :d)), s(:lvar, :a)))
        expect(parse("get 'foo', bar { 'baz' }")).must_equal s(:block, s(:call, nil, :get, s(:str, 'foo'), s(:iter, s(:call, nil, :bar), 0, s(:str, 'baz'))))
        expect(parse("get 'foo', bar do\n'baz'\nend")).must_equal s(:block, s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz')))
        expect(parse("foo1 = foo do\n'foo'\nend")).must_equal s(:block, s(:lasgn, :foo1, s(:iter, s(:call, nil, :foo), 0, s(:str, 'foo'))))
        expect(parse("@foo = foo do\n'foo'\nend")).must_equal s(:block, s(:iasgn, :@foo, s(:iter, s(:call, nil, :foo), 0, s(:str, 'foo'))))
        expect(parse("@foo ||= foo do\n'foo'\nend")).must_equal s(:block, s(:op_asgn_or, s(:ivar, :@foo), s(:iasgn, :@foo, s(:iter, s(:call, nil, :foo), 0, s(:str, 'foo')))))
        expect(parse("get 'foo', bar do 'baz'\n end")).must_equal s(:block, s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz')))
        expect(parse("get 'foo', bar do\n 'baz' end")).must_equal s(:block, s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz')))
        expect(parse("get 'foo', bar do 'baz' end")).must_equal s(:block, s(:iter, s(:call, nil, :get, s(:str, 'foo'), s(:call, nil, :bar)), 0, s(:str, 'baz')))
        expect(parse("foo += bar { |x| x }")).must_equal s(:block, s(:lasgn, :foo, s(:call, s(:lvar, :foo), :+, s(:iter, s(:call, nil, :bar), s(:args, :x), s(:lvar, :x)))))
        if parser == 'NatalieParser'
          # I don't like how the RubyParser gem appends a nil on the args array; there's no reason for it.
          expect(parse('bar { |a, | a }')).must_equal s(:block, s(:iter, s(:call, nil, :bar), s(:args, :a), s(:lvar, :a)))
        else
          expect(parse('bar { |a, | a }')).must_equal s(:block, s(:iter, s(:call, nil, :bar), s(:args, :a, nil), s(:lvar, :a)))
        end
      end

      it 'parses block pass (ampersand operator)' do
        expect(parse('map(&:foo)')).must_equal s(:block, s(:call, nil, :map, s(:block_pass, s(:lit, :foo))))
        expect(parse('map(&myblock)')).must_equal s(:block, s(:call, nil, :map, s(:block_pass, s(:call, nil, :myblock))))
        expect(parse('map(&nil)')).must_equal s(:block, s(:call, nil, :map, s(:block_pass, s(:nil))))
      end

      it 'parses break, next, super, and yield' do
        expect(parse('break')).must_equal s(:block, s(:break))
        expect(parse('break()')).must_equal s(:block, s(:break, s(:nil)))
        expect(parse('break 1')).must_equal s(:block, s(:break, s(:lit, 1)))
        expect(parse('break 1, 2')).must_equal s(:block, s(:break, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('break([1, 2])')).must_equal s(:block, s(:break, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('break if true')).must_equal s(:block, s(:if, s(:true), s(:break), nil))
        expect(parse('next')).must_equal s(:block, s(:next))
        expect(parse('next()')).must_equal s(:block, s(:next, s(:nil)))
        expect(parse('next 1')).must_equal s(:block, s(:next, s(:lit, 1)))
        expect(parse('next 1, 2')).must_equal s(:block, s(:next, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('next([1, 2])')).must_equal s(:block, s(:next, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('next if true')).must_equal s(:block, s(:if, s(:true), s(:next), nil))
        expect(parse('super')).must_equal s(:block, s(:zsuper))
        expect(parse('super()')).must_equal s(:block, s(:super))
        expect(parse('super 1')).must_equal s(:block, s(:super, s(:lit, 1)))
        expect(parse('super 1, 2')).must_equal s(:block, s(:super, s(:lit, 1), s(:lit, 2)))
        expect(parse('super([1, 2])')).must_equal s(:block, s(:super, s(:array, s(:lit, 1), s(:lit, 2))))
        expect(parse('super if true')).must_equal s(:block, s(:if, s(:true), s(:zsuper), nil))
        expect(parse('yield')).must_equal s(:block, s(:yield))
        expect(parse('yield()')).must_equal s(:block, s(:yield))
        expect(parse('yield 1')).must_equal s(:block, s(:yield, s(:lit, 1)))
        expect(parse('yield 1, 2')).must_equal s(:block, s(:yield, s(:lit, 1), s(:lit, 2)))
        expect(parse('yield(1, 2)')).must_equal s(:block, s(:yield, s(:lit, 1), s(:lit, 2)))
        expect(parse('yield if true')).must_equal s(:block, s(:if, s(:true), s(:yield), nil))
        expect(parse('x += yield y')).must_equal s(:block, s(:lasgn, :x, s(:call, s(:lvar, :x), :+, s(:yield, s(:call, nil, :y)))))
      end

      it 'parses self' do
        expect(parse('self')).must_equal s(:block, s(:self))
      end

      it 'parses __FILE__ and __dir__' do
        expect(parse('__FILE__', 'foo/bar.rb')).must_equal s(:block, s(:str, 'foo/bar.rb'))
        expect(parse('__dir__')).must_equal s(:block, s(:call, nil, :__dir__))
      end

      it 'parses splat *' do
        expect(parse('def foo(*args); end')).must_equal s(:block, s(:defn, :foo, s(:args, :'*args'), s(:nil)))
        expect(parse('def foo *args; end')).must_equal s(:block, s(:defn, :foo, s(:args, :'*args'), s(:nil)))
        expect(parse('foo(*args)')).must_equal s(:block, s(:call, nil, :foo, s(:splat, s(:call, nil, :args))))
      end

      it 'parses keyword splat *' do
        expect(parse('def foo(**kwargs); end')).must_equal s(:block, s(:defn, :foo, s(:args, :'**kwargs'), s(:nil)))
        expect(parse('def foo **kwargs; end')).must_equal s(:block, s(:defn, :foo, s(:args, :'**kwargs'), s(:nil)))
        expect(parse('foo(**kwargs)')).must_equal s(:block, s(:call, nil, :foo, s(:hash, s(:kwsplat, s(:call, nil, :kwargs)))))
      end

      it 'parses stabby proc' do
        expect(parse('-> { puts 1 }')).must_equal s(:block, s(:iter, s(:lambda), 0, s(:call, nil, :puts, s(:lit, 1))))
        expect(parse('-> x { puts x }')).must_equal s(:block, s(:iter, s(:lambda), s(:args, :x), s(:call, nil, :puts, s(:lvar, :x))))
        expect(parse('-> x, y { puts x, y }')).must_equal s(:block, s(:iter, s(:lambda), s(:args, :x, :y), s(:call, nil, :puts, s(:lvar, :x), s(:lvar, :y))))
        expect(parse('-> (x, y) { x; y }')).must_equal s(:block, s(:iter, s(:lambda), s(:args, :x, :y), s(:block, s(:lvar, :x), s(:lvar, :y))))
        expect(-> { parse('->') }).must_raise(SyntaxError)
        expect(parse('foo -> { x } do y end')).must_equal s(:block, s(:iter, s(:call, nil, :foo, s(:iter, s(:lambda), 0, s(:call, nil, :x))), 0, s(:call, nil, :y)))
      end

      it 'parses case/when/else' do
        expect(parse("case 1\nwhen 1\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a)), nil))
        expect(parse("case 1\nwhen 1 then :a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a)), nil))
        expect(parse("case 1\nwhen 1\n:a\n:b\nwhen 2, 3\n:c\nelse\n:d\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a), s(:lit, :b)), s(:when, s(:array, s(:lit, 2), s(:lit, 3)), s(:lit, :c)), s(:lit, :d)))
        expect(parse("case 1\nwhen 1 then :a\n:b\nwhen 2, 3 then :c\nelse :d\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:when, s(:array, s(:lit, 1)), s(:lit, :a), s(:lit, :b)), s(:when, s(:array, s(:lit, 2), s(:lit, 3)), s(:lit, :c)), s(:lit, :d)))
        expect(parse("case\nwhen true\n:a\nelse\n:b\nend")).must_equal s(:block, s(:case, nil, s(:when, s(:array, s(:true)), s(:lit, :a)), s(:lit, :b)))
        expect(parse("case\nwhen true then :a\nelse :b\nend")).must_equal s(:block, s(:case, nil, s(:when, s(:array, s(:true)), s(:lit, :a)), s(:lit, :b)))
      end

      it 'parses case/in/else' do
        expect(parse("case 1\nin x\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:lvar, :x), s(:lit, :a)), nil))
        expect(parse("case 1\nin x then :a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:lvar, :x), s(:lit, :a)), nil))
        expect(parse("case 1\nin x | y\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:or, s(:lvar, :x), s(:lvar, :y)), s(:lit, :a)), nil))
        expect(parse("case 1\nin []\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:array_pat), s(:lit, :a)), nil))
        expect(parse("case 1\nin [ ]\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:array_pat), s(:lit, :a)), nil))
        expect(parse("case 1\nin [:x, x]\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lit, :x), s(:lvar, :x)), s(:lit, :a)), nil))
        expect(parse("case 1\nin [a, a]\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lvar, :a), s(:lvar, :a)), s(:lit, :a)), nil))
        if parser == 'NatalieParser'
          # pinned variables not supported in ruby_parser yet
          expect(parse("case 1\nin [^a, a]\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:pin, s(:lvar, :a)), s(:lvar, :a)), s(:lit, :a)), nil))
        end
        expect(parse("case 1\nin [1, x]\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lit, 1), s(:lvar, :x)), s(:lit, :a)), nil))
        expect(parse("case 1\nin [1.2, x]\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:lit, 1.2), s(:lvar, :x)), s(:lit, :a)), nil))
        expect(parse("case 1\nin ['one', x]\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:array_pat, nil, s(:str, 'one'), s(:lvar, :x)), s(:lit, :a)), nil))
        expect(parse("case 1\nin [1, 2] => a\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:lasgn, :a, s(:array_pat, nil, s(:lit, 1), s(:lit, 2))), s(:lit, :a)), nil))
        expect(parse("case 1\nin [1 => a] => b\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:lasgn, :b, s(:array_pat, nil, s(:lasgn, :a, s(:lit, 1)))), s(:lit, :a)), nil))
        expect(parse("case 1\nin {}\n:a\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil), s(:lit, :a)), nil))
        expect(parse("case 1\nin { x: x }\nx\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:hash_pat, nil, s(:lit, :x), s(:lvar, :x)), s(:call, nil, :x)), nil))
        expect(parse("case 1\nin { x: [:a, a] => b } => y\nx\nend")).must_equal s(:block, s(:case, s(:lit, 1), s(:in, s(:lasgn, :y, s(:hash_pat, nil, s(:lit, :x), s(:lasgn, :b, s(:array_pat, nil, s(:lit, :a), s(:lvar, :a))))), s(:call, nil, :x)), nil))
      end

      it 'parses begin/rescue/else/ensure' do
        expect(parse('begin;1;2;rescue;3;4;end')).must_equal s(:block, s(:rescue, s(:block, s(:lit, 1), s(:lit, 2)), s(:resbody, s(:array), s(:lit, 3), s(:lit, 4))))
        expect(parse('begin;1;rescue => e;e;end')).must_equal s(:block, s(:rescue, s(:lit, 1), s(:resbody, s(:array, s(:lasgn, :e, s(:gvar, :$!))), s(:lvar, :e))))
        expect(parse('begin;rescue SyntaxError, NoMethodError => e;3;end')).must_equal s(:block, s(:rescue, s(:resbody, s(:array, s(:const, :SyntaxError), s(:const, :NoMethodError), s(:lasgn, :e, s(:gvar, :$!))), s(:lit, 3))))
        expect(parse('begin;rescue SyntaxError;3;else;4;5;end')).must_equal s(:block, s(:rescue, s(:resbody, s(:array, s(:const, :SyntaxError)), s(:lit, 3)), s(:block, s(:lit, 4), s(:lit, 5))))
        expect(parse("begin\n0\nensure\n:a\n:b\nend")).must_equal s(:block, s(:ensure, s(:lit, 0), s(:block, s(:lit, :a), s(:lit, :b))))
        expect(parse('begin;0;rescue;:a;else;:c;ensure;:d;:e;end')).must_equal s(:block, s(:ensure, s(:rescue, s(:lit, 0), s(:resbody, s(:array), s(:lit, :a)), s(:lit, :c)), s(:block, s(:lit, :d), s(:lit, :e))))
        expect(parse('def foo;0;rescue;:a;else;:c;ensure;:d;:e;end')).must_equal s(:block, s(:defn, :foo, s(:args), s(:ensure, s(:rescue, s(:lit, 0), s(:resbody, s(:array), s(:lit, :a)), s(:lit, :c)), s(:block, s(:lit, :d), s(:lit, :e)))))
        expect(parse('begin;0;rescue foo(1), bar(2);1;end')).must_equal s(:block, s(:rescue, s(:lit, 0), s(:resbody, s(:array, s(:call, nil, :foo, s(:lit, 1)), s(:call, nil, :bar, s(:lit, 2))), s(:lit, 1))))
        expect(parse('begin;0;ensure;1;end')).must_equal s(:block, s(:ensure, s(:lit, 0), s(:lit, 1)))
      end

      it 'parses backticks and %x()' do
        expect(parse('`ls`')).must_equal s(:block, s(:xstr, 'ls'))
        expect(parse('%x(ls)')).must_equal s(:block, s(:xstr, 'ls'))
        expect(parse("%x(ls \#{path})")).must_equal s(:block, s(:dxstr, 'ls ', s(:evstr, s(:call, nil, :path))))
      end

      it 'parses alias' do
        expect(parse('alias foo bar')).must_equal s(:block, s(:alias, s(:lit, :foo), s(:lit, :bar)))
        expect(parse('alias :foo :bar')).must_equal s(:block, s(:alias, s(:lit, :foo), s(:lit, :bar)))
        expect(parse("alias write <<\ndef foo; end")).must_equal s(:block, s(:alias, s(:lit, :write), s(:lit, :<<)), s(:defn, :foo, s(:args), s(:nil)))
        expect(parse("alias << write\ndef foo; end")).must_equal s(:block, s(:alias, s(:lit, :<<), s(:lit, :write)), s(:defn, :foo, s(:args), s(:nil)))
        expect(parse("alias yield <<\ndef foo; end")).must_equal s(:block, s(:alias, s(:lit, :yield), s(:lit, :<<)), s(:defn, :foo, s(:args), s(:nil)))
        expect(parse("alias << yield\ndef foo; end")).must_equal s(:block, s(:alias, s(:lit, :<<), s(:lit, :yield)), s(:defn, :foo, s(:args), s(:nil)))
      end

      it 'parses defined?' do
        expect(parse('defined? foo')).must_equal s(:block, s(:defined, s(:call, nil, :foo)))
        expect(parse('defined?(:foo)')).must_equal s(:block, s(:defined, s(:lit, :foo)))
      end

      it 'parses logical and/or with block' do
        result = parse('x = foo.bar || ->(i) { baz(i) }')
        expect(result).must_equal s(:block, s(:lasgn, :x, s(:or, s(:call, s(:call, nil, :foo), :bar), s(:iter, s(:lambda), s(:args, :i), s(:call, nil, :baz, s(:lvar, :i))))))
        result = parse('x = foo.bar || ->(i) do baz(i) end')
        expect(result).must_equal s(:block, s(:lasgn, :x, s(:or, s(:call, s(:call, nil, :foo), :bar), s(:iter, s(:lambda), s(:args, :i), s(:call, nil, :baz, s(:lvar, :i))))))
        result = parse('x = foo.bar && ->(i) { baz(i) }')
        expect(result).must_equal s(:block, s(:lasgn, :x, s(:and, s(:call, s(:call, nil, :foo), :bar), s(:iter, s(:lambda), s(:args, :i), s(:call, nil, :baz, s(:lvar, :i))))))
        result = parse('x = foo.bar && ->(i) do baz(i) end')
        expect(result).must_equal s(:block, s(:lasgn, :x, s(:and, s(:call, s(:call, nil, :foo), :bar), s(:iter, s(:lambda), s(:args, :i), s(:call, nil, :baz, s(:lvar, :i))))))
      end

      it 'parses heredocs' do
        expect(parse("<<FOO\nFOO")).must_equal s(:block, s(:str, ""))
        doc1 = <<END
foo = <<FOO_BAR
 1
2
FOO_BAR
END
        expect(parse(doc1)).must_equal s(:block, s(:lasgn, :foo, s(:str, " 1\n2\n")))
        doc2 = <<END
foo(1, <<-foo, 2)
 1
2
  foo
END
        expect(parse(doc2)).must_equal s(:block, s(:call, nil, :foo, s(:lit, 1), s(:str, " 1\n2\n"), s(:lit, 2)))
        doc3 = <<END
<<FOO
  \#{1+1}
FOO
END
        expect(parse(doc3)).must_equal s(:block, s(:dstr, '  ', s(:evstr, s(:call, s(:lit, 1), :+, s(:lit, 1))), s(:str, "\n")))
        doc4 = <<END
<<-BAR
FOOBAR
  BAR
END
        expect(parse(doc4)).must_equal s(:block, s(:str, "FOOBAR\n"))
      end

      it 'tracks file and line/column number' do
        ast = parse("1 +\n\n    2", 'foo.rb')
        expect(ast.file).must_equal('foo.rb')
        expect(ast.line).must_equal(1)
        expect(ast.column).must_equal(1) if parser == 'NatalieParser'

        two = ast.last.last
        expect(two).must_equal s(:lit, 2)
        expect(two.file).must_equal('foo.rb')
        expect(two.line).must_equal(3)
        expect(two.column).must_equal(5) if parser == 'NatalieParser'
      end
    end
  end
end
