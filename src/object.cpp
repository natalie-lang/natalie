#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Object_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *obj;
    switch (self->as_class()->object_type()) {
    case Value::Type::Array:
        obj = new ArrayValue { env, self->as_class() };
        break;

    case Value::Type::Class:
        obj = new ClassValue { env, self->as_class() };
        break;

    case Value::Type::Encoding:
        obj = new EncodingValue { env, self->as_class() };
        break;

    case Value::Type::Exception:
        obj = new ExceptionValue { env, self->as_class() };
        break;

    case Value::Type::Hash:
        obj = new HashValue { env, self->as_class() };
        break;

    case Value::Type::Io:
        obj = new IoValue { env, self->as_class() };
        break;

    case Value::Type::MatchData:
        obj = new MatchDataValue { env, self->as_class() };
        break;

    case Value::Type::Module:
        obj = new ModuleValue { env, self->as_class() };
        break;

    case Value::Type::Object:
        obj = new Value { self->as_class() };
        break;

    case Value::Type::Proc:
        obj = new ProcValue { env, self->as_class() };
        break;

    case Value::Type::Range:
        obj = new RangeValue { env, self->as_class() };
        break;

    case Value::Type::Regexp:
        obj = new RegexpValue { env, self->as_class() };
        break;

    case Value::Type::String:
        obj = new StringValue { env, self->as_class() };
        break;

    case Value::Type::VoidP:
        obj = new VoidPValue { env, self->as_class() };
        break;

    case Value::Type::Nil:
    case Value::Type::False:
    case Value::Type::True:
    case Value::Type::Integer:
    case Value::Type::Float:
    case Value::Type::Symbol:
        NAT_UNREACHABLE();
    }

    return obj->initialize(env, argc, args, block);
}

}
