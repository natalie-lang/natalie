#pragma once

#include "natalie/forward.hpp"
#include "natalie/hash_object.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/io_object.hpp"
#include "natalie/regexp_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

class EnvObject : public Object {
public:
    Value assoc(Env *, Value name);
    Value clear(Env *);
    Value delete_key(Env *, Value name, Block *block);
    Value dup(Env *);
    Value each(Env *, Block *block);
    Value each_key(Env *, Block *);
    Value each_value(Env *, Block *);
    Value fetch(Env *, Value name, Value default_value, Block *block);
    bool has_key(Env *, Value name);
    Value has_value(Env *, Value name);
    Value inspect(Env *);
    bool is_empty() const;
    Value keys(Env *);
    Value ref(Env *, Value name);
    Value refeq(Env *, Value name, Value value);
    Value rehash() const;
    Value replace(Env *, Value hash);
    Value size(Env *) const;
    Value slice(Env *, Args);
    Value to_s() const;
    Value to_hash(Env *);
    Value update(Env *env, Args args, Block *block);
    Value values(Env *);
};

}
