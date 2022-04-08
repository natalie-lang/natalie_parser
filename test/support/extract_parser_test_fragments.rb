require 'natalie_parser'

rb = File.read(File.expand_path('../parser_test.rb', __dir__))
ast = NatalieParser.parse(rb)
pp ast
