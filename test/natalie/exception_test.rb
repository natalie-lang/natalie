require_relative '../spec_helper'

describe '$@' do
  it 'returns nil if no exception has been thrown' do
    $@.should be_nil
  end
end
