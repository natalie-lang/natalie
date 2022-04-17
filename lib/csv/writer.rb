class CSV
  class Writer
    attr_reader :lineno, :headers

    def initialize(io, options)
      @io = io
      @options = options
      if options[:headers].is_a? Array
        @headers = options[:headers]
      end
      @lineno = 0
    end

    def <<(row)
      case row
      when Row
        row = row.fields
      when Hash
        row = @headers.map { |header| row[header] }
      end

      if @options[:headers] == true
        @headers ||= row
      end

      row = row.collect do |field|
        quote(__apply_converters(field))
      end

      @io.write(row.join(@options[:col_sep]) + __row_sep)
      @lineno += 1
    end

    def quote(field)
      # FIXME: Regexp.escape these two options once we have that method to use :-)
      if field =~ /#{@options[:col_sep]}|#{@options[:quote_char]}/
        field.inspect
      else
        field
      end
    end

    private

    def __row_sep
      @options[:row_sep] == :auto ? "\n" : @options[:row_sep]
    end

    def __apply_converters(field)
      converters = @options[:write_converters]

      if !converters || converters.empty?
        converters = [-> (str) {
          if str.is_a?(String)
            str
          elsif str.respond_to?(:to_str)
            str.to_str
          elsif str.respond_to?(:to_s)
            str.to_s
          end
        }]
      end

      converters.each do |converter|
        field = converter.(field)
        unless field.is_a? String
          break
        end
      end
      field
    end
  end
end
