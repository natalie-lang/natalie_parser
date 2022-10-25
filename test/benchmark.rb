# This benchmark was made in good faith. If it seems that I have used
# one of these libraries in a way that impairs their performance, I
# promise it was out of ignorance.
#
# I did not build NatalieParser with the intention of it being fast,
# so I didn't have any expectations that it would be. That said, it
# does **appear** to be faster than many of the other Ruby parsers,
# possibly by sheer dumb luck. :-)
#
#     rake benchmark
#                           user     system      total        real
#     RubyParser        5.404129   0.027824   5.431953 (  5.433486)
#     Parser            4.412873   0.000000   4.412873 (  4.412874)
#     SyntaxTree        2.299520   0.000000   2.299520 (  2.299575)
#     NatalieParser     0.917792   0.000000   0.917792 (  0.917849)

require 'bundler/inline'

gemfile do
  source 'https://rubygems.org'
  gem 'ruby_parser'
  gem 'parser'
  gem 'syntax_tree'
end

require 'benchmark'
require 'ruby_parser'
require 'parser/current'
require 'syntax_tree'

$LOAD_PATH << File.expand_path('../lib', __dir__)
$LOAD_PATH << File.expand_path('../ext', __dir__)

source_path = File.expand_path('./boardslam.rb', __dir__)
source = File.read(source_path)
ruby_parser = RubyParser.new
parser = Parser::CurrentRuby

iterations = 1000

Benchmark.bm(25) do |x|
  x.report("RubyParser #{RubyParserStuff::VERSION}") { iterations.times { ruby_parser.parse(source) } }
  x.report("Parser #{Parser::VERSION}")              { iterations.times { parser.parse(source) } }
  x.report("SyntaxTree #{SyntaxTree::VERSION}")      { iterations.times { SyntaxTree.parse(source) } }

  # we have to load this here because our Sexp overwrites RubyParser's
  # and breaks RubyParser :-(
  require 'natalie_parser'
  x.report("NatalieParser #{NatalieParser::VERSION}") { iterations.times { NatalieParser.parse(source) } }
end
