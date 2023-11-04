require_relative '../spec_helper'

describe 'eval macro' do
  def three
    3
  end

  it 'can compile static strings' do
    eval('1 + 1').should == 2
    eval('three').should == 3
  end

  def eval_in_method_with_locals
    two = 2
    eval('two')
  end

  it 'can reference local variables' do
    # in a block
    one = 1
    eval('one').should == 1

    # in a method
    eval_in_method_with_locals.should == 2

    # in a module
    module M
      three = 3
      eval('three').should == 3
    end

    # in a class
    class C
      four = 4
      eval('four').should == 4
    end

    # at the top level of a program
    ruby_exe("five = 5; print eval('five')").should == '5'
  end

  it 'raises an TypeError for interpolated strings or other values' do
    if RUBY_ENGINE == 'natalie'
      -> { eval("1 + #{1}") }.should raise_error(TypeError)
    end
    -> { eval(1) }.should raise_error(TypeError)
  end

  it 'raises a SyntaxError for invalid parse' do
    -> { eval('())))') }.should raise_error(SyntaxError)
  end
end
