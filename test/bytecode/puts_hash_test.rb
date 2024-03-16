require_relative '../spec_helper'

describe 'puts a hash' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a puts with a hash with static values' do
    code = 'puts({foo: "foo", bar: "baz"})'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "{:foo=>\"foo\", :bar=>\"baz\"}\n"
  end
end
