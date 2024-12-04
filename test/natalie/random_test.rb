require_relative '../spec_helper'

describe 'Random' do
  it 'should extend Random::Formatter' do
    Random.should be_kind_of(Random::Formatter)
  end
end
