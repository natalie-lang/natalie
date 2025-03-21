require_relative '../spec_helper'

describe 'instance_eval' do
  class Foo
    def foo
    end
    def self.bar
    end
  end

  before :each do
    @foo = Foo.new
  end

  it 'sets self inside the block' do
    Foo.instance_eval { self }.should == Foo
  end

  it 'passes self as parameter' do
    Foo.instance_eval { |foo| foo }.should == Foo
  end

  it 'works on singleton class' do
    klass = @foo.singleton_class
    klass.instance_eval do |foo|
      self.should == klass
      foo.should == klass
    end
  end

  describe 'alias' do
    it 'operates on the singleton class' do
      -> { Foo.instance_eval { alias foo2 foo } }.should raise_error(NameError)
      -> { Foo.instance_eval { alias bar2 bar } }.should_not raise_error
    end

    it 'works on object' do
      -> { @foo.instance_eval { alias foo2 foo } }.should_not raise_error
      -> { @foo.instance_eval { alias bar2 bar } }.should raise_error(NameError)
    end

    it 'works on singleton class' do
      klass = @foo.singleton_class
      -> { klass.instance_eval { alias foo2 foo } }.should raise_error(NameError)
      -> { klass.instance_eval { alias bar2 bar } }.should_not raise_error
    end
  end

  describe 'alias_method' do
    it 'operates on the class' do
      -> { Foo.instance_eval { alias_method :foo2, :foo } }.should_not raise_error
      -> { Foo.instance_eval { alias_method :bar2, :bar } }.should raise_error(NameError)
    end

    it 'works on singleton class' do
      klass = @foo.singleton_class
      -> { klass.instance_eval { alias_method :foo2, :foo } }.should_not raise_error
      -> { klass.instance_eval { alias_method :bar2, :bar } }.should raise_error(NameError)
    end
  end
end
