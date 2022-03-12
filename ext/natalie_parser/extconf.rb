require 'mkmf'
$CXXFLAGS += ' -std=c++17'
$INCFLAGS += ' -I ../../include'
$LDFLAGS += ' -L ../../build'
$LIBS += ' -lnatalie_parser'
create_header
create_makefile 'natalie_parser'
