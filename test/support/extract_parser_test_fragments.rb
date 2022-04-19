require 'natalie_parser'

def find_fragments(node)
  ary = []
  if node.is_a?(Sexp)
    if node[0] == :call && node[1] == nil && %i[expect expect_raise_with_message].include?(node[2]) && node[3].is_a?(Sexp) && node[3].sexp_type == :iter
      # SKIP
      # expect(-> { ... }).must_raise ...
    elsif node[0] == :call && node[1] == nil && node[2] == :parse
      arg = node[3]
      if arg.sexp_type == :str
        ary << arg
      else
        # FIXME: would need to actually execute parser_test.rb to get some of these
      end
    else
      ary += node.flat_map { |n| find_fragments(n) }
    end
  end
  ary
end

build_path = File.expand_path('../../build', __dir__)

rb = File.read(File.expand_path('../parser_test.rb', __dir__))
fragments = find_fragments(NatalieParser.parse(rb))
File.open(File.join(build_path, 'fragments.hpp'), 'w') do |file|
  file.puts '#include "tm/vector.hpp"'
  file.puts '#include "tm/string.hpp"'
  file.puts
  file.puts 'TM::Vector<TM::String> *build_fragments() {'
  file.puts '  auto vec = new TM::Vector<TM::String> {};'
  fragments.each do |node|
    file.puts "  vec->push(#{node[1].inspect.gsub(/\\#/, '#')});"
  end
  file.puts '  return vec;'
  file.puts '}'
end
