class ScriptError < Exception; end
  class NotImplementedError < ScriptError; end
  class SyntaxError < ScriptError; end

class StandardError < Exception; end
  class ArgumentError < StandardError; end
  class EncodingError < StandardError; end
  class FiberError < StandardError; end
  class IndexError < StandardError; end
    class StopIteration < IndexError; end
class NameError < StandardError
  attr_reader :name, :receiver
  def initialize(message=nil, name=nil, receiver: nil)
    super(message)
    @name = name
    @receiver = receiver
  end
  def local_variables
    [] # documented as "for internal use only"
  end
end

class NoMethodError < NameError
  attr_reader :args, :private_call?
  def initialize(message=nil, name=nil, args=nil, priv=false, receiver: nil)
    super(message, name, receiver: receiver)
    # Set instance variables on NoMethodError but not NameError
    @args = args
    instance_variable_set("@private_call?", !!priv)
  end
end

  class IOError < StandardError; end
    class EOFError < IOError; end
  class RangeError < StandardError; end
    class FloatDomainError < RangeError; end
  class RegexpError < StandardError; end
  class RuntimeError < StandardError; end
class FrozenError < RuntimeError
  attr_reader :receiver
  def initialize(message=nil, receiver: nil)
    super(message)
    @receiver = receiver
  end
end
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
  def initialize(message=nil, receiver: nil, key: nil)
    super(message)
    @receiver = receiver
    @key = key
  end
end
