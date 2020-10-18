# skip-ruby

require_relative '../spec_helper'

describe 'Parser' do
  describe '#parse' do
    it 'parses numbers' do
      Parser.parse('1').should == [:block, [:lit, 1]]
      Parser.parse(' 1').should == [:block, [:lit, 1]]
      Parser.parse('1.5 ').should == [:block, [:lit, 1.5]]
    end

    it 'parses operator calls' do
      Parser.parse('1 + 3').should == [:block, [:call, [:lit, 1], :+, [:lit, 3]]]
      Parser.parse('1 - 3').should == [:block, [:call, [:lit, 1], :-, [:lit, 3]]]
      Parser.parse('1 * 3').should == [:block, [:call, [:lit, 1], :*, [:lit, 3]]]
      Parser.parse('1 / 3').should == [:block, [:call, [:lit, 1], :/, [:lit, 3]]]
      Parser.parse('1 * 2 + 3').should == [:block, [:call, [:call, [:lit, 1], :*, [:lit, 2]], :+, [:lit, 3]]]
      Parser.parse('1 / 2 - 3').should == [:block, [:call, [:call, [:lit, 1], :/, [:lit, 2]], :-, [:lit, 3]]]
      Parser.parse('1 + 2 * 3').should == [:block, [:call, [:lit, 1], :+, [:call, [:lit, 2], :*, [:lit, 3]]]]
      Parser.parse('1 - 2 / 3').should == [:block, [:call, [:lit, 1], :-, [:call, [:lit, 2], :/, [:lit, 3]]]]
      Parser.parse('(1 + 2) * 3').should == [:block, [:call, [:call, [:lit, 1], :+, [:lit, 2]], :*, [:lit, 3]]]
      Parser.parse('(1 - 2) / 3').should == [:block, [:call, [:call, [:lit, 1], :-, [:lit, 2]], :/, [:lit, 3]]]
      Parser.parse('(1 + 2) * (3 + 4)').should == [:block, [:call, [:call, [:lit, 1], :+, [:lit, 2]], :*, [:call, [:lit, 3], :+, [:lit, 4]]]]
    end

    it 'raises an error if there is a syntax error' do
      -> { Parser.parse(')') }.should raise_error(SyntaxError)
    end

    it 'parses strings' do
      Parser.parse('""').should == [:block, [:str, '']]
      Parser.parse('"foo"').should == [:block, [:str, 'foo']]
      Parser.parse('"this is \"quoted\""').should == [:block, [:str, 'this is "quoted"']]
      Parser.parse('"other escaped chars \\\\ \n"').should == [:block, [:str, "other escaped chars \\ \n"]]
      Parser.parse("''").should == [:block, [:str, '']]
      Parser.parse("'foo'").should == [:block, [:str, 'foo']]
      Parser.parse("'this is \\'quoted\\''").should == [:block, [:str, "this is 'quoted'"]]
      Parser.parse("'other escaped chars \\\\ \\n'").should == [:block, [:str, "other escaped chars \\ \\n"]]
    end
  end
end
