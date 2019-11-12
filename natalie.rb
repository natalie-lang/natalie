require 'strscan'

module Natalie
  class Parser
    def initialize(code_str)
      @code_str = code_str.strip + "\n"
      @scanner = StringScanner.new(@code_str)
    end

    def ast
      ast = []
      while !@scanner.eos? && (e = expr)
        ast << e
        raise "expected ; or newline; got: #{@scanner.inspect}" unless @scanner.skip(/;+|\n+/)
      end
      ast
    end

    def expr
      assignment || number || string || method
    end

    def assignment
      if @scanner.check(/[a-z]+\s*=/)
        id = identifier
        @scanner.skip(/\s*=\s*/)
        [:assign, id, expr]
      end
    end

    def identifier
      @scanner.scan(/[a-z][a-z0-9_]*/)
    end

    def number
      if (n = @scanner.scan(/\d+/))
        [:number, n]
      end
    end

    def string
      if (s = @scanner.scan(/'[^']*'|"[^"]*"/))
        [:string, s[1...-1]]
      end
    end

    def method
      if @scanner.scan(/def /)
        @scanner.skip(/\s*/)
        name = @scanner.scan(/[a-z][a-z0-9_]*/)
        raise 'expected method name after def' unless name
        @scanner.skip(/[;\n]*/)
        body = []
        until @scanner.check(/\s*end[;\n\s]/)
          body << expr
          raise 'expected ; or newline' unless @scanner.skip(/;+|\n+/)
        end
        @scanner.skip(/\s*end/)
        [:def, name, [], body]
      end
    end
  end
end

require 'minitest/spec'
require 'minitest/autorun'

describe Natalie::Parser do
  describe '#ast' do
    it 'builds an ast' do
      @parser = Natalie::Parser.new("num = 1\nstr = 'a'\ndef foo\n2\nend")
      @parser.ast.must_equal [
        [:assign, 'num', [:number, '1']],
        [:assign, 'str', [:string, 'a']],
        [:def, 'foo', [], [[:number, '2']]],
      ]
    end
  end
end
