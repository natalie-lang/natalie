require 'mkmf'
$CXXFLAGS += ' -Wno-deprecated-declarations -std=c++17'
$INCFLAGS += ' -I ../../include'
$INCFLAGS += ' -I ../../build/onigmo/include'
$LDFLAGS += ' -L ../../build'
$LIBS += ' -lnatalie'
create_header
create_makefile 'parser_c_ext'
