require_relative '../spec_helper'

describe 'nil' do
  it 'cannot be created with .new' do
    -> { NilClass.new }.should raise_error(NoMethodError)
  end

  describe '=~' do
    it 'always returns nil' do
      (nil =~ /foo/).should be_nil
    end
  end
end
