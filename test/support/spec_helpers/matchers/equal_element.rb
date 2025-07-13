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

class EqualElementMatcher
  def initialize(element, attributes = nil, content = nil, options = {})
    @element = element
    @attributes = attributes
    @content = content
    @options = options
  end

  def matches?(actual)
    @actual = actual

    matched = true

    if @options[:not_closed]
      matched &&= actual =~ /^#{Regexp.quote("<" + @element)}.*#{Regexp.quote(">" + (@content || ''))}$/
    else
      matched &&= actual =~ /^#{Regexp.quote("<" + @element)}/
      matched &&= actual =~ /#{Regexp.quote("</" + @element + ">")}$/
      matched &&= actual =~ /#{Regexp.quote(">" + @content + "</")}/ if @content
    end

    if @attributes
      if @attributes.empty?
        matched &&= actual.scan(/\w+\=\"(.*)\"/).size == 0
      else
        @attributes.each do |key, value|
          if value == true
            matched &&= (actual.scan(/#{Regexp.quote(key)}(\s|>)/).size == 1)
          else
            matched &&= (actual.scan(%Q{ #{key}="#{value}"}).size == 1)
          end
        end
      end
    end

    !!matched
  end

  def failure_message
    ["Expected #{MSpec.format(@actual)}",
     "to be a '#{@element}' element with #{attributes_for_failure_message} and #{content_for_failure_message}"]
  end

  def negative_failure_message
    ["Expected #{MSpec.format(@actual)}",
      "not to be a '#{@element}' element with #{attributes_for_failure_message} and #{content_for_failure_message}"]
  end

  def attributes_for_failure_message
    if @attributes
      if @attributes.empty?
        "no attributes"
      else
        @attributes.inject([]) { |memo, n| memo << %Q{#{n[0]}="#{n[1]}"} }.join(" ")
      end
    else
      "any attributes"
    end
  end

  def content_for_failure_message
    if @content
      if @content.empty?
        "no content"
      else
        "#{@content.inspect} as content"
      end
    else
      "any content"
    end
  end
end

module MSpecMatchers
  private def equal_element(*args)
    EqualElementMatcher.new(*args)
  end
end
