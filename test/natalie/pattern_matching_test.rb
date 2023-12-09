require_relative '../spec_helper'

describe 'Pattern matching' do
  describe 'can be standalone assoc operator that' do
    it 'matches literal value' do
      [0, 1] => [a, 1]
      a.should == 0
      { a: 1, b: 2 } => { a: a }
      a.should == 1
    end

    it 'is a void value expression that cannot be returned' do
      a = :foo
      -> {
        eval(<<~RUBY)
          a = ([0, 1] => [a, 1])
        RUBY
      }.should raise_error(SyntaxError, /void value expression/)
      a.should == :foo

      -> {
        [0, 1] => [a, 1]
      }.().should == nil
    end

    it 'does not modify the original value' do
      a = [0, 1]
      a => [b, 1]
      a.should == a
    end

    it 'raises if a literal value does not match' do
      ruby_version_is '3.1' do
        -> {
          [0, 1] => [a, 2]
        }.should raise_error(NoMatchingPatternError, /2 === 1 does not return true/)

        -> {
          { a: 1, b: 2 } => { a: a, b: 3 }
        }.should raise_error(NoMatchingPatternError, /3 === 2 does not return true/)
      end
    end

    it 'raises if the number of array elements does not match' do
      ruby_version_is '3.1' do
        -> {
          [1, 2, 3] => [a, b]
        }.should raise_error(StandardError, /length mismatch \(given 3, expected 2\)/)
        -> {
          [1, 2, 3] => [a, b, c, d]
        }.should raise_error(StandardError, /length mismatch \(given 3, expected 4\)/)
      end
    end
  end
end
