require_relative '../spec_helper'

describe 'lambda' do
  describe '#call' do
    context 'with wrong number of arguments' do
      it 'raises an error' do
        -> { lambda { 'no args' }.call(1) }.should raise_error(ArgumentError)
        -> { lambda { |a| '1 arg' }.call }.should raise_error(ArgumentError)

        # calling to_proc doesn't change anything
        l1 = lambda { |a, b| 'lambda, now a proc 1' }
        -> { l1.to_proc.call }.should raise_error(ArgumentError)
        l2 = ->(a, b) { 'lambda, now a proc 2' }
        -> { l2.to_proc.call }.should raise_error(ArgumentError)
      end
    end
  end
end
