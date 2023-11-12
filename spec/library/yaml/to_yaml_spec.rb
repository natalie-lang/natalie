require_relative '../../spec_helper'
require_relative 'fixtures/common'
require_relative 'fixtures/example_class'

describe "Object#to_yaml" do

  it "returns the YAML representation of an Array object" do
    NATFIXME 'YAML.dump for Arrays', exception: NotImplementedError, message: 'TODO: Implement YAML output for Array' do
      %w( 30 ruby maz irb 99 ).to_yaml.gsub("'", '"').should match_yaml("--- \n- \"30\"\n- ruby\n- maz\n- irb\n- \"99\"\n")
    end
  end

  it "returns the YAML representation of a Hash object" do
    NATFIXME 'YAML.dump for Hashes', exception: NotImplementedError, message: 'TODO: Implement YAML output for Hash' do
      { "a" => "b"}.to_yaml.should match_yaml("--- \na: b\n")
    end
  end

  it "returns the YAML representation of a Class object" do
    NATFIXME 'YAML.dump for Class objects', exception: NotImplementedError, message: 'TODO: Implement YAML output for YAMLSpecs::Example' do
      YAMLSpecs::Example.new("baz").to_yaml.should match_yaml("--- !ruby/object:YAMLSpecs::Example\nname: baz\n")
    end
  end

  it "returns the YAML representation of a Date object" do
    require 'date'
    NATFIXME 'Implement Date.parse', exception: NoMethodError, message: "undefined method `parse' for Date:Class" do
      Date.parse('1997/12/30').to_yaml.should match_yaml("--- 1997-12-30\n")
    end
  end

  it "returns the YAML representation of a FalseClass" do
    false_klass = false
    false_klass.should be_kind_of(FalseClass)
    NATFIXME 'YAML.dump for booleans', exception: NotImplementedError, message: 'TODO: Implement YAML output for FalseClass' do
      false_klass.to_yaml.should match_yaml("--- false\n")
    end
  end

  it "returns the YAML representation of a Float object" do
    float = 1.2
    float.should be_kind_of(Float)
    NATFIXME 'YAML.dump for floats', exception: NotImplementedError, message: 'TODO: Implement YAML output for Float' do
      float.to_yaml.should match_yaml("--- 1.2\n")
    end
  end

  it "returns the YAML representation of an Integer object" do
    int = 20
    int.should be_kind_of(Integer)
    NATFIXME 'YAML.dump for Integers', exception: NotImplementedError, message: 'TODO: Implement YAML output for Integer' do
      int.to_yaml.should match_yaml("--- 20\n")
    end
  end

  it "returns the YAML representation of a NilClass object" do
    nil_klass = nil
    nil_klass.should be_kind_of(NilClass)
    NATFIXME 'YAML.dump for nil', exception: NotImplementedError, message: 'TODO: Implement YAML output for NilClass' do
      nil_klass.to_yaml.should match_yaml("--- \n")
    end
  end

  it "returns the YAML representation of a RegExp object" do
    NATFIXME 'YAML.dump for Regexps', exception: NotImplementedError, message: 'TODO: Implement YAML output for Regexp' do
      Regexp.new('^a-z+:\\s+\w+').to_yaml.should match_yaml("--- !ruby/regexp /^a-z+:\\s+\\w+/\n")
    end
  end

  it "returns the YAML representation of a String object" do
    "I love Ruby".to_yaml.should match_yaml("--- I love Ruby\n")
  end

  it "returns the YAML representation of a Struct object" do
    Person = Struct.new(:name, :gender)
    NATFIXME 'YAML.dump for Struct objects', exception: NotImplementedError, message: 'TODO: Implement YAML output for Person' do
      Person.new("Jane", "female").to_yaml.should match_yaml("--- !ruby/struct:Person\nname: Jane\ngender: female\n")
    end
  end

  it "returns the YAML representation of a Symbol object" do
    :symbol.to_yaml.should match_yaml("--- :symbol\n")
  end

  it "returns the YAML representation of a Time object" do
    NATFIXME 'YAML.dump for Time objects', exception: NotImplementedError, message: 'Implement YAML output for Time' do
      Time.utc(2000,"jan",1,20,15,1).to_yaml.sub(/\.0+/, "").should match_yaml("--- 2000-01-01 20:15:01 Z\n")
    end
  end

  it "returns the YAML representation of a TrueClass" do
    true_klass = true
    true_klass.should be_kind_of(TrueClass)
    NATFIXME 'YAML.dump for booleans', exception: NotImplementedError, message: 'TODO: Implement YAML output for TrueClass' do
      true_klass.to_yaml.should match_yaml("--- true\n")
    end
  end

  it "returns the YAML representation of a Error object" do
    NATFIXME 'YAML.dump for Error objects', exception: NotImplementedError, message: 'TODO: Implement YAML output for StandardError' do
      StandardError.new("foobar").to_yaml.should match_yaml("--- !ruby/exception:StandardError\nmessage: foobar\nbacktrace: \n")
    end
  end

  it "returns the YAML representation for Range objects" do
    NATFIXME 'YAML.dump for ranges', exception: NotImplementedError, message: 'TODO: Implement YAML output for Range' do
      yaml = Range.new(1,3).to_yaml
      yaml.include?("!ruby/range").should be_true
      yaml.include?("begin: 1").should be_true
      yaml.include?("end: 3").should be_true
      yaml.include?("excl: false").should be_true
    end
  end

  it "returns the YAML representation of numeric constants" do
    NATFIXME 'YAML.dump for floats', exception: NotImplementedError, message: 'TODO: Implement YAML output for Float' do
      nan_value.to_yaml.downcase.should match_yaml("--- .nan\n")
      infinity_value.to_yaml.downcase.should match_yaml("--- .inf\n")
      (-infinity_value).to_yaml.downcase.should match_yaml("--- -.inf\n")
      (0.0).to_yaml.should match_yaml("--- 0.0\n")
    end
  end

  it "returns the YAML representation of an array of hashes" do
    players = [{"a" => "b"}, {"b" => "c"}]
    NATFIXME 'YAML.dump for arrays and hashes', exception: NotImplementedError, message: 'TODO: Implement YAML output for Array' do
      players.to_yaml.should match_yaml("--- \n- a: b\n- b: c\n")
    end
  end
end
