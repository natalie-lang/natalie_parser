require 'mkmf'
$CXXFLAGS += ' -Wno-deprecated-declarations -std=c++17'
$INCFLAGS += ' -I ../../include'
$LDFLAGS += ' -L ../../build'
$LIBS += ' -lnatalie_parser'
create_header
create_makefile 'parser_c_ext'
