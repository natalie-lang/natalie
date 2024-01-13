#pragma once

#ifndef TM_CALLOC
#pragma GCC error "Added just for Natalie so we don't forget to set these macros."
#define TM_CALLOC calloc
#define TM_REALLOC realloc
#define TM_FREE free
#endif
