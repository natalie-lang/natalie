require 'sexp_processor'

require_relative '../spec_helper'
require_relative '../../lib/natalie/compiler/pass1'
require_relative '../../lib/natalie/compiler/pass2'
require_relative '../../lib/natalie/compiler/pass3'
require_relative '../../lib/natalie/compiler/pass4'

describe 'Natalie::Compiler' do
  describe 'Pass1' do
    it 'compiles from raw AST to pass1 AST' do
      context = {
        var_prefix: '',
        var_num: 0,
        template: '',
        repl: false,
        vars: {},
        inline_cpp_enabled: false,
        compile_flags: [],
      }
      fib = s(:block,
              s(:defn, :fib, s(:args, :n),
                s(:if, s(:call, s(:lvar, :n), :==, s(:lit, 0)),
                  s(:lit, 0),
                  s(:if, s(:call, s(:lvar, :n), :==, s(:lit, 1)),
                    s(:lit, 1),
                    s(:call,
                      s(:call, nil, :fib,
                        s(:call, s(:lvar, :n), :-, s(:lit, 1))),
                      :+,
                      s(:call, nil, :fib,
                        s(:call, s(:lvar, :n), :-, s(:lit, 2))))))),
              s(:lasgn, :num, s(:call, s(:const, :ARGV), :first)),
              s(:call, nil, :puts,
                s(:call, nil, :fib,
                  s(:if, s(:lvar, :num),
                    s(:call, s(:lvar, :num), :to_i),
                    s(:lit, 25)))))
      pass1 = Natalie::Compiler::Pass1.new(context)
      pass1.go(fib).should == s(:block,
                                s(:block,
                                  s(:def_fn, "fn1",
                                    s(:block,
                                      s(:env_set_method_name, "fib"),
                                      s(:assert_argc, :env, :argc, 1),
                                      s(:block,
                                        s(:declare, "args_as_array2", s(:args_to_array, :env, s(:l, "argc"), s(:l, "args"))),
                                        s(:arg_set, :env, s(:s, :n), s(:arg_value_by_path, :env, "args_as_array2", s(:nil), s(:l, :false), 1, 0, s(:l, :false), 0, 1, 0))),
                                      s(:block),
                                      s(:block,
                                        s(:c_if, s(:is_truthy, s(:send, s(:var_get, :env, s(:s, :n)), :==, s(:args, s(:new, :IntegerValue, :env, 0)), "nullptr")),
                                          s(:new, :IntegerValue, :env, 0),
                                          s(:c_if, s(:is_truthy, s(:send, s(:var_get, :env, s(:s, :n)), :==, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")),
                                            s(:new, :IntegerValue, :env, 1),
                                            s(:send, s(:send, :self, :fib, s(:args, s(:send, s(:var_get, :env, s(:s, :n)), :-, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")), "nullptr"), :+, s(:args, s(:send, :self, :fib, s(:args, s(:send, s(:var_get, :env, s(:s, :n)), :-, s(:args, s(:new, :IntegerValue, :env, 2)), "nullptr")), "nullptr")), "nullptr")))))),
                                  s(:define_method, :self, :env, s(:s, :fib), "fn1"),
                                  s(:"SymbolValue::intern", :env, s(:s, :fib))),
                              s(:var_set, :env, s(:s, :num), s(:send, s(:const_find, :self, :env, s(:s, :ARGV), s(:l, "Value::ConstLookupSearchMode::NotStrict")), :first, s(:args), "nullptr")),
                              s(:send, :self, :puts,
                                s(:args, s(:send, :self, :fib,
                                           s(:args, s(:c_if, s(:is_truthy, s(:var_get, :env, s(:s, :num))),
                                                      s(:send, s(:var_get, :env, s(:s, :num)), :to_i, s(:args), "nullptr"),
                                                      s(:new, :IntegerValue, :env, 25))), "nullptr")), "nullptr"))
    end
  end

  describe 'Pass2' do
    it 'compiles from pass1 AST to pass2 AST' do
      context = {
        var_prefix: '',
        var_num: 0,
        template: '',
        repl: false,
        vars: {},
        inline_cpp_enabled: false,
        compile_flags: [],
      }
      ast = s(:block,
              s(:block,
                s(:def_fn, "fn1",
                  s(:block,
                    s(:env_set_method_name, "fib"),
                    s(:assert_argc, :env, :argc, 1),
                    s(:block,
                      s(:declare, "args_as_array2", s(:args_to_array, :env, s(:l, "argc"), s(:l, "args"))),
                      s(:arg_set, :env, s(:s, :n), s(:arg_value_by_path, :env, "args_as_array2", s(:nil), s(:l, :false), 1, 0, s(:l, :false), 0, 1, 0))),
                    s(:block),
                    s(:block,
                      s(:c_if, s(:is_truthy, s(:send, s(:var_get, :env, s(:s, :n)), :==, s(:args, s(:new, :IntegerValue, :env, 0)), "nullptr")),
                        s(:new, :IntegerValue, :env, 0),
                        s(:c_if, s(:is_truthy, s(:send, s(:var_get, :env, s(:s, :n)), :==, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")),
                          s(:new, :IntegerValue, :env, 1),
                          s(:send, s(:send, :self, :fib, s(:args, s(:send, s(:var_get, :env, s(:s, :n)), :-, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")), "nullptr"), :+, s(:args, s(:send, :self, :fib, s(:args, s(:send, s(:var_get, :env, s(:s, :n)), :-, s(:args, s(:new, :IntegerValue, :env, 2)), "nullptr")), "nullptr")), "nullptr")))))),
                s(:define_method, :self, :env, s(:s, :fib), "fn1"),
                s(:"SymbolValue::intern", :env, s(:s, :fib))),
            s(:var_set, :env, s(:s, :num), s(:send, s(:const_find, :self, :env, s(:s, :ARGV), s(:l, "Value::ConstLookupSearchMode::NotStrict")), :first, s(:args), "nullptr")),
            s(:send, :self, :puts,
              s(:args, s(:send, :self, :fib,
                         s(:args, s(:c_if, s(:is_truthy, s(:var_get, :env, s(:s, :num))),
                                    s(:send, s(:var_get, :env, s(:s, :num)), :to_i, s(:args), "nullptr"),
                                    s(:new, :IntegerValue, :env, 25))), "nullptr")), "nullptr"))
      pass2 = Natalie::Compiler::Pass2.new(context)
      pass2.go(ast).should == s(:block,
                                s(:var_alloc, 1),
                                s(:declare, "num2", s(:nil)),
                                s(:block,
                                  s(:block,
                                    s(:def_fn, "fn1",
                                      s(:block,
                                        s(:var_alloc, 1),
                                        s(:declare, "n1", s(:nil)),
                                        s(:block,
                                          s(:env_set_method_name, "fib"),
                                          s(:assert_argc, :env, :argc, 1),
                                          s(:block,
                                            s(:declare, "args_as_array2", s(:args_to_array, :env, s(:l, "argc"), s(:l, "args"))),
                                            s(:var_set, "env", {:name=>"n", :index=>0, :var_num=>1}, false, s(:arg_value_by_path, :env, "args_as_array2", s(:nil), s(:l, :false), 1, 0, s(:l, :false), 0, 1, 0))),
                                          s(:block),
                                          s(:block,
                                            s(:c_if,
                                              s(:is_truthy, s(:send, s(:var_get, "env", {:name=>"n", :index=>0, :var_num=>1}), :==, s(:args, s(:new, :IntegerValue, :env, 0)), "nullptr")),
                                              s(:new, :IntegerValue, :env, 0),
                                              s(:c_if,
                                                s(:is_truthy, s(:send, s(:var_get, "env", {:name=>"n", :index=>0, :var_num=>1}), :==, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")),
                                                s(:new, :IntegerValue, :env, 1),
                                                s(:send,
                                                  s(:send, :self, :fib, s(:args, s(:send, s(:var_get, "env", {:name=>"n", :index=>0, :var_num=>1}), :-, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")), "nullptr"),
                                                  :+,
                                                  s(:args, s(:send, :self, :fib, s(:args, s(:send, s(:var_get, "env", {:name=>"n", :index=>0, :var_num=>1}), :-, s(:args, s(:new, :IntegerValue, :env, 2)), "nullptr")), "nullptr")),
                                                  "nullptr"))))))),
                                    s(:define_method, :self, :env, s(:s, :fib), "fn1"),
                                    s(:"SymbolValue::intern", :env, s(:s, :fib))),
                                  s(:var_set, "env", {:name=>"num", :index=>0, :var_num=>2}, false, s(:send, s(:const_find, :self, :env, s(:s, :ARGV), s(:l, "Value::ConstLookupSearchMode::NotStrict")), :first, s(:args), "nullptr")),
                                  s(:send,
                                    :self,
                                    :puts,
                                    s(:args, s(:send, :self, :fib, s(:args, s(:c_if, s(:is_truthy, s(:var_get, "env", {:name=>"num", :index=>0, :var_num=>2})), s(:send, s(:var_get, "env", {:name=>"num", :index=>0, :var_num=>2}), :to_i, s(:args), "nullptr"), s(:new, :IntegerValue, :env, 25))), "nullptr")),
                                    "nullptr")))
    end
  end

  describe 'Pass3' do
    it 'compiles from pass2 AST to pass3 AST' do
      context = {
        var_prefix: '',
        var_num: 0,
        template: '',
        repl: false,
        vars: {},
        inline_cpp_enabled: false,
        compile_flags: [],
      }
    ast = s(:block,
            s(:var_alloc, 1),
            s(:declare, "num2", s(:nil)),
            s(:block,
              s(:block,
                s(:def_fn, "fn1",
                  s(:block,
                    s(:var_alloc, 1),
                    s(:declare, "n1", s(:nil)),
                    s(:block,
                      s(:env_set_method_name, "fib"),
                      s(:assert_argc, :env, :argc, 1),
                      s(:block,
                        s(:declare, "args_as_array2", s(:args_to_array, :env, s(:l, "argc"), s(:l, "args"))),
                        s(:var_set, "env", {:name=>"n", :index=>0, :var_num=>1}, false, s(:arg_value_by_path, :env, "args_as_array2", s(:nil), s(:l, :false), 1, 0, s(:l, :false), 0, 1, 0))),
                      s(:block),
                      s(:block,
                        s(:c_if,
                          s(:is_truthy, s(:send, s(:var_get, "env", {:name=>"n", :index=>0, :var_num=>1}), :==, s(:args, s(:new, :IntegerValue, :env, 0)), "nullptr")),
                          s(:new, :IntegerValue, :env, 0),
                          s(:c_if,
                            s(:is_truthy, s(:send, s(:var_get, "env", {:name=>"n", :index=>0, :var_num=>1}), :==, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")),
                            s(:new, :IntegerValue, :env, 1),
                            s(:send,
                              s(:send, :self, :fib, s(:args, s(:send, s(:var_get, "env", {:name=>"n", :index=>0, :var_num=>1}), :-, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")), "nullptr"),
                              :+,
                              s(:args, s(:send, :self, :fib, s(:args, s(:send, s(:var_get, "env", {:name=>"n", :index=>0, :var_num=>1}), :-, s(:args, s(:new, :IntegerValue, :env, 2)), "nullptr")), "nullptr")),
                              "nullptr"))))))),
                s(:define_method, :self, :env, s(:s, :fib), "fn1"),
                s(:"SymbolValue::intern", :env, s(:s, :fib))),
              s(:var_set, "env", {:name=>"num", :index=>0, :var_num=>2}, false, s(:send, s(:const_find, :self, :env, s(:s, :ARGV), s(:l, "Value::ConstLookupSearchMode::NotStrict")), :first, s(:args), "nullptr")),
              s(:send,
                :self,
                :puts,
                s(:args, s(:send, :self, :fib, s(:args, s(:c_if, s(:is_truthy, s(:var_get, "env", {:name=>"num", :index=>0, :var_num=>2})), s(:send, s(:var_get, "env", {:name=>"num", :index=>0, :var_num=>2}), :to_i, s(:args), "nullptr"), s(:new, :IntegerValue, :env, 25))), "nullptr")),
                "nullptr")))
      pass3 = Natalie::Compiler::Pass3.new(context)
      pass3.go(ast).should == s(:block,
                                s(:var_alloc, 1),
                                s(:declare, "num2", s(:nil)),
                                s(:block,
                                  s(:block,
                                    s(:def_fn, "fn1",
                                      s(:block,
                                        s(:var_alloc, 1),
                                        s(:declare, "n1", s(:nil)),
                                        s(:block,
                                          s(:env_set_method_name, "fib"),
                                          s(:assert_argc, :env, :argc, 1),
                                          s(:block,
                                            s(:declare, "args_as_array2", s(:args_to_array, :env, s(:l, "argc"), s(:l, "args"))),
                                            s(:c_assign, "n1", s(:arg_value_by_path, :env, "args_as_array2", s(:nil), s(:l, :false), 1, 0, s(:l, :false), 0, 1, 0))),
                                          s(:block),
                                          s(:block,
                                            s(:c_if,
                                              s(:is_truthy, s(:send, s(:l, "n1"), :==, s(:args, s(:new, :IntegerValue, :env, 0)), "nullptr")),
                                              s(:new, :IntegerValue, :env, 0),
                                              s(:c_if,
                                                s(:is_truthy, s(:send, s(:l, "n1"), :==, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")),
                                                s(:new, :IntegerValue, :env, 1),
                                                s(:send,
                                                  s(:send, :self, :fib, s(:args, s(:send, s(:l, "n1"), :-, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")), "nullptr"),
                                                  :+,
                                                  s(:args, s(:send, :self, :fib, s(:args, s(:send, s(:l, "n1"), :-, s(:args, s(:new, :IntegerValue, :env, 2)), "nullptr")), "nullptr")),
                                                  "nullptr"))))))),
                                    s(:define_method, :self, :env, s(:s, :fib), "fn1"),
                                    s(:"SymbolValue::intern", :env, s(:s, :fib))),
                                  s(:c_assign, "num2", s(:send, s(:const_find, :self, :env, s(:s, :ARGV), s(:l, "Value::ConstLookupSearchMode::NotStrict")), :first, s(:args), "nullptr")),
                                  s(:send, :self, :puts, s(:args, s(:send, :self, :fib, s(:args, s(:c_if, s(:is_truthy, s(:l, "num2")), s(:send, s(:l, "num2"), :to_i, s(:args), "nullptr"), s(:new, :IntegerValue, :env, 25))), "nullptr")), "nullptr")))
    end
  end

  describe 'Pass4' do
    it 'compiles from pass3 AST to pass4 AST' do
      context = {
        var_prefix: '',
        var_num: 0,
        # NOTE: The template below is intentionally broken up to work around a bug where the code
        # was being inserted in the wrong place. There's probably a better way.
        template: '/*' + "NAT_TOP*/\n" \
          '/*' + "NAT_INIT*/\n" \
          '/*' + "NAT_BODY*/",
        repl: false,
        vars: {},
        inline_cpp_enabled: false,
        compile_flags: [],
      }
      ast = s(:block,
              s(:var_alloc, 1),
              s(:declare, "num2", s(:nil)),
              s(:block,
                s(:block,
                  s(:def_fn, "fn1",
                    s(:block,
                      s(:var_alloc, 1),
                      s(:declare, "n1", s(:nil)),
                      s(:block,
                        s(:env_set_method_name, "fib"),
                        s(:assert_argc, :env, :argc, 1),
                        s(:block,
                          s(:declare, "args_as_array2", s(:args_to_array, :env, s(:l, "argc"), s(:l, "args"))),
                          s(:c_assign, "n1", s(:arg_value_by_path, :env, "args_as_array2", s(:nil), s(:l, :false), 1, 0, s(:l, :false), 0, 1, 0))),
                        s(:block),
                        s(:block,
                          s(:c_if,
                            s(:is_truthy, s(:send, s(:l, "n1"), :==, s(:args, s(:new, :IntegerValue, :env, 0)), "nullptr")),
                            s(:new, :IntegerValue, :env, 0),
                            s(:c_if,
                              s(:is_truthy, s(:send, s(:l, "n1"), :==, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")),
                              s(:new, :IntegerValue, :env, 1),
                              s(:send,
                                s(:send, :self, :fib, s(:args, s(:send, s(:l, "n1"), :-, s(:args, s(:new, :IntegerValue, :env, 1)), "nullptr")), "nullptr"),
                                :+,
                                s(:args, s(:send, :self, :fib, s(:args, s(:send, s(:l, "n1"), :-, s(:args, s(:new, :IntegerValue, :env, 2)), "nullptr")), "nullptr")),
                                "nullptr"))))))),
                  s(:define_method, :self, :env, s(:s, :fib), "fn1"),
                  s(:"SymbolValue::intern", :env, s(:s, :fib))),
                s(:c_assign, "num2", s(:send, s(:const_find, :self, :env, s(:s, :ARGV), s(:l, "Value::ConstLookupSearchMode::NotStrict")), :first, s(:args), "nullptr")),
                s(:send, :self, :puts, s(:args, s(:send, :self, :fib, s(:args, s(:c_if, s(:is_truthy, s(:l, "num2")), s(:send, s(:l, "num2"), :to_i, s(:args), "nullptr"), s(:new, :IntegerValue, :env, 25))), "nullptr")), "nullptr")))
      pass4 = Natalie::Compiler::Pass4.new(context)
      pass4.go(ast).strip.should == <<END.strip
const char *source_files[0] = {  };

const char *source_methods[1] = { "fib" };

Value *fn1(Env *env, Value *self, size_t argc, Value **args, Block *block) {
    env->build_vars(1);
    Value * n1 = env->nil_obj();
    env->set_method_name(source_methods[0]);
    env->assert_argc(argc, 1);
    Value *argstoarray1 = args_to_array(env, argc, args);
    Value * args_as_array2 = argstoarray1;
    Value *argvaluebypath2 = arg_value_by_path(env, args_as_array2, env->nil_obj(), false, 1, 0, false, 0, 1, 0);
    n1 = argvaluebypath2;
    Value *new4 = new IntegerValue { env, 0 };
    Value *args3[1] = { new4 };
    Value *callresult5 = n1->send(env, "==", 1, args3, nullptr);
    Value *if6;
    if (callresult5->is_truthy()) {
        Value *new7 = new IntegerValue { env, 0 };
        if6 = new7;
    } else {
        Value *new9 = new IntegerValue { env, 1 };
        Value *args8[1] = { new9 };
        Value *callresult10 = n1->send(env, "==", 1, args8, nullptr);
        Value *if11;
        if (callresult10->is_truthy()) {
            Value *new12 = new IntegerValue { env, 1 };
            if11 = new12;
        } else {
            Value *new15 = new IntegerValue { env, 1 };
            Value *args14[1] = { new15 };
            Value *callresult16 = n1->send(env, "-", 1, args14, nullptr);
            Value *args13[1] = { callresult16 };
            Value *callresult17 = self->send(env, "fib", 1, args13, nullptr);
            Value *new21 = new IntegerValue { env, 2 };
            Value *args20[1] = { new21 };
            Value *callresult22 = n1->send(env, "-", 1, args20, nullptr);
            Value *args19[1] = { callresult22 };
            Value *callresult23 = self->send(env, "fib", 1, args19, nullptr);
            Value *args18[1] = { callresult23 };
            Value *callresult24 = callresult17->send(env, "+", 1, args18, nullptr);
            if11 = callresult24;
        }
        if6 = if11;
    }
    return if6;
}

env->build_vars(1);
Value * num2 = env->nil_obj();
self->define_method(env, "fib", fn1);
Value *SymbolValueintern25 = SymbolValue::intern(env, "fib");
Value *constfind26 = self->const_find(env, "ARGV", Value::ConstLookupSearchMode::NotStrict);
Value *callresult27 = constfind26->send(env, "first", 0, nullptr, nullptr);
num2 = callresult27;
Value *if30;
if (num2->is_truthy()) {
    Value *callresult31 = num2->send(env, "to_i", 0, nullptr, nullptr);
    if30 = callresult31;
} else {
    Value *new32 = new IntegerValue { env, 25 };
    if30 = new32;
}
Value *args29[1] = { if30 };
Value *callresult33 = self->send(env, "fib", 1, args29, nullptr);
Value *args28[1] = { callresult33 };
Value *callresult34 = self->send(env, "puts", 1, args28, nullptr);
END
    end
  end
end
