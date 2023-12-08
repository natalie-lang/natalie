require_relative '../spec_helper'

describe 'Pattern matching' do
  describe 'can be standalone assoc operator that' do
    it "matches literal value" do
      eval(<<-RUBY).should == 0
        [0, 1] => [a, 1]
        a
      RUBY
      eval(<<-RUBY).should == 1
        { a: 1, b: 2 } => { a: a }
        a
      RUBY
    end

    ruby_version_is "3.1" do
      it "raises if a literal value does not match" do
        -> {
          eval(<<-RUBY)
            [0, 1] => [a, 2]
            a
          RUBY
        }.should raise_error(NoMatchingPatternError, /2 === 1 does not return true/)

        -> {
          eval(<<-RUBY)
            { a: 1, b: 2 } => { a: a, b: 3 }
            a
          RUBY
        }.should raise_error(NoMatchingPatternError, /3 === 2 does not return true/)
      end

      it 'raises if the number of array elements does not match' do
        -> {
          eval(<<-RUBY)
            [1, 2, 3] => [a, b]
          RUBY
        }.should raise_error(StandardError, /length mismatch \(given 3, expected 2\)/)
        -> {
          eval(<<-RUBY)
            [1, 2, 3] => [a, b, c, d]
          RUBY
        }.should raise_error(StandardError, /length mismatch \(given 3, expected 4\)/)
      end
    end
  end
end
