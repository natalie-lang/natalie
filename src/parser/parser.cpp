#include "natalie/parser.hpp"
#include "extconf.h"
#include "natalie.hpp"
#include "natalie/string.hpp"
#include "ruby.h"
#include "stdio.h"

Natalie::ValuePtr init_obj_sexp(Natalie::Env *env, Natalie::ValuePtr self);

Natalie::Env *env;

extern "C" {

VALUE to_mri_ruby(Natalie::ValuePtr node) {
    switch (node->type()) {
    case Natalie::Value::Type::Array: {
        VALUE ary = rb_ary_new();
        for (auto item : *node->as_array())
            rb_ary_push(ary, to_mri_ruby(item));
        return ary;
        break;
    }
    case Natalie::Value::Type::Symbol:
        return ID2SYM(rb_intern(node->as_symbol()->c_str()));
    case Natalie::Value::Type::Nil:
        return Qnil;
    default:
        printf("Unknown Natalie node type: %d\n", (int)node->type());
        abort();
    }
}

VALUE parse(int argc, VALUE *argv, VALUE self) {
    if (argc < 1 || argc > 2) {
        VALUE SyntaxError = rb_const_get(rb_cObject, rb_intern("SyntaxError"));
        rb_raise(SyntaxError, "wrong number of arguments (given %d, expected 1..2)", argc);
    }
    auto code = argv[0];
    auto path = argv[0];
    auto code_nat_string = new Natalie::String { StringValueCStr(code) };
    auto path_nat_string = new Natalie::String { StringValueCStr(path) };
    auto parser = Natalie::Parser { code_nat_string, path_nat_string };
    Natalie::Node *tree;
    Natalie::ValuePtr tree_value;
    try {
        tree = parser.tree(env);
        tree_value = tree->to_ruby(env);
        return to_mri_ruby(tree_value);
    } catch (Natalie::ExceptionValue *exception) {
        handle_top_level_exception(env, exception, false);
        abort();
    }
}

void Init_parser() {
    env = Natalie::build_top_env();
    init_obj_sexp(env, Natalie::GlobalEnv::the()->Object());
    VALUE Parser = rb_define_class("Parser", rb_cObject);
    rb_define_singleton_method(Parser, "parse", parse, -1);
}
}
