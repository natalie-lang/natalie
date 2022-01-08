require_relative '../spec_helper'

def break_in_while_in_method
  x = 0
  while true
    x += 1
    break if x > 2
  end
  x
end

def break_in_until_in_method
  x = 0
  until false
    x += 1
    break if x > 2
  end
  x
end

def break_in_iter_in_method
  x = 0
  [1, 2, 3].each do |i|
    x += 1
    break if x > 2
  end
  x
end

def break_propagates(x, result)
  x.each do |y|
    result << y
    yield y
    result << y
  end
end

describe 'break' do
  it 'breaks from a block' do
    result = [1, 2, 3].each { break }
    result.should == nil
    result = [1, 2, 3].map { break }
    result.should == nil
    result = { 1 => 1, 2 => 2 }.each { break }
    result.should == nil
    result = 10.times { break }
    result.should == nil
  end

  # FIXME
  xit 'breaks from a block in a method' do
    break_in_while_in_method.should == 3
    break_in_until_in_method.should == 3
    break_in_iter_in_method.should == 3
  end

  it 'returns the value given' do
    result = [1, 2, 3].each { break 1 }
    result.should == 1
    result = [1, 2, 3].map { break 2, 3 }
    result.should == [2, 3]
    result = { 1 => 1, 2 => 2 }.each { break nil }
    result.should == nil
    result = 10.times { break 'foo' }
    result.should == 'foo'
  end

  def break_from_proc_test
    [1].each { break 'good' }
    Proc.new { break 'bad' }.call
  end

  it 'raises an error when breaking from a Proc' do
    -> { break_from_proc_test }.should raise_error(LocalJumpError, 'break from proc-closure')
  end

  def yield_test
    yield
    yield
    yield
    'end'
  end

  it 'returns early from a method that yielded' do
    result = yield_test { break 'from block' }
    result.should == 'from block'
  end

  it 'propagates the break to the env where the block was passed' do
    result = []
    break_propagates([1, 2, 3], result) { break }
    result.should == [1]
  end

  it 'breaks out of a rescue inside a loop' do
    x = 1
    loop do
      break if x > 1
      begin
        raise 'error'
      rescue StandardError
        break
      end
      x += 1
    end
    x.should == 1

    def foo
      x = 1
      loop do
        break if x > 1
        begin
          raise 'error'
        rescue StandardError
          yield x
        end
        x += 1
      end
      x
    end
    result = foo { |x| break x }
    result.should == 1
  end

  it 'breaks out of an else inside a loop' do
    r =
      loop do
        begin

        rescue StandardError
        else
          break :ok
        end
        break :bad
      end
    r.should == :ok

    def break_in_else_in_loop
      x = 1
      loop do
        break if x > 1
        begin

        rescue StandardError
        else
          yield x
        end
        x += 1
      end
      x
    end
    result = foo { |x| break x }
    result.should == 1
  end

  it 'breaks out of a begin inside a while' do
    r =
      while true
        begin
          break :ok
        rescue StandardError
        end
        break :bad
      end
    r.should == :ok

    def break_in_begin_in_while
      while true
        begin
          yield if true
        rescue StandardError
        end
        break :bad
      end
    end
    result = break_in_begin_in_while { |x| break :ok }
    result.should == :ok
  end

  it 'should not leak break flag' do
    x = 0
    while x < 2
      break while true
      x += 1
    end
    x.should == 2
  end
end
