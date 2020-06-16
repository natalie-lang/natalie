#pragma once

#ifdef NAT_GC_DISABLE
class gc { };
#else
#include <gc_cpp.h>
#endif
