require_relative '../spec_helper'

class Foo
  def public_foo
    'public'
  end

  def public_foo_calling_private_foo
    private_foo
  end

  def public_foo_calling_private_foo_with_self
    self.private_foo # rubocop:disable Style/RedundantSelf
  end

  def public_foo_calling_protected_foo
    protected_foo
  end

  private

  def private_foo
    'private'
  end

  protected

  def protected_foo
    'protected'
  end

  public

  def initialize
  end

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

class Bar < Foo
  def public_bar_calling_protected_foo
    protected_foo
  end

  def public_bar_calling_private_foo
    private_foo
  end

  def public_bar_calling_protected_foo_on_another_object
    Foo.new.protected_foo
  end

  def public_bar_calling_private_foo_on_another_object
    Foo.new.private_foo
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
    ruby_version_is ''...'3.3' do
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
    end

    ruby_version_is '3.3' do
      it 'is not visible outside the class' do
        -> { Foo.new.private_foo }.should raise_error(
                                            NoMethodError,
                                            "private method `private_foo' called for an instance of Foo",
                                          )
        -> { Foo.new.private_foo2 }.should raise_error(
                                             NoMethodError,
                                             "private method `private_foo2' called for an instance of Foo",
                                           )
      end
    end

    it 'is visible inside the class' do
      Foo.new.public_foo_calling_private_foo.should == 'private'
    end

    it 'is visible inside a subclass' do
      Bar.new.public_bar_calling_private_foo.should == 'private'
    end

    it 'is not visible from inside a subclass calling the method on another object (not self)' do
      -> { Bar.new.public_bar_calling_private_foo_on_another_object }.should raise_error(NoMethodError)
    end

    it 'is callable from public method with using self' do
      Foo.new.public_foo_calling_private_foo_with_self.should == 'private'
    end
  end

  describe 'explicit protected' do
    ruby_version_is ''...'3.3' do
      it 'is not visible outside the class' do
        -> { Foo.new.protected_foo }.should raise_error(
                                              NoMethodError,
                                              /^protected method `protected_foo' called for #<Foo:0x\X+>/,
                                            )
      end
    end

    ruby_version_is '3.3' do
      it 'is not visible outside the class' do
        -> { Foo.new.protected_foo }.should raise_error(
                                              NoMethodError,
                                              "protected method `protected_foo' called for an instance of Foo",
                                            )
      end
    end

    it 'is visible inside the class' do
      Foo.new.public_foo_calling_protected_foo.should == 'protected'
    end

    it 'is visible from inside a subclass' do
      Bar.new.public_bar_calling_protected_foo.should == 'protected'
    end

    it 'is visible from inside a subclass calling the method on another object (not self)' do
      Bar.new.public_bar_calling_protected_foo_on_another_object.should == 'protected'
    end
  end

  describe '#initialize' do
    it 'is always private unless changed by name' do
      Foo.private_instance_methods.should include_all(:initialize)
      Foo.send(:public, :initialize)
      Foo.private_instance_methods.should_not include_all(:initialize)
      Foo.public_instance_methods.should include_all(:initialize)
    end
  end
end
