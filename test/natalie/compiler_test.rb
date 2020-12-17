require 'sexp_processor'

require_relative '../spec_helper'
require_relative '../../lib/natalie/compiler/pass1'

describe 'Natalie::Compiler' do
  describe 'Pass1' do
    it 'works' do
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
end
