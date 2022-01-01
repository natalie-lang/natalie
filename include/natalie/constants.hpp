// This is used at build time as well in order to generate numbers.rb
#include <limits.h>
#include <math.h>

#define NAT_MAX_FIXNUM LLONG_MAX
#define NAT_MIN_FIXNUM LLONG_MIN + 1

const long long NAT_MAX_FIXNUM_SQRT = (long long)sqrt((double)NAT_MAX_FIXNUM);
