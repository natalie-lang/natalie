# encoding: US-ASCII

require_relative '../spec_helper'
require_relative '../support/encoding_magic_comment_eucjp'

describe 'encoding magic comment' do
  it 'sets the encoding for new strings' do
    'foo'.encoding.should == Encoding::US_ASCII
    "#{1 + 1}".encoding.should == Encoding::US_ASCII
    <<-END.encoding.should == Encoding::US_ASCII
      foo
    END
  end

  it 'can be changed per file' do
    encoding_magic_comment_eucjp.should == Encoding::EUC_JP
  end
end

describe 'string escapes affect encoding' do
  it 'can be different from file encoding' do
    "\u{3042}".encoding.should == Encoding::UTF_8
    "\u{3042} #{1 + 1}".encoding.should == Encoding::UTF_8
    "#{1 + 1} \u{3042}".encoding.should == Encoding::UTF_8
    <<-END.encoding.should == Encoding::UTF_8
      \u{3042}
      #{1 + 1}
    END
    <<-END.encoding.should == Encoding::UTF_8
      #{1 + 1}
      \u{3042}
    END

    "\xFF".encoding.should == Encoding::ASCII_8BIT
    "\xFF #{1 + 1}".encoding.should == Encoding::ASCII_8BIT
    "#{1 + 1} \xFF".encoding.should == Encoding::ASCII_8BIT
    <<-END.encoding.should == Encoding::ASCII_8BIT
      \xFF
      #{1 + 1}
    END
    <<-END.encoding.should == Encoding::ASCII_8BIT
      #{1 + 1}
      \xFF
    END
  end
end
