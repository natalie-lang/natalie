# frozen_string_literal: true

require_relative '../spec_helper'
require 'stringio'
require 'pp'

class Foo
  def initialize
    @foo = 'foo'
  end
end

describe 'PP' do
  it 'it handles generic objects' do
    out = +''
    io = StringIO.new(out)
    obj = Foo.new
    PP.pp obj, io
    out.should =~ /\A#<Foo:0x[0-9a-f]+ @foo="foo">\n\z/
  end
end
