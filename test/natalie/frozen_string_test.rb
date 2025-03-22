# frozen_string_literal: true

require_relative '../spec_helper'

describe 'frozen string' do
  it 'does not produce a compilation error if it contains C++ comment delimiters' do
    ['foo // bar', 'foo /* bar */'].each do |s|
      # ensure each string is "used" by testing *something*
      s.should =~ /foo/
    end
  end
end
