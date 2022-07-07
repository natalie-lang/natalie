require_relative '../../spec_helper'

describe "Encoding.aliases" do
  it "returns a Hash" do
    Encoding.aliases.should be_an_instance_of(Hash)
  end

  it "has Strings as keys" do
    Encoding.aliases.keys.each do |key|
      key.should be_an_instance_of(String)
    end
  end

  it "has Strings as values" do
    Encoding.aliases.values.each do |value|
      value.should be_an_instance_of(String)
    end
  end

  it "has alias names as its keys" do
    Encoding.aliases.key?('BINARY').should be_true
    Encoding.aliases.key?('ASCII').should be_true
  end

  it "has the names of the aliased encoding as its values" do
    Encoding.aliases['BINARY'].should == 'ASCII-8BIT'
    Encoding.aliases['ASCII'].should == 'US-ASCII'
  end

  # NATFIXME: add back once we have a concept of external default encoding
  xit "has an 'external' key with the external default encoding as its value" do
    Encoding.aliases['external'].should == Encoding.default_external.name
  end

  # NATFIXME: add back once we have a concept of locale charmap
  xit "has a 'locale' key and its value equals the name of the encoding found by the locale charmap" do
    Encoding.aliases['locale'].should == Encoding.find(Encoding.locale_charmap).name
  end

  # NATFIXME: add back once we have a find method
  xit "only contains valid aliased encodings" do
    Encoding.aliases.each do |aliased, original|
      Encoding.find(aliased).should == Encoding.find(original)
    end
  end
end
