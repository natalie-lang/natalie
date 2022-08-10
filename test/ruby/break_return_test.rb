require 'minitest/spec'
require 'minitest/autorun'
require_relative '../support/compare_rubies'
require_relative '../support/nat_binary'

describe 'break/return' do
  include CompareRubies

  it 'has same output in both Ruby and Natalie' do
    run_both_and_compare('test/ruby/break_return.rb')
  end
end
