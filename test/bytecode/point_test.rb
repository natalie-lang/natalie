require_relative '../spec_helper'

# `point.rb` from https://ruby-compilers.com/examples/
describe 'define a class and use it' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run the point example' do
    code = <<~'RUBY'
      class Point
        def initialize(x, y)
          @x = x
          @y = y
        end

        def to_s
          "(#{@x}, #{@y})"
        end
      end

      def create_point(x, y)
        Point.new(x, y)
      end

      def print_point(point)
        point.to_s
      end

      point = create_point(14, 2)
      puts print_point(point)
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "(14, 2)\n"
  end
end
