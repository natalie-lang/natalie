require_relative '../spec_helper'

describe 'puts a complex number' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a puts with a complex integer' do
    code = 'puts 2i'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: '--bytecode').should == ruby_exe(code)
  end

  it 'can run a puts with a complex float' do
    code = 'puts 2.5i'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: '--bytecode').should == ruby_exe(code)
  end

  it 'can construct a complex integer with a real part' do
    code = 'puts 2i + 1'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: '--bytecode').should == ruby_exe(code)
  end
end
