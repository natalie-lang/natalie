require "natalie/inline"

class OptionParser
  class Switch
    attr_accessor :short_name, :long_name, :value_label, :value_type, :description, :value

    def initialize(short_name, long_name, value_label, description, block)
      @short_name = short_name&.sub(/^\-/, '')&.to_sym
      @long_name = long_name&.sub(/^\-\-/, '')&.to_sym
      @value_label = value_label
      @value_type = if value_label =~ /^\[.+\]$/
                     :optional
                   elsif value_label
                     :required
                   end
      @description = description
      @block = block
    end

    def match?(arg)
      name = arg.split('=', 2).first
      name == "-#{short_name}" || name == "--#{long_name}"
    end

    def consume!(args)
      arg = args.shift
      (arg, @value) = arg.split('=', 2)
      if @value.nil? && consume_value?(args.first)
        @value = args.shift
      end
      if @value.nil?
        @value = true
      end
      if @block
        @block.call(@value)
      end
    end

    def help
      str = [short_name_formatted, long_name_formatted].compact.join(', ')
      if value_label
        str << " #{value_label}"
      end
      str.ljust(33) + description
    end

    private

    def consume_value?(arg)
      [:required, :optional].include?(value_type) && arg !~ /^\-/
    end

    def short_name_formatted
      "-#{short_name}" if short_name
    end

    def long_name_formatted
      "--#{long_name}" if long_name
    end
  end

  def initialize
    parser = self
    @base_switches = [
      Switch.new('-h', '--help', nil, nil, -> { print parser.help; exit })
    ]
    @switches = []
    yield self if block_given?
  end

  attr_accessor :banner

  def on(*args, &block)
    (short_name, short_value) = args.grep(/^\-[^\-]/).first&.split(/[ =]/, 2)
    (long_name, long_value) = args.grep(/^\-\-/).first&.split(/[ =]/, 2)
    value = short_value || long_value
    description = args.grep_v(/^\-/).first
    @switches << Switch.new(short_name, long_name, value, description, block)
  end

  def parse(argv = ARGV, into: {})
    parse!(argv.dup, into: into)
  end

  def parse!(argv = ARGV, into: {})
    left_over = []
    matched = []
    position_argv = []
    while argv.any?
      if argv.first == '--'
        argv.shift
        while argv.any?
          position_argv << argv.shift
        end
      elsif (switch = all_switches.detect { |s| s.match?(argv.first) })
        switch.consume!(argv)
        matched << switch
      else
        position_argv << argv.shift
      end
    end
    matched.each do |switch|
      into[switch.long_name || switch.short_name] = switch.value
    end
    position_argv.each { |a| argv << a }
    argv
  end

  def help
    str = "#{banner}\n"
    @switches.each do |switch|
      str << "    #{switch.help}\n"
    end
    str
  end

  alias to_s help

  private

  def all_switches
    @base_switches + @switches
  end
end
