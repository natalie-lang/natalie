# skip-ruby

require_relative '../spec_helper'

describe 'built-in constants' do
  describe 'RUBY_PLATFORM' do
    it 'matches that of the system ruby' do
      RUBY_PLATFORM.should == `ruby -e "print RUBY_PLATFORM"`
    end
  end

  describe 'RUBY_VERSION' do
    it 'returns the version Natalie tries to be compatible with' do
      RUBY_VERSION.should =~ /^4\.\d+\.\d+$/
    end
  end

  describe 'RUBY_ENGINE' do
    it 'returns "natalie"' do
      RUBY_ENGINE.should == 'natalie'
    end
  end
end
