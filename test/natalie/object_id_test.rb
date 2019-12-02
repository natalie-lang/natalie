require 'minitest/spec'
require 'minitest/autorun'

describe 'object_id' do
  describe 'inspect' do
    TESTS = {
      'p Object.new' => /^#<Object:0x[0-9a-f]+>$/,
      'p Class.new' => /^#<Class:0x[0-9a-f]+>$/,
      'p Class.new.new' => /^#<#<Class:0x[0-9a-f]+>:0x[0-9a-f]+>$/,
      'class C; end; p C' => /^C$/,
      'p Module.new' => /^#<Module:0x[0-9a-f]+>$/,
      'module M; end; p M' => /^M$/,
      'p([:foo.object_id, :foo.object_id])' => /^\[([0-9]+), \1\]$/
    }

    before do 
      @output = `bin/natalie -e #{TESTS.keys.join(';').inspect}`.strip.split(/\n/)
    end

    specify do
      TESTS.values.each_with_index do |output_match, index|
        @output[index].must_match(output_match)
      end
    end
  end
end
