# NOTE: This library is not internally compatible with the optparse library in MRI.
# since the API for OptionParser::Switch is changed. Though, you probably shouldn't
# be using it directly anyway.
class OptionParser
  class Switch
    attr_accessor :short_name, :long_name, :value_label, :value_type, :description, :value

    def initialize(short_name:, long_name:, block:, value_label: nil, description: nil, options: nil)
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
      @options = options
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

    COL_SIZE = 33

    def help
      summary = [short_name_formatted, long_name_formatted].compact.join(', ')
      if value_label
        summary << " #{value_label}"
      end
      out = [summary.ljust(COL_SIZE) + description]
      if @options
        @options.each do |value, value_description|
          out << "    #{value}".ljust(COL_SIZE) + value_description
        end
      end
      out.map { |line| '    ' + line }.join("\n")
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
      Switch.new(short_name: '-h', long_name: '--help', block: -> (*) { print parser.help; exit }),
      Switch.new(short_name: '-v', long_name: '--version', block: -> (*) { puts parser.version; exit }),
    ]
    @switches = []
    yield self if block_given?
  end

  attr_accessor :banner, :program_name, :version

  def on(*args, options: nil, &block)
    (short_name, short_value) = args.grep(/^\-[^\-]/).first&.split(/[ =]/, 2)
    (long_name, long_value) = args.grep(/^\-\-/).first&.split(/[ =]/, 2)
    value_label = short_value || long_value
    description = args.grep_v(/^\-/).first
    @switches << Switch.new(short_name: short_name, long_name: long_name, value_label: value_label, description: description, block: block, options: options)
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
    out = [banner]
    @switches.each do |switch|
      out << switch.help
    end
    out.join("\n") + "\n"
  end

  def version
    "#{@program_name || $0} #{@version || 'unknown'}"
  end

  alias to_s help

  private

  def all_switches
    @base_switches + @switches
  end
end
