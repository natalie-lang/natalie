require_relative '../spec_helper'

describe 'global aliases' do
  it 'should create a new global if the alias destination does not exist' do
    code = <<~RUBY
      alias $foo $bar
      $bar = 'bar'
      puts $foo
    RUBY
    ruby_exe(code).should == "bar\n"
  end
end
