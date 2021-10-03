require 'mkmf'
$CXXFLAGS += " -Wno-deprecated-declarations -std=c++17"
$INCFLAGS += " -I ../../include"
$INCFLAGS += " -I ../../build/onigmo/include"
$LDFLAGS += " -L ../../build"
$LIBS += ' -lnatalie'
$srcs = ["parser.cpp"]
$objs = ["parser.o", "../../build/generated/sexp.rb.o"]
create_header
create_makefile 'parser'
