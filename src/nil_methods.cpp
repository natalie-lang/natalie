#include "natalie.hpp"

namespace Natalie {

bool NilMethods::and_method(const Value, const Value) {
    return false;
}

bool NilMethods::or_method(const Value, Value other) {
    return other.is_truthy();
}

Value NilMethods::eqtilde(const Value, const Value) {
    return Value::nil();
}

Value NilMethods::rationalize(const Value self, const Optional<Value>) {
    return to_r(self);
}

Value NilMethods::to_s(const Value) {
    if (!s_string)
        s_string = StringObject::create("");
    return s_string;
}

Value NilMethods::to_a(const Value) {
    return new ArrayObject {};
}

Value NilMethods::to_c(const Value) {
    return new ComplexObject { Value::integer(0) };
}

Value NilMethods::to_h(const Value) {
    return new HashObject {};
}

Value NilMethods::to_f(const Value) {
    return FloatObject::create(0.0);
}

Value NilMethods::to_i(const Value) {
    return Value::integer(0);
}

Value NilMethods::to_r(const Value) {
    return new RationalObject { Value::integer(0), Value::integer(1) };
}

Value NilMethods::inspect(const Value) {
    return StringObject::create("nil");
}

}
