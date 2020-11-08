require_relative '../spec_helper'

describe 'Fiber' do
  it 'works' do
    result = []
    f1 = Fiber.new do |*args|
      result << args
      result << 123
      result << Fiber.yield(6, 7)
      result << 456
      :end1
    end

    f2 = Fiber.new do |*args|
      result << args
      result << 'abc'
      result << Fiber.yield(8, 9)
      result << 'def'
      :end2
    end

    result << f1.resume(1, 2)
    result << f2.resume(3)
    result << f1.resume(4)
    result << f2.resume(5)
    result << 789

    result.should == [
      [1, 2],
      123,
      [6, 7],
      [3],
      'abc',
      [8, 9],
      4,
      456,
      :end1,
      5,
      'def',
      :end2,
      789
    ]
  end
end
