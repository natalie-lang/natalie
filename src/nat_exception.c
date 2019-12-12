#include "natalie.h"
#include "nat_exception.h"
#include "nat_class.h"

NatObject *Exception_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(self->type == NAT_VALUE_EXCEPTION);
    NatObject *str = nat_string(env, "#<");
    assert(self->class);
    nat_string_append(str, Class_inspect(env, self->class, 0, NULL, NULL, NULL)->str);
    nat_string_append(str, ": ");
    nat_string_append(str, self->message);
    nat_string_append_char(str, '>');
    return str;
}
