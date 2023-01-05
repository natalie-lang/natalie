#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

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
    Value fetch(Env *, Value name);
    bool has_key(Env *, Value name);
    Value inspect(Env *);
    bool is_empty() const;
    Value ref(Env *, Value name);
    Value refeq(Env *, Value name, Value value);
    Value rehash() const;
    Value replace(Env *, Value hash);
    Value size(Env *) const;
    Value to_s() const;
    Value to_hash(Env *);
};

}
