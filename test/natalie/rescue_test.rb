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
    r.message.should == 'wrong number of arguments (given 3, expected 1..2)'

    r2 ||= begin
             raise 'foo'
           rescue => e
             e
           end
    r2.message.should == 'foo'
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

  it 'sets the magic $! global' do
    begin
      raise 'foo'
    rescue
      $!.message.should == 'foo'
      begin
        raise 'bar'
      rescue
        $!.message.should == 'bar'
      end
      $!.message.should == 'foo'
    end
  end

  it 'does not get confused by nested rescues' do
    ran = []
    begin
      raise 'foo'
    rescue
      ran << :rescue
      begin
      rescue
      end
    else
      ran << :else
    end
    ran.should == [:rescue]
  end

  it 'does not get confused by an unused LocalJumpError' do
    ran = []
    l = -> {
      begin
        raise 'foo'
      rescue
        ran << :rescue
        nil.tap { return 'bar' if false } # return here could raise a LocalJumpError, but doesn't
      else
        ran << :else
      end
    }
    l.call
    ran.should == [:rescue]
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

  it 'can re-raise the current exception' do
    r =
      begin
        begin
          raise 'foo'
        rescue
          raise
        end
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
    re_raised_error = nil
    begin
      begin
        raise MyException, 'stuff'
      ensure
        x = 2
      end
    rescue => e
      re_raised_error = e
    end
    x.should == 2
    e.should be_an_instance_of(MyException)
    e.message.should == 'stuff'
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

  it 'does not return the value from the ensure section' do
    result = begin
               1
             rescue
               2
             ensure
               3
             end
    result.should == 1
    result = begin
               1
               raise 'foo'
             rescue
               2
             ensure
               3
             end
    result.should == 2
    result = begin
               1
             rescue
               2
             else
               3
             ensure
               4
             end
    result.should == 3
  end
end
