=begin
Copyright (c) 2008 Engine Yard, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
=end

class InfinityMatcher
  def initialize(expected_sign)
    @expected_sign = expected_sign
  end

  def matches?(actual)
    @actual = actual
    @actual.kind_of?(Float) && @actual.infinite? == @expected_sign
  end

  def failure_message
    ["Expected #{@actual}", "to be #{"-" if @expected_sign == -1}Infinity"]
  end

  def negative_failure_message
    ["Expected #{@actual}", "not to be #{"-" if @expected_sign == -1}Infinity"]
  end
end

module MSpecMatchers
  private def be_positive_infinity
    InfinityMatcher.new(1)
  end

  private def be_negative_infinity
    InfinityMatcher.new(-1)
  end
end
