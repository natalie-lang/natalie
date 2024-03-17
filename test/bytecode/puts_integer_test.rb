require_relative '../spec_helper'

describe 'puts an integer' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a puts with 0' do
    code = 'puts 0'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "0\n"
  end

  it 'can run a puts with 1' do
    code = 'puts 1'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "1\n"
  end

  it 'can run a puts with -1' do
    code = 'puts -1'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "-1\n"
  end

  it 'can run a puts with the extreme positive value for regular int encoding' do
    num = (1 << 30) - 1
    code = "puts #{num}"
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "#{num}\n"
  end

  it 'can run a puts with the extreme negative value for regular int encoding' do
    num = -(1 << 30)
    code = "puts #{num}"
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "#{num}\n"
  end

  it 'can run a puts with a bignum' do
    num = 1 << 64
    code = "puts #{num}"
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "#{num}\n"
  end

  it 'can run a puts with a negative bignum' do
    num = 1 << 64
    code = "puts -#{num}"
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "-#{num}\n"
  end
end
