# encoding: US-ASCII

require_relative '../spec_helper'
require_relative '../support/encoding_magic_comment_eucjp'

describe 'encoding magic comment' do
  it 'sets the encoding for new strings' do
    'foo'.encoding.should == Encoding::US_ASCII
  end

  it 'can be changed per file' do
    encoding_magic_comment_eucjp.should == Encoding::EUC_JP
  end
end
