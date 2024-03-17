require_relative '../spec_helper'

describe 'puts a const' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a puts with a const' do
    code = 'puts Float'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "Float\n"
  end

  it 'can run a puts with a const with path' do
    code = 'puts Float::NAN'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "NaN\n"
  end
end
