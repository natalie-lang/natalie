#
# Copyright (c) 2008 Engine Yard, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# ----
#
# This file is taken from https://github.com/ruby/mspec
#

class SpecVersion
  # If beginning implementations have a problem with this include, we can
  # manually implement the relational operators that are needed.
  include Comparable

  # SpecVersion handles comparison correctly for the context by filling in
  # missing version parts according to the value of +ceil+. If +ceil+ is
  # +false+, 0 digits fill in missing version parts. If +ceil+ is +true+, 9
  # digits fill in missing parts. (See e.g. VersionGuard and BugGuard.)
  def initialize(version, ceil = false)
    @version = version
    @ceil = ceil
    @integer = nil
  end

  def to_s
    @version
  end

  def to_str
    to_s
  end

  # Converts a string representation of a version major.minor.tiny
  # to an integer representation so that comparisons can be made. For example,
  # "2.2.10" < "2.2.2" would be false if compared as strings.
  def to_i
    unless @integer
      major, minor, tiny = @version.split '.'
      if @ceil
        tiny = 99 unless tiny
      end
      parts =
        [major, minor, tiny].map do |x|
          x = x.to_i
          x < 10 ? "0#{x}" : x.to_s
        end
      @integer = "1#{parts.join}".to_i
      # NATFIXME: Implement String#%
      # parts = [major, minor, tiny].map { |x| x.to_i }
      # @integer = ("1%02d%02d%02d" % parts).to_i
    end
    @integer
  end

  def to_int
    to_i
  end

  def <=>(other)
    if other.respond_to? :to_int
      other = other.to_int
      # NATFIXME: Implement Kernel#Integer
      # other = Integer(other.to_int)
    else
      other = SpecVersion.new(String(other)).to_i
    end

    self.to_i <=> other
  end
end
