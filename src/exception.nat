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
end

class LocalJumpError < StandardError
  attr_reader :exit_value
end
