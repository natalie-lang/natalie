require 'minitest/spec'
require 'minitest/autorun'
require_relative '../support/nat_binary'

describe 'object_id' do
  describe 'inspect' do
    TESTS = {
      'p Object.new' => /^#<Object:0x[0-9a-f]+>$/,
      'p Class.new' => /^#<Class:0x[0-9a-f]+>$/,
      'p Class.new.new' => /^#<#<Class:0x[0-9a-f]+>:0x[0-9a-f]+>$/,
      'class C; end; p C' => /^C$/,
      'p Module.new' => /^#<Module:0x[0-9a-f]+>$/,
      'module M; end; p M' => /^M$/,
      'p([:foo.object_id, :foo.object_id])' => /^\[([0-9]+), \1\]$/,
      'p([true.object_id, true.object_id])' => /^\[([0-9]+), \1\]$/,
      'p([false.object_id, false.object_id])' => /^\[([0-9]+), \1\]$/,
      'p([nil.object_id, nil.object_id])' => /^\[([0-9]+), \1\]$/,
    }

    before do
      @output = `#{NAT_BINARY} -e #{TESTS.keys.join(';').inspect}`.strip.split(/\n/)
      expect($?).must_be :success?
    end

    specify { TESTS.values.each_with_index { |output_match, index| expect(@output[index]).must_match(output_match) } }
  end
end
