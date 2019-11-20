require 'minitest/spec'
require 'minitest/autorun'

describe 'Natalie::Parser' do
  def build_ast(code)
    # Don't hard-depend on the internal implementation, but use it most of the time for speed.
    if rand(20) == 0
      out = `#{bin} --ast -e #{code.inspect} 2>&1`
      raise out if out =~ /lib\/natalie.*Error/
      eval(out)
    else
      require_relative '../../lib/natalie/parser'
      Natalie::Parser.new(code).ast
    end
  end

  def bin
    File.expand_path('../../bin/natalie', __dir__)
  end

  describe 'AST' do
    it 'parses numbers' do
      ast = build_ast('1')
      ast.must_equal [[:number, '1']]
    end

    it 'parses multiple expressions' do
      ast = build_ast("1\n2")
      ast.must_equal [[:number, '1'], [:number, '2']]
      ast = build_ast("1 \n 2")
      ast.must_equal [[:number, '1'], [:number, '2']]
      ast = build_ast("1;2")
      ast.must_equal [[:number, '1'], [:number, '2']]
      ast = build_ast("1 ; 2")
      ast.must_equal [[:number, '1'], [:number, '2']]
    end

    it 'parses strings' do
      ast = build_ast('"a"')
      ast.must_equal [[:string, 'a']]
      ast = build_ast("'a'")
      ast.must_equal [[:string, 'a']]
    end

    it 'parses operator method calls' do
      ast = build_ast("'a' << 'b'")
      ast.must_equal [[:send, [:string, 'a'], '<<', [[:string, 'b']]]]
      ast = build_ast("'a' << 'b' << 'c'")
      ast.must_equal [[:send, [:string, "a"], "<<", [[:send, [:string, "b"], "<<", [[:string, "c"]]]]]]
      ast = build_ast("'a'.<< 'b'")
      ast.must_equal [[:send, [:string, 'a'], '<<', [[:string, 'b']]]]
      ast = build_ast("'a'.<<('b')")
      ast.must_equal [[:send, [:string, 'a'], '<<', [[:string, 'b']]]]
      ast = build_ast("'a'.<<('b', 'c')")
      ast.must_equal [[:send, [:string, 'a'], '<<', [[:string, 'b'], [:string, 'c']]]]
    end

    it 'parses method calls with a dot' do
      ast = build_ast("'a'.upcase")
      ast.must_equal [[:send, [:string, 'a'], 'upcase', []]]
      ast = build_ast("'a'.upcase.strip")
      ast.must_equal [[:send, [:send, [:string, 'a'], 'upcase', []], 'strip', []]]
      ast = build_ast("'a'.upcase.include?('a')")
      ast.must_equal [[:send, [:send, [:string, 'a'], 'upcase', []], 'include?', [[:string, 'a']]]]
      ast = build_ast("'a'.upcase.include? 'a'")
      ast.must_equal [[:send, [:send, [:string, 'a'], 'upcase', []], 'include?', [[:string, 'a']]]]
    end

    it 'parses method calls without a dot' do
      ast = build_ast("foo")
      ast.must_equal [[:send, 'self', 'foo', []]]
      ast = build_ast("foo 'bar'")
      ast.must_equal [[:send, 'self', 'foo', [[:string, 'bar']]]]
      ast = build_ast("foo('bar', 'baz')")
      ast.must_equal [[:send, 'self', 'foo', [[:string, 'bar'], [:string, 'baz']]]]
      ast = build_ast("foo.upcase")
      ast.must_equal [[:send, [:send, 'self', 'foo', []], 'upcase', []]]
    end

    it 'parses assignments' do
      ast = build_ast("x = 1")
      ast.must_equal [[:assign, 'x', [:number, '1']]]
      ast = build_ast("num=1")
      ast.must_equal [[:assign, 'num', [:number, '1']]]
      ast = build_ast("message_upcase = 'hi'.upcase")
      ast.must_equal [[:assign, 'message_upcase', [:send, [:string, 'hi'], 'upcase', []]]]
    end

    it 'parses method definitions' do
      ast = build_ast("def foo; 'foo'; end")
      ast.must_equal [[:def, 'foo', [], [[:string, 'foo']]]]
      ast = build_ast("def foo \n 'foo'\n 2 \n 'bar'\n end")
      ast.first.first.must_equal :def
    end
  end
end
