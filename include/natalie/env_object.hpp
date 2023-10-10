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
    Value clone(Env *);
    Value delete_if(Env *, Block *);
    Value delete_key(Env *, Value name, Block *block);
    Value dup(Env *);
    Value each(Env *, Block *block);
    Value each_key(Env *, Block *);
    Value each_value(Env *, Block *);
    Value except(Env *, Args);
    Value fetch(Env *, Value name, Value default_value, Block *block);
    bool has_key(Env *, Value name);
    Value has_value(Env *, Value name);
    Value inspect(Env *);
    Value invert(Env *);
    bool is_empty() const;
    Value keep_if(Env *, Block *);
    Value key(Env *, Value);
    Value keys(Env *);
    Value rassoc(Env *, Value name);
    Value ref(Env *, Value name);
    Value refeq(Env *, Value name, Value value);
    Value rehash() const;
    Value reject(Env *, Block *);
    Value reject_in_place(Env *, Block *);
    Value replace(Env *, Value hash);
    Value select(Env *, Block *);
    Value select_in_place(Env *, Block *);
    Value shift();
    size_t size() const;
    Value slice(Env *, Args);
    Value to_s() const;
    Value to_hash(Env *, Block *);
    Value update(Env *env, Args args, Block *block);
    Value values(Env *);
    Value values_at(Env *, Args args);
};

}
