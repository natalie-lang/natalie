class ScriptError < Exception; end
  class NotImplementedError < ScriptError; end
  class SyntaxError < ScriptError; end

class StandardError < Exception; end
  class ArgumentError < StandardError; end
  class EncodingError < StandardError; end
  class FiberError < StandardError; end
  class IndexError < StandardError; end
    class StopIteration < IndexError; end
  class NameError < StandardError; attr_reader :name; end
    class NoMethodError < NameError; end
  class IOError < StandardError; end
  class RangeError < StandardError; end
    class FloatDomainError < RangeError; end
  class RegexpError < StandardError; end
  class RuntimeError < StandardError; end
    class FrozenError < RuntimeError; end
  class TypeError < StandardError; end
  class ZeroDivisionError < StandardError; end
  class LoadError < StandardError; end

class Encoding
  class InvalidByteSequenceError < EncodingError; end
  class UndefinedConversionError < EncodingError; end
  class ConverterNotFoundError < EncodingError; end
end

class SystemExit < Exception
  def initialize(*args)
    @status = 0
    if args.size == 0
      super()
    elsif args.size == 1
      if args.first.is_a?(Integer)
        super()
        @status = args.first
      else
        super(args.first)
      end
    elsif args.size == 2
      if args.first.is_a?(Integer)
        super(args.last)
        @status = args.first
      elsif args.last.is_a?(Integer)
        super(args.first)
        @status = args.last
      end
    else
      raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 0..2)"
    end
  end

  attr_reader :status

  def success?
    @status.zero?
  end
end

class LocalJumpError < StandardError
  attr_reader :exit_value
end

class KeyError < IndexError
  attr_reader :receiver, :key
  def initialize(message, receiver = nil, key = nil)
    @receiver = receiver
    @key = key
  end
end
