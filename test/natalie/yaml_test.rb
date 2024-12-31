require_relative '../spec_helper'
require 'yaml'

describe 'YAML.dump' do
  it 'supports an IO object as a second argument' do
    filename = tmp('yaml_dump')
    File.open(filename, 'w') do |fh|
      res = YAML.dump(['a', 'b'], fh)
      res.should == fh
    end
    File.read(filename).should == YAML.dump(['a', 'b'])
  ensure
    rm_r filename
  end

  it 'can load floats' do
    YAML.load('90.0').should be_kind_of(Float)
  end

  it 'can output more than 16 KiB' do
    -> { ('x' * 17_000).to_yaml }.should_not raise_error
  end
end
