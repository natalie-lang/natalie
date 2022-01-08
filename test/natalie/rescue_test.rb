require_relative '../spec_helper'

class MyException < StandardError
end

describe 'begin/rescue/else' do
  it 'runs the rescue block with the exception named' do
    r =
      begin
        a = []
        a[1, 2, 3]
      rescue => e
        e
      end
    r.message.should == 'wrong number of arguments (given 3, expected 1..2)'
    r =
      begin
        raise 'foo'
      rescue => e
        e
      end
    r.message.should == 'foo'
  end

  it 'uses the same scope' do
    begin
      x = 1
    rescue StandardError
    end
    x.should == 1
    begin
      x = 2
    rescue StandardError
    else
      x.should == 2
    end
  end

  it 'can return early' do
    def test1
      begin
        raise 'foo'
      rescue StandardError
        return 'return from rescue'
      end
      'not reached'
    end

    def test2
      begin
        raise 'foo'
      rescue StandardError
        return 'return from rescue' if true
      end
      'not reached'
    end

    test1.should == 'return from rescue'
    test2.should == 'return from rescue'
  end

  it 'can break out of a while/until loop' do
    def test_while1
      result =
        while true
          begin
            raise 'foo'
          rescue StandardError
            break 'bar'
          end
        end
      "break in while works: #{result}"
    end

    def test_while2
      while true
        begin
          raise 'foo'
        rescue StandardError
          break if true
        end
      end
      'break in if in while works'
    end

    def test_until1
      result =
        until false
          begin
            raise 'foo'
          rescue StandardError
            break 'bar'
          end
        end
      "break in until works: #{result}"
    end

    def test_until2
      until false
        begin
          raise 'foo'
        rescue StandardError
          break if true
        end
      end
      'break in if in until works'
    end

    test_while1.should == 'break in while works: bar'
    test_while2.should == 'break in if in while works'
    test_until1.should == 'break in until works: bar'
    test_until2.should == 'break in if in until works'
  end
end

describe 'raise' do
  it 'can raise a built-in exception class' do
    r =
      begin
        raise StandardError
      rescue => e
        e
      end
    r.message.should == 'StandardError'
  end

  it 'can raise a built-in exception class with a custom message string' do
    r =
      begin
        raise StandardError, 'foo'
      rescue => e
        e
      end
    r.message.should == 'foo'
  end

  it 'can raise a custom exception class with a custom message string' do
    r =
      begin
        raise MyException, 'foo'
      rescue => e
        e
      end
    r.message.should == 'foo'
  end

  it 'can raise a custom exception instance' do
    r =
      begin
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
    rescue StandardError
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
    rescue StandardError
    end
    x.should == 2
  end

  it 'can see variables defined in the begin section' do
    begin
      x = 1
    rescue StandardError
    ensure
      x.should == 1
    end
    begin
      x = 2
      raise 'here'
    rescue StandardError
      x.should == 2
    ensure
      x.should == 2
    end
  end
end
