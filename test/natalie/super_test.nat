require_relative '../spec_helper'

class Greeter
  def greet
    "Hello."
  end

  def greet_by_name(name)
    "Hello, #{name}."
  end

  def greet_by_name_implicitly(name)
    "Hello, #{name}."
  end
end

class PirateGreeter < Greeter
  def greet
    "ARRRR. #{super}"
  end

  def greet_by_name(name)
    "ARRRR. #{super name}"
  end

  def greet_by_name_implicitly(name)
    "ARRRR. #{super}"
  end
end

describe 'super' do
  it 'works without args' do
    greeter = PirateGreeter.new
    greeter.greet.should == "ARRRR. Hello."
  end

  it 'works with args' do
    greeter = PirateGreeter.new
    greeter.greet_by_name('Tim').should == "ARRRR. Hello, Tim."
  end

  it 'works with implicit args' do
    greeter = PirateGreeter.new
    greeter.greet_by_name_implicitly('Tim').should == "ARRRR. Hello, Tim."
  end
end
