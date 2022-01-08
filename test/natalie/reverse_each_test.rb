require_relative '../spec_helper'

class MyEnum
  include Enumerable

  def each(*args)
    args.each { |arg| yield arg }
  end
end

describe 'return_each' do
  it 'accepts list as *args and yields a reversed list when passed a block' do
    yielded = []
    MyEnum.new.reverse_each(1, 2, 3) { |i| yielded << i }
    yielded.should == [3, 2, 1]
  end

  it 'accepts *args, returns a reversed Enumerable Instance, that then yields reversed list when iterated through' do
    yielded = []
    e = MyEnum.new.reverse_each(1, 2, 3)
    e.each { |i| yielded << i }
    yielded.should == [3, 2, 1]
  end
end
