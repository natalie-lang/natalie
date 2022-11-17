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
    Value inspect(Env *);
    Value size(Env *) const;
    Value to_hash(Env *);
    Value delete_key(Env *, Value name);
    bool has_key(Env *, Value name);
    Value assoc(Env *, Value name);
    Value fetch(Env *, Value name);
    Value ref(Env *, Value name);
    Value refeq(Env *, Value name, Value value);
    Value to_s() const;
    Value rehash() const;
};

}
