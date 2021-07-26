#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/integer_value.hpp"
#include "natalie/io_value.hpp"
#include "natalie/regexp_value.hpp"
#include "natalie/string_value.hpp"
#include "natalie/symbol_value.hpp"

namespace Natalie {

class FileValue : public IoValue {
public:
    FileValue()
        : IoValue { GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("File"))->as_class() } { }

    ValuePtr initialize(Env *, ValuePtr, ValuePtr, Block *);

    static ValuePtr open(Env *env, ValuePtr filename, ValuePtr flags_obj, Block *block) {
        ValuePtr args[] = { filename, flags_obj };
        size_t argc = 1;
        if (flags_obj) argc++;
        auto obj = _new(env, GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("File"))->as_class(), argc, args, nullptr);
        if (block) {
            ValuePtr block_args[] = { obj };
            ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WITH_CLEANUP(env, block, 1, block_args, nullptr, obj->as_file()->close(env));
            return result;
        } else {
            return obj;
        }
    }

    static ValuePtr expand_path(Env *env, ValuePtr path, ValuePtr root);
    static ValuePtr unlink(Env *env, ValuePtr path);

    static void build_constants(Env *env, ClassValue *klass);

    static bool exist(Env *env, ValuePtr path) {
        struct stat sb;
        path->assert_type(env, Value::Type::String, "String");
        return stat(path->as_string()->c_str(), &sb) != -1;
    }

    const String *path() { return m_path; }
    void set_path(String *path) { m_path = path; };

    virtual void gc_inspect(char *buf, size_t len) override {
        snprintf(buf, len, "<FileValue %p>", this);
    }

    virtual void visit_children(Visitor &visitor) override final {
        Value::visit_children(visitor);
        visitor.visit(m_path);
    }

private:
    const String *m_path { nullptr };
};

}
