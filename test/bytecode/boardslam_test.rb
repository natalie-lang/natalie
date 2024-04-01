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
      #{File.read(File.join(__dir__, '../../examples/boardslam.rb'))}
      board = BoardSlam.new(3, 5, 1)
      puts board.missing.sort.join(', ')
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "19, 30\n"
  end
end
