#include "natalie/parser.hpp"
#include "extconf.h"
#include "natalie.hpp"
#include "natalie/string.hpp"
#include "ruby.h"
#include "stdio.h"

Natalie::Env *env;
VALUE Sexp;

extern "C" {

VALUE to_mri_ruby(Natalie::ValuePtr value) {
    switch (value->type()) {
    case Natalie::Value::Type::Array: {
        VALUE sexp = rb_class_new_instance(0, nullptr, Sexp);
        for (auto item : *value->as_array())
            rb_ary_push(sexp, to_mri_ruby(item));
        return sexp;
        break;
    }
    case Natalie::Value::Type::Symbol:
        return ID2SYM(rb_intern(value->as_symbol()->c_str()));
    case Natalie::Value::Type::Nil:
        return Qnil;
    case Natalie::Value::Type::Integer:
        return rb_int_new(value->as_integer()->to_nat_int_t());
    case Natalie::Value::Type::Range:
        return rb_range_new(
            to_mri_ruby(value->as_range()->begin()),
            to_mri_ruby(value->as_range()->end()),
            value->as_range()->exclude_end());
    case Natalie::Value::Type::String:
        return rb_str_new_cstr(value->as_string()->c_str());
    default:
        printf("Unknown Natalie value type: %d\n", (int)value->type());
        abort();
    }
}

VALUE parse(int argc, VALUE *argv, VALUE self) {
    if (argc < 1 || argc > 2) {
        VALUE SyntaxError = rb_const_get(rb_cObject, rb_intern("SyntaxError"));
        rb_raise(SyntaxError, "wrong number of arguments (given %d, expected 1..2)", argc);
    }
    auto code = argv[0];
    VALUE path;
    if (argc > 1)
        path = argv[1];
    else
        path = rb_str_new_cstr("(string)");
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
    Sexp = rb_eval_string_protect("class Sexp < Array \n\
                                     def initialize(*items) \n\
                                       items.each { |i| self << i } \n\
                                     end \n\
                                     def inspect \n\
                                       \"s(#{map(&:inspect).join(', ')})\" \n\
                                     end \n\
                                     def pretty_print q \n\
                                       nnd = \")\" \n\
                                       q.group(1, \"s(\", nnd) do \n\
                                         q.seplist(self) { |v| q.pp v } \n\
                                       end \n\
                                     end \n\
                                   end \n\
                                   Sexp",
        &error);
    env = Natalie::build_top_env();
    VALUE Parser = rb_define_class("Parser", rb_cObject);
    rb_define_singleton_method(Parser, "parse", parse, -1);
}
}
