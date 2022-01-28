#include "extconf.h"
#include "ruby.h"
#include "ruby/intern.h"
#include "stdio.h"

// these includes MUST come after
#include "natalie.hpp"
#include "natalie_parser/parser.hpp"

Natalie::Env *env;
VALUE Parser;

extern "C" {

VALUE to_mri_ruby(Natalie::Value value) {
    VALUE Sexp = rb_const_get(rb_cObject, rb_intern("Sexp"));
    switch (value->type()) {
    case Natalie::Object::Type::Array: {
        VALUE sexp = rb_class_new_instance(0, nullptr, Sexp);
        for (auto item : *value->as_array())
            rb_ary_push(sexp, to_mri_ruby(item));
        return sexp;
        break;
    }
    case Natalie::Object::Type::Float:
        return rb_float_new(value->as_float()->to_double());
    case Natalie::Object::Type::Integer:
        return rb_int_new(value->as_integer()->to_nat_int_t());
    case Natalie::Object::Type::Nil:
        return Qnil;
    case Natalie::Object::Type::Range:
        return rb_range_new(
            to_mri_ruby(value->as_range()->begin()),
            to_mri_ruby(value->as_range()->end()),
            value->as_range()->exclude_end());
    case Natalie::Object::Type::Regexp: {
        auto re = value->as_regexp();
        return rb_reg_new(re->pattern(), strlen(re->pattern()), re->options());
    }
    case Natalie::Object::Type::String:
        return rb_str_new_cstr(value->as_string()->c_str());
    case Natalie::Object::Type::Symbol:
        return ID2SYM(rb_intern(value->as_symbol()->c_str()));
    default:
        printf("Unknown Natalie value type: %d\n", (int)value->type());
        abort();
    }
}

VALUE initialize(int argc, VALUE *argv, VALUE self) {
    if (argc < 1 || argc > 2)
        rb_raise(rb_eSyntaxError, "wrong number of arguments (given %d, expected 1..2)", argc);
    rb_ivar_set(self, rb_intern("@code"), argv[0]);
    VALUE path;
    if (argc > 1)
        path = argv[1];
    else
        path = rb_str_new_cstr("(string)");
    rb_ivar_set(self, rb_intern("@path"), path);
    return self;
}

VALUE parse_on_instance(VALUE self) {
    VALUE code = rb_ivar_get(self, rb_intern("@code"));
    VALUE path = rb_ivar_get(self, rb_intern("@path"));
    auto code_nat_string = new Natalie::String { StringValueCStr(code) };
    auto path_nat_string = new Natalie::String { StringValueCStr(path) };
    auto parser = Natalie::Parser { code_nat_string, path_nat_string };
    Natalie::ParserObject parser_object;
    Natalie::Node *tree;
    Natalie::Value tree_value;
    try {
        tree = parser.tree();
        tree_value = parser_object.node_to_ruby(env, tree);
        return to_mri_ruby(tree_value);
    } catch (NatalieParser::Parser::SyntaxError &error) {
        rb_raise(rb_eSyntaxError, "%s", error.message());
    }
}

VALUE parse(int argc, VALUE *argv, VALUE self) {
    VALUE parser = rb_class_new_instance(argc, argv, Parser);
    return parse_on_instance(parser);
}

VALUE s(int argc, VALUE *argv, VALUE self) {
    VALUE Sexp = rb_const_get(rb_cObject, rb_intern("Sexp"));
    VALUE sexp = rb_class_new_instance(0, nullptr, Sexp);
    for (size_t i = 0; i < argc; ++i)
        rb_ary_push(sexp, argv[i]);
    return sexp;
}

void Init_parser_c_ext() {
    int error;
    env = Natalie::build_top_env();
    Parser = rb_define_class("Parser", rb_cObject);
    rb_define_method(Parser, "parse", parse_on_instance, 0);
    rb_define_method(Parser, "initialize", initialize, -1);
    rb_define_singleton_method(Parser, "parse", parse, -1);
    rb_define_method(rb_cObject, "s", s, -1);
}
}
