require 'minitest/spec'
require 'minitest/autorun'
require_relative '../support/compare_rubies'
require_relative '../support/nat_binary'

describe 'examples' do
  include CompareRubies

  describe 'examples/fib.rb' do
    it 'computes the Nth fibonacci number' do
      run_both_and_compare('examples/fib.rb', 5)
    end
  end

  describe 'examples/boardslam.rb' do
    it 'prints solutions to a 6x6 boardslam game card' do
      run_both_and_compare('examples/boardslam.rb', 1, 2, 3)
    end
  end
end
