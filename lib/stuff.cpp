#include "natalie.hpp"

using namespace Natalie;

Value init_obj_stuff(Env *env, Value self) {
    printf("yo\n");
    return NilObject::the();
}
