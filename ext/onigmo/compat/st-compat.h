/* Internal st.c declarations normally supplied by MRI's internal headers. */
#ifndef CORA_ONIGMO_ST_COMPAT_H
#define CORA_ONIGMO_ST_COMPAT_H

#include "sanitizers.h"

static inline unsigned int
nlz_intptr(uintptr_t value)
{
    return value ? (unsigned int)__builtin_clzll(value) -
        (unsigned int)((sizeof(unsigned long long) - sizeof(uintptr_t)) * CHAR_BIT) :
        (unsigned int)(sizeof(uintptr_t) * CHAR_BIT);
}

typedef int st_foreach_with_hash_callback_func(st_data_t, st_data_t, st_data_t, st_data_t);
void st_free_embedded_table(st_table *tab);

#endif
