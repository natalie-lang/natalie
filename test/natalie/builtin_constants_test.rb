# skip-ruby

require_relative '../spec_helper'

describe 'built-in constants' do
  describe 'RUBY_PLATFORM' do
    it 'matches that of the system ruby' do
      RUBY_PLATFORM.should == `ruby -e "print RUBY_PLATFORM"`
    end
  end

  describe 'RUBY_VERSION' do
    it 'returns something fairly modern' do
      # NOTE: This may change up or down depending on what we decide we want Natalie to be :-)
      RUBY_VERSION.should == '2.7.1'
    end
  end

  describe 'RUBY_ENGINE' do
    it 'returns "natalie"' do
      RUBY_ENGINE.should == 'natalie'
    end
  end
end
