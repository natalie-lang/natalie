require_relative '../spec_helper'
require_relative '../../spec/library/yaml/fixtures/example_class'
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

  # https://github.com/ruby/spec/pull/1114
  it 'can dump an anonymous struct' do
    person = Struct.new(:name, :gender)
    person.new("Jane", "female").to_yaml.should match_yaml("--- !ruby/struct\nname: Jane\ngender: female\n")
  end

  # https://github.com/ruby/spec/pull/1114
  it "returns the YAML representation of a Class object" do
    YAMLSpecs::Example.to_yaml.should match_yaml("--- !ruby/class 'YAMLSpecs::Example'\n")
  end
end
