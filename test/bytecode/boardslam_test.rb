require_relative '../spec_helper'

describe 'run the boardslam example' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run the boardslam example' do
    code = <<~RUBY
      board = BoardSlam.new(3, 5, 1)
      puts board.missing.sort.join(', ')
    RUBY
    filename = File.join(__dir__, '../../examples/boardslam.rb')
    code_bytecode = "#{File.read(filename)}\n#{code}"
    code_ruby = "load '#{filename}'\n#{code}"
    ruby_exe(code_bytecode, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == ruby_exe(code_ruby)
  end
end
