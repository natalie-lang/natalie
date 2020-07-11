require_relative '../spec_helper'

class MyException < StandardError
end

describe 'begin/rescue/else' do
  it 'runs the rescue block with the exception named' do
    r = begin
          a = []
          a[1, 2, 3]
        rescue => e
          e
        end
    r.message.should == "wrong number of arguments (given 3, expected 1..2)"
    r = begin
          raise "foo"
        rescue => e
          e
        end
    r.message.should == "foo"
  end

  it 'uses the same scope' do
    begin
      x = 1
    rescue
    end
    x.should == 1
    begin
      x = 2
    rescue
    else
      x.should == 2
    end
  end
end

describe 'raise' do
  it 'can raise a built-in exception class' do
    r = begin
          raise StandardError
        rescue => e
          e
        end
    r.message.should == 'StandardError'
  end

  it 'can raise a built-in exception class with a custom messsage string' do
    r = begin
          raise StandardError, 'foo'
        rescue => e
          e
        end
    r.message.should == 'foo'
  end

  it 'can raise a custom exception class with a custom messsage string' do
    r = begin
          raise MyException, 'foo'
        rescue => e
          e
        end
    r.message.should == 'foo'
  end

  it 'can raise a custom exception instance' do
    r = begin
          raise MyException.new('foo')
        rescue => e
          e
        end
    r.message.should == 'foo'
  end
end

describe 'ensure' do
  it 'always runs, even if there is a caught exception' do
    x = 1
    begin
      raise 'error'
    rescue
    ensure
      x = 2
    end
    x.should == 2
  end

  it 'always runs, even if there is an uncaught exception' do
    x = 1
    begin
      begin
        raise 'error'
      ensure
        x = 2
      end
    rescue
    end
    x.should == 2
  end

  it 'can see variables defined in the begin section' do
    begin
      x = 1
    rescue
    ensure
      x.should == 1
    end
    begin
      x = 2
      raise 'here'
    rescue
      x.should == 2
    ensure
      x.should == 2
    end
  end
end
