require 'stringio'
require 'csv/parser'
require 'csv/row'
require 'csv/writer'

class CSV
  DEFAULT_OPTIONS = {
    # For both parsing and generating.
    col_sep:            ',',
    row_sep:            :auto,
    quote_char:         '"',
    # For parsing.
    field_size_limit:   nil,
    converters:         nil,
    unconverted_fields: nil,
    headers:            false,
    return_headers:     false,
    header_converters:  nil,
    skip_blanks:        false,
    skip_lines:         nil,
    liberal_parsing:    false,
    nil_value:          nil,
    empty_value:        '',
    strip:              false,
    # For generating.
    write_headers:      nil,
    quote_empty:        true,
    force_quotes:       false,
    write_converters:   nil,
    write_nil_value:    nil,
    write_empty_value:  '',
  }.freeze

  attr_reader :encoding

  def initialize(io, **options)
    if io.is_a? String
      io = StringIO.new(io)
    end

    @io = io
    @encoding = @io.internal_encoding || @io.external_encoding
    @options = DEFAULT_OPTIONS.merge(options).freeze
    @line = nil
    @lineno = 0
  end

  def self.generate(string = '', **options)
    io = StringIO.new(string)
    csv = CSV.new(io, **options)
    yield csv
    io.string
  end

  def self.generate_line(ary, **options)
    generate('', **options) do |csv|
      csv << ary
    end
  end

  def self.open(file_path_or_io, mode = 'r')
    io =
      if file_path_or_io.is_a? IO
        file_path_or_io
      else
        File.open(file_path_or_io, mode)
      end
    yield CSV.new(io)
  end

  def self.parse(io, **options, &block)
    csv = CSV.new(io, **options)
    if block_given?
      csv.each(&block)
    else
      csv.read
    end
  end

  def <<(row)
    writer << row
  end
  alias add_row <<

  def col_sep
    @col_sep ||= @options[:col_sep].to_s.encode(encoding)

    if @col_sep.empty?
      raise ArgumentError, ":col_sep must be 1 or more characters: #{@options[:col_sep].inspect}"
    end

    @col_sep
  end

  def each
    while (row = shift)
      yield row
    end
  end

  def headers
    if @writer
      @writer.headers
    else
      parser.headers
    end
  end

  def liberal_parsing?
    !!@options[:liberal_parsing]
  end

  def lineno
    # If there is no parser we are writing!
    @parser&.lineno || @writer&.lineno || 0
  end

  def line
    parser.line
  end

  def parser
    @parser ||= Parser.new(@io, @options)
  end

  def read
    [].tap do |out|
      while (line = shift)
        out << line
      end
    end
  end
  alias readlines read

  def rewind
    @line = nil
    @lineno = 0
    @io.rewind
  end

  def row_sep
    @row_sep ||=
      if @options[:row_sep] == :auto
        parser.detect_row_separator || "\n"
      else
        @options[:row_sep].to_s
      end.encode(encoding)
  end

  def shift
    parser.next_line
  end
  alias readline shift

  def writer
    @writer ||= Writer.new(@io, @options)
  end

  def header_row?
    parser.header_row?
  end
end
