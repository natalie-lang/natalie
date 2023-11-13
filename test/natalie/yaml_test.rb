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
end
