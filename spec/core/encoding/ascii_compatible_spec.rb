require_relative '../../spec_helper'

describe "Encoding#ascii_compatible?" do
  it "returns true if self represents an ASCII-compatible encoding" do
    Encoding::UTF_8.ascii_compatible?.should be_true
  end

  # NATFIXME: add UTF_16LE encoding
  xit "returns false if self does not represent an ASCII-compatible encoding" do
    Encoding::UTF_16LE.ascii_compatible?.should be_false
  end
end
