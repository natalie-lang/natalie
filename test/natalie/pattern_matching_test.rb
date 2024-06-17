require_relative '../spec_helper'

# NATFIXME: Temprorary file until we can run some things in `language/pattern_matching_spec.rb`
describe 'pattern matching' do
  it 'can assign a single value' do
    1 => a
    a.should == 1
  end

  it 'has no used mode, always returns nil' do
    # (1 => a).should is a parse error, so use a lambda instead
    l = -> { 1 => a }
    l.call.should be_nil
  end
end
