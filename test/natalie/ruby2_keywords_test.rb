require_relative '../spec_helper'

describe '#ruby2_keywords' do
  it 'does not capture self when used as a method' do
    x = 9
    klass =
      Class.new do
        define_method(:plain, ->(*) { [self, x] })
        define_method(:wrapped, ->(*) { [self, x] }.ruby2_keywords)
      end
    obj = klass.new
    obj.wrapped.first.should == obj
    obj.wrapped.last.should == 9
    obj.plain.should == obj.wrapped
  end
end
