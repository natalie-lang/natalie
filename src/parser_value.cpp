#include "natalie.hpp"
#include "natalie/array_value.hpp"
#include "natalie/env.hpp"
#include "natalie/symbol_value.hpp"

#include <peg_parser/generator.h>

namespace Natalie {

Value *ParserValue::parse(Env *env, Value *code) {
    return new ArrayValue { env };
}

}
