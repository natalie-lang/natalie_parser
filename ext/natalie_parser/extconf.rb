require 'mkmf'
$CXXFLAGS += ' -std=c++17'
$INCFLAGS += ' -I ../../include'
$srcs = Dir['../../src/**/*.cpp', 'natalie_parser.cpp']
$VPATH << "$(srcdir)/../../src"
$VPATH << "$(srcdir)/../../src/node"
create_header
create_makefile 'natalie_parser'
