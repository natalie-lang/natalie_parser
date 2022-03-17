lib = File.expand_path('lib', __dir__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'natalie_parser/version'

Gem::Specification.new do |spec|
  spec.name          = 'natalie_parser'
  spec.version       = NatalieParser::VERSION
  spec.authors       = ['Tim Morgan']
  spec.email         = ['tim@timmorgan.org']

  spec.summary       = 'A Parser for the Ruby Programming Language'
  spec.description   = 'NatalieParser is a zero-dependency, from-scratch, hand-written recursive descent parser for the Ruby Programming Language.'
  spec.homepage      = 'https://github.com/natalie-lang/natalie_parser'
  spec.license       = 'MIT'

  spec.files         = Dir.chdir(File.expand_path(__dir__)) do
    `git ls-files`.split("\n").reject { |f| f.match(%r{^(test|\.)}) }
  end

  spec.require_paths = ['lib', 'ext']
  spec.extensions = %w[ext/natalie_parser/extconf.rb]
end

