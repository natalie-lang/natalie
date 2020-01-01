#ifndef __NAT_EXCEPTION__
#define __NAT_EXCEPTION__

#include "natalie.h"

NatObject *Exception_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Exception_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Exception_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Exception_message(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *Exception_backtrace(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

#endif
