# skip-ruby

require_relative '../spec_helper'
$LOAD_PATH << File.expand_path('../support/load_path', __dir__)
require 'works'

describe '$LOAD_PATH' do
  it 'has some basic support for compile-time appending' do
    LoadPathWorks.should be_an_instance_of(Class)
  end

  it 'does not support conditional modification' do
    -> { $LOAD_PATH << 'foo' }.should raise_error(LoadError, 'Cannot maniuplate $LOAD_PATH at runtime')
  end
end
