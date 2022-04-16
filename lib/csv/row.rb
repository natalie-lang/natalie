class CSV
  class Row
    attr_reader :headers, :fields

    def initialize(headers, fields)
      @headers = headers
      @fields = fields
    end
  end
end
