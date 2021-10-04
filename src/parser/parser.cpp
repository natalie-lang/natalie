#include "natalie/parser.hpp"
#include "extconf.h"
#include "natalie.hpp"
#include "natalie/string.hpp"
#include "ruby.h"
#include "stdio.h"

Natalie::Env *env;
VALUE Sexp;

extern "C" {

VALUE to_mri_ruby(Natalie::ValuePtr node) {
    switch (node->type()) {
    case Natalie::Value::Type::Array: {
        VALUE sexp = rb_class_new_instance(0, nullptr, Sexp);
        for (auto item : *node->as_array())
            rb_ary_push(sexp, to_mri_ruby(item));
        return sexp;
        break;
    }
    case Natalie::Value::Type::Symbol:
        return ID2SYM(rb_intern(node->as_symbol()->c_str()));
    case Natalie::Value::Type::Nil:
        return Qnil;
    case Natalie::Value::Type::Integer:
        return rb_int_new(node->as_integer()->to_nat_int_t());
    case Natalie::Value::Type::String:
        return rb_str_new_cstr(node->as_string()->c_str());
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
    int error;
    Sexp = rb_eval_string_protect("class Sexp < Array; def initialize(*items); items.each { |i| self << i }; end; def inspect; \"s(#{map(&:inspect).join(', ')})\"; end; end; Sexp", &error);
    env = Natalie::build_top_env();
    VALUE Parser = rb_define_class("Parser", rb_cObject);
    rb_define_singleton_method(Parser, "parse", parse, -1);
}
}
