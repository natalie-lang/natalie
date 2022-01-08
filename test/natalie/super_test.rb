require_relative '../spec_helper'

module GreetingModule
  def greet_from_module
    'Hello from a module.'
  end
end

class Greeter
  include GreetingModule

  def greet
    'Hello.'
  end

  def greet_no_arg
    'Hello.'
  end

  def greet_by_name(name)
    "Hello, #{name}."
  end

  def greet_by_name_implicitly(name)
    "Hello, #{name}."
  end

  def greet_in_rescue
    'Hello.'
  end

  def greet_in_block
    'Hello from the block.'
  end

  def greet_using_block
    yield
  end

  def greet_from_module
    super
  end

  def greet_without_super
    super
  end
end

class PirateGreeter < Greeter
  def greet
    "ARRRR. #{super}"
  end

  alias aliased_greet greet

  def greet_no_arg(_arg)
    "ARRRR. #{super()}"
  end

  def greet_by_name(name)
    "ARRRR. #{super name}"
  end

  def greet_by_name_implicitly(name)
    "ARRRR. #{super}"
  end

  def greet_in_rescue
    begin
      "ARRRR. #{super}"
    rescue StandardError
    end
  end

  def yield_to_block
    yield
  end

  def greet_in_block
    yield_to_block { "ARRRR. #{super}" }
  end

  def greet_using_block
    super { 'ARRRR. Hello using a block.' }
  end
end

class UppercasePirateGreeter < PirateGreeter
  def greet
    super.upcase
  end
end

describe 'super' do
  it 'works without args' do
    greeter = PirateGreeter.new
    greeter.greet.should == 'ARRRR. Hello.'
  end

  it 'works with args' do
    greeter = PirateGreeter.new
    greeter.greet_by_name('Tim').should == 'ARRRR. Hello, Tim.'
  end

  it 'works with implicit args' do
    greeter = PirateGreeter.new
    greeter.greet_by_name_implicitly('Tim').should == 'ARRRR. Hello, Tim.'
  end

  it 'works with explicit empty args' do
    greeter = PirateGreeter.new
    greeter.greet_no_arg('Tim').should == 'ARRRR. Hello.'
  end

  it 'works inside a rescue block' do
    greeter = PirateGreeter.new
    greeter.greet_in_rescue.should == 'ARRRR. Hello.'
  end

  it 'works inside a block' do
    greeter = PirateGreeter.new
    greeter.greet_in_block.should == 'ARRRR. Hello from the block.'
  end

  it 'works using a block' do
    greeter = PirateGreeter.new
    greeter.greet_using_block.should == 'ARRRR. Hello using a block.'
  end

  it 'works with superclass of superclass' do
    greeter = UppercasePirateGreeter.new
    greeter.greet.should == 'ARRRR. HELLO.'
  end

  it 'works with modules' do
    greeter = Greeter.new
    greeter.greet_from_module.should == 'Hello from a module.'
  end

  it 'raises an error when there is no super' do
    greeter = Greeter.new
    -> { greeter.greet_without_super }.should raise_error(NoMethodError)
  end

  it 'works with an aliased method' do
    greeter = PirateGreeter.new
    greeter.aliased_greet.should == 'ARRRR. Hello.'
  end
end
