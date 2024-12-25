require_relative '../spec_helper'

describe 'it in block' do
  it 'should use the local variable if exists' do
    it = 123
    [1, 2, 3].map { it }.should == [123, 123, 123]
  end

  # NATFIXME: Fix it argument
  guard -> { ruby_version_is(''...'3.4') || RUBY_ENGINE == 'natalie' } do
    it 'should use the method if no local variable exists' do
      # This test is hacky: we now depend on the `it` method of the specs
      suppress_warning do
        -> {
          # eval is required for suppress_warning
          eval('[1, 2, 3].map { it }')
        }.should raise_error(ArgumentError, 'wrong number of arguments (given 0, expected 1)')
      end
    end
  end

  # NATFIXME: Fix it argument
  guard -> { ruby_version_is('3.4') && RUBY_ENGINE != 'natalie' } do
    it 'should act as the first argument if no local variable exists' do
      # eval is required for suppress_warning
      eval('[1, 2, 3].map { it }').should == [1, 2, 3]
    end
  end
end
