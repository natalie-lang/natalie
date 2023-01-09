# -*- encoding: binary -*-
require_relative '../../../spec_helper'
require_relative '../fixtures/classes'
require_relative 'shared/basic'
require_relative 'shared/numeric_basic'

describe "Array#pack with format 'w'" do
  it_behaves_like :array_pack_basic, 'w'
  it_behaves_like :array_pack_basic_non_float, 'w'
  it_behaves_like :array_pack_arguments, 'w'
  it_behaves_like :array_pack_numeric_basic, 'w'

  # NATFIXME: encodes a BER-compressed integer
  xit "encodes a BER-compressed integer" do
    [ [[0],     "\x00"],
      [[1],     "\x01"],
      [[9999],  "\xce\x0f"],
      [[2**65], "\x84\x80\x80\x80\x80\x80\x80\x80\x80\x00"]
    ].should be_computed_by(:pack, "w")
  end

  # NATFIXME: calls #to_int to convert the pack argument to an Integer
  xit "calls #to_int to convert the pack argument to an Integer" do
    obj = mock('to_int')
    obj.should_receive(:to_int).and_return(5)
    [obj].pack("w").should == "\x05"
  end

  ruby_version_is ""..."3.3" do
    # NATFIXME: ignores NULL bytes between directives
    xit "ignores NULL bytes between directives" do
      [1, 2, 3].pack("w\x00w").should == "\x01\x02"
    end
  end

  ruby_version_is "3.3" do
    it "raise ArgumentError for NULL bytes between directives" do
      -> {
        [1, 2, 3].pack("w\x00w")
      }.should raise_error(ArgumentError, /unknown pack directive/)
    end
  end

  # NATFIXME: ignores spaces between directives
  xit "ignores spaces between directives" do
    [1, 2, 3].pack("w w").should == "\x01\x02"
  end

  # NATFIXME: raises an ArgumentError when passed a negative value
  xit "raises an ArgumentError when passed a negative value" do
    -> { [-1].pack("w") }.should raise_error(ArgumentError)
  end

  it "returns a binary string" do
    [1].pack('w').encoding.should == Encoding::BINARY
  end
end
