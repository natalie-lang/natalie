require_relative '../spec_helper'

describe 'return' do
  it 'returns early from a method' do
    def foo
      return 'foo'
      'bar'
    end
    foo.should == 'foo'
  end

  it 'returns early from a block' do
    def one
      [1, 2, 3].each { |i| [1, 2, 3].each { |i| return i if i == 1 } }
    end
    one.should == 1
  end

  it 'handles other errors properly' do
    def foo(x)
      [1].each { |i| return i if x == i }
      raise 'foo'
    end
    -> { foo(2) }.should raise_error(StandardError)
    foo(1).should == 1
  end

  it 'can return from a deeply nested loop' do
    def foo
      loop do
        while true
          return 1 until false
          return :not_this_one
        end
        return :not_this_one
      end
      :not_this_one
    end
    foo.should == 1
  end

  it 'can return from a begin in a deeply nested loop' do
    def foo
      loop do
        begin
          # => fn
          while true
            until false
              begin
                # => fn
                return 2
              rescue StandardError
              end
              return :not_this_one1
            end
            return :not_this_one2
          end
          return :not_this_one3
        rescue StandardError
        end
        return :not_this_one4
      end
      :not_this_one5
    end
    foo.should == 2
  end

  it 'can return from a begin/rescue in a deeply nested loop' do
    def foo
      loop do
        begin
          raise :foo
        rescue StandardError
          while true
            until false
              begin
                raise :foo
              rescue StandardError
                return 3
              end
            end
            return :not_this_one
          end
          return :not_this_one
        end
      end
      :not_this_one
    end
    foo.should == 3
  end

  it 'can return from a begin/rescue/else in a deeply nested loop' do
    def foo
      begin

      rescue StandardError
      else
        loop do
          while true
            until false
              begin

              rescue StandardError
              else
                return 4
              end
            end
            return :not_this_one
          end
          return :not_this_one
        end
        :not_this_one
      end
    end
    foo.should == 4
  end
end
