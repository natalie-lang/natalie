require_relative '../spec_helper'

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
    result = yield_test do
      break 'from block'
    end
    result.should == 'from block'
  end
end
