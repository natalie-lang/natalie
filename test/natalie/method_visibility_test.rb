require_relative '../spec_helper'

class Foo
  def public_foo
    'public'
  end

  def public_foo_calling_private_foo
    private_foo
  end

  private

  def private_foo
    'private'
  end

  public

  def public_foo2
    'public'
  end

  def private_foo2
    'private'
  end
  private :private_foo2
end

class Foo
  def another_public_foo
    'public'
  end
end

describe 'method visibility' do
  describe 'public by default' do
    it 'is visible outside the class' do
      Foo.new.public_foo.should == 'public'
      Foo.new.another_public_foo.should == 'public'
    end
  end

  describe 'explicit public' do
    it 'is visible outside the class' do
      Foo.new.public_foo2.should == 'public'
    end
  end

  describe 'explicit private' do
    it 'is not visible outside the class' do
      -> { Foo.new.private_foo }.should raise_error(
                                          NoMethodError,
                                          /^private method `private_foo' called for #<Foo:0x\X+>/,
                                        )
      -> { Foo.new.private_foo2 }.should raise_error(
                                           NoMethodError,
                                           /^private method `private_foo2' called for #<Foo:0x\X+>/,
                                         )
    end

    it 'is visible inside the class' do
      Foo.new.public_foo_calling_private_foo.should == 'private'
    end
  end
end
