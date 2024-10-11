# frozen_string_literal: true

class Time
  def to_date
    require 'date'

    Date.new(year, month, mday)
  end
end
