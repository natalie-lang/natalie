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

  def greet_in_rescue
    "Hello."
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

  def greet_in_rescue
    begin
      "ARRRR. #{super}"
    rescue
    end
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

  it 'works inside a rescue block' do
    greeter = PirateGreeter.new
    greeter.greet_in_rescue.should == "ARRRR. Hello."
  end
end
