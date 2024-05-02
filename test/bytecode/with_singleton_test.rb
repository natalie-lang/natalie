require_relative '../spec_helper'

describe 'it can define a class with a singleton part' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can define a class with a singleton part' do
    code = <<~RUBY
      class Foo
        class << self
          def foo = "foo"
        end
      end
      puts Foo.foo
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "foo\n"
  end
end
