# frozen_string_literal: true
#
# Can't define this in process.rb, that file is compiled before struct.rb.

module Process
  Tms = Struct.new(:utime, :stime, :cutime, :cstime)
end
