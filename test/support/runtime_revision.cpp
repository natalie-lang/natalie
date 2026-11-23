#include "natalie.hpp"
#include <cstdlib>

using namespace Natalie;

__attribute__((constructor)) static void set_test_revision() {
    if (auto revision = std::getenv("NAT_TEST_RUBY_REVISION"))
        Natalie::ruby_revision = revision;
}

Value init_runtime_revision(Env *env, Value self) {
    return Value::nil();
}
