require_relative '../../spec_helper'

describe "Encoding#dummy?" do
  it "returns false for proper encodings" do
    Encoding::UTF_8.dummy?.should be_false
    Encoding::ASCII.dummy?.should be_false
  end

  it "returns true for dummy encodings" do
    NATFIXME 'Implement ISO_2022_JP', exception: NameError, message: 'uninitialized constant Encoding::ISO_2022_JP' do
      Encoding::ISO_2022_JP.dummy?.should be_true
    end
    NATFIXME 'Implement CP50221', exception: NameError, message: 'uninitialized constant Encoding::CP50221' do
      Encoding::CP50221.dummy?.should be_true
    end
    NATFIXME 'Implement UTF_7', exception: NameError, message: 'uninitialized constant Encoding::UTF_7' do
      Encoding::UTF_7.dummy?.should be_true
    end
  end

  it "returns true for UTF_16 and UTF_32" do
    Encoding::UTF_16.should.dummy?
    Encoding::UTF_32.should.dummy?
  end

  it "implies not #ascii_compatible?" do
    Encoding.list.select(&:dummy?).each do |encoding|
      encoding.should_not.ascii_compatible?
    end
  end
end
