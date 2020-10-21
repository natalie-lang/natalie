require_relative '../spec_helper'
require 'sexp'

unless defined?(Parser)
  class Parser
    def self.parse(code)
      node = RubyParser.new.parse(code)
      if node.nil?
        s(:block)
      elsif node.first == :block
        node
      else
        s(:block, node)
      end
    rescue Racc::ParseError
      raise SyntaxError
    end
  end
end

describe 'Parser' do
  describe '#parse' do
    it 'parses numbers' do
      Parser.parse('1').should == s(:block, s(:lit, 1))
      Parser.parse(' 1').should == s(:block, s(:lit, 1))
      Parser.parse('1.5 ').should == s(:block, s(:lit, 1.5))
    end

    it 'parses operator calls' do
      Parser.parse('1 + 3').should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 3)))
      Parser.parse('1+3').should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 3)))
      Parser.parse("1+\n 3").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 3)))
      Parser.parse('1 - 3').should == s(:block, s(:call, s(:lit, 1), :-, s(:lit, 3)))
      Parser.parse('1 * 3').should == s(:block, s(:call, s(:lit, 1), :*, s(:lit, 3)))
      Parser.parse('1 / 3').should == s(:block, s(:call, s(:lit, 1), :/, s(:lit, 3)))
      Parser.parse('1 * 2 + 3').should == s(:block, s(:call, s(:call, s(:lit, 1), :*, s(:lit, 2)), :+, s(:lit, 3)))
      Parser.parse('1 / 2 - 3').should == s(:block, s(:call, s(:call, s(:lit, 1), :/, s(:lit, 2)), :-, s(:lit, 3)))
      Parser.parse('1 + 2 * 3').should == s(:block, s(:call, s(:lit, 1), :+, s(:call, s(:lit, 2), :*, s(:lit, 3))))
      Parser.parse('1 - 2 / 3').should == s(:block, s(:call, s(:lit, 1), :-, s(:call, s(:lit, 2), :/, s(:lit, 3))))
      Parser.parse('(1 + 2) * 3').should == s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:lit, 3)))
      Parser.parse("(\n1 + 2\n) * 3").should == s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:lit, 3)))
      Parser.parse('(1 - 2) / 3').should == s(:block, s(:call, s(:call, s(:lit, 1), :-, s(:lit, 2)), :/, s(:lit, 3)))
      Parser.parse('(1 + 2) * (3 + 4)').should == s(:block, s(:call, s(:call, s(:lit, 1), :+, s(:lit, 2)), :*, s(:call, s(:lit, 3), :+, s(:lit, 4))))
    end

    it 'raises an error if there is a syntax error' do
      -> { Parser.parse(')') }.should raise_error(SyntaxError)
    end

    it 'parses strings' do
      Parser.parse('""').should == s(:block, s(:str, ''))
      Parser.parse('"foo"').should == s(:block, s(:str, 'foo'))
      Parser.parse('"this is \"quoted\""').should == s(:block, s(:str, 'this is "quoted"'))
      Parser.parse('"other escaped chars \\\\ \n"').should == s(:block, s(:str, "other escaped chars \\ \n"))
      Parser.parse("''").should == s(:block, s(:str, ''))
      Parser.parse("'foo'").should == s(:block, s(:str, 'foo'))
      Parser.parse("'this is \\'quoted\\''").should == s(:block, s(:str, "this is 'quoted'"))
      Parser.parse("'other escaped chars \\\\ \\n'").should == s(:block, s(:str, "other escaped chars \\ \\n"))
    end

    it 'parses multiple expressions' do
      Parser.parse("1 + 2\n3 + 4").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:call, s(:lit, 3), :+, s(:lit, 4)))
      Parser.parse("1 + 2;'foo'").should == s(:block, s(:call, s(:lit, 1), :+, s(:lit, 2)), s(:str, "foo"))
    end
  end
end
