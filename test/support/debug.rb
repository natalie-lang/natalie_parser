# Use this for debugging with gdb, e.g.:
#
#     gdb $(rbenv which ruby)
#     > run test/support/debug.rb "1+1"
#
$LOAD_PATH << 'lib'
$LOAD_PATH << 'ext'
require 'natalie_parser'
p NatalieParser.parse(ARGV.first)
