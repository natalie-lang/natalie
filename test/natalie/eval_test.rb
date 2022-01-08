# skip-ruby

require_relative '../spec_helper'

describe 'eval macro' do
  def three
    3
  end

  it 'can compile static strings' do
    eval('1 + 1').should == 2
    eval('three').should == 3

    # FIXME: need to be able to pass in locals into Parser
    # BLOCKER: we need to implement local_variables first
    #four = 4
    #eval("four").should == 4
  end

  it 'raises a SyntaxError for anything else' do
    -> { eval("1 + #{1}") }.should raise_error(SyntaxError)
  end
end
