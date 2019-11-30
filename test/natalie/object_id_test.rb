require 'minitest/spec'
require 'minitest/autorun'

describe 'object_id' do
  describe 'inspect' do
    it 'works on anonymous classes' do
      `bin/natalie -e "p Class.new"`.strip.must_match(/^#<Class:0x[0-9a-f]+>$/)
      `bin/natalie -e "p Class.new.new"`.strip.must_match(/^#<#<Class:0x[0-9a-f]+>:0x[0-9a-f]+>$/)
    end

    it 'works on anonymous modules' do
      `bin/natalie -e "p Module.new"`.strip.must_match(/^#<Module:0x[0-9a-f]+>$/)
    end
  end
end
