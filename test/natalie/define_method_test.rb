require_relative '../spec_helper'

describe 'define_singleton_method' do
  it 'defines a new method on the object given a name and block' do
    obj = Object.new
    obj.define_singleton_method(:foo) { 'foo method' }
    obj.foo.should == 'foo method'
  end
end

describe '#define_method' do
  it 'warns on overwriting non-overwritable function' do
    [Integer, Float].each do |numeric_class|
      %i[+ !].each do |method|
        cmd = <<~RUBY
          #{numeric_class}.define_method(:#{method}, ->(*) {})
        RUBY

        ruby_exe(cmd, args: '2>&1').should include_all(
                                             "Natalie will not allow overwriting #{numeric_class}##{method}. If you depend on it, please compile with the `--allow-overwrites` flag.\n",
                                           )
      end
    end

    cmd = <<~RUBY
    Array.define_method(:min, ->(*) {  })
  RUBY
    ruby_exe(cmd, args: '2>&1').should include_all(
                                         "Natalie will not allow overwriting Array#min. If you depend on it, please compile with the `--allow-overwrites` flag.\n",
                                       )
  end

  it 'allows overwriting non-overwritable function if compiler flag passed' do
    [Integer, Float].each do |numeric_class|
      %i[+ !].each do |method|
        cmd = <<~RUBY
          #{numeric_class}.define_method(:#{method}, ->(*) {})
        RUBY
        ruby_exe(cmd, args: '--allow-overwrites 2>&1').should == ''
      end
    end

    cmd = <<~RUBY
    Array.define_method(:min, ->(*) {  })
  RUBY
    ruby_exe(cmd, args: '--allow-overwrites 2>&1').should == ''
  end
end
