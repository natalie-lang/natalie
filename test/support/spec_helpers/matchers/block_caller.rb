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

class BlockingMatcher
  def matches?(block)
    t = Thread.new do
      block.call
    end

    loop do
      case t.status
      when "sleep"    # blocked
        t.kill
        t.join
        return true
      when false      # terminated normally, so never blocked
        t.join
        return false
      when nil        # terminated exceptionally
        t.value
      else
        Thread.pass
      end
    end
  end

  def failure_message
    ['Expected the given Proc', 'to block the caller']
  end

  def negative_failure_message
    ['Expected the given Proc', 'to not block the caller']
  end
end

module MSpecMatchers
  private def block_caller
    BlockingMatcher.new
  end
end
