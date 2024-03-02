#pragma once

/*
 * Experimental file to emulate the MRI API in Natalie.
 */

#include "natalie.hpp"

#include <type_traits>

using VALUE = Value;

const char *rb_eRuntimeError = "RuntimeError";

#define rb_raise(CLASS, MSG)        \
    do {                            \
        env->raise((CLASS), (MSG)); \
    } while (0);

inline char *RSTRING_PTR(Value string) {
    // We probably shouldn't be const casting
    return const_cast<char *>(string->as_string()->c_str());
}

inline size_t RSTRING_LEN(Value string) {
    return string->as_string()->bytesize();
}

inline StringObject *rb_str_new(const char c, const size_t size) {
    String str { size, c }; // Add this contructor to StringObject
    return new StringObject { std::move(str) };
}

#define StringValue(string)                                                \
    do {                                                                   \
        static_assert(!std::is_rvalue_reference<decltype(string)>::value); \
        string = string->to_str(env);                                      \
    } while (0);
