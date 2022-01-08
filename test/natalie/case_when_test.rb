require_relative '../spec_helper'

class CaseTestObj
  def initialize
    @compare_count = 0
  end

  attr_reader :compare_count

  def ===(other)
    @compare_count += 1
    true
  end
end

class PrivateCaseTestObj
  def ===(other)
    true
  end
  private :===
end

describe 'case/when' do
  it 'matches on equality' do
    result =
      case 'one'
      when 'one'
        1
      when 'two'
        2
      end
    result.should == 1
    result =
      case 3
      when 4
        'four'
      when 3
        'three'
      end
    result.should == 'three'
  end

  it 'returns nil if no match' do
    result =
      case 100
      when 1
        'one'
      when 2
        'two'
      end
    result.should == nil
  end

  it 'can have an empty when body' do
    result =
      case 1
      when 1
      when 2
        'two'
      end
    result.should == nil
    result =
      case 2
      when 1
      when 2
        'two'
      end
    result.should == 'two'
  end

  it 'returns else if no match' do
    result =
      case 100
      when 1
        'one'
      when 2
        'two'
      else
        'other'
      end
    result.should == 'other'
  end

  it 'matches on multiple values' do
    result =
      case 2
      when 3, 4
        'three or four'
      when 1, 2
        'one or two'
      end
    result.should == 'one or two'
  end

  it 'does not have unintended side-effects' do
    o1 = CaseTestObj.new
    o2 = CaseTestObj.new
    o3 = CaseTestObj.new
    result =
      case 1
      when o1, o2
        'one'
      when o3
        'other'
      end
    result.should == 'one'
    o1.compare_count.should == 1
    o2.compare_count.should == 0
    o3.compare_count.should == 0
  end

  it 'matches based on class' do
    result =
      case 1
      when Integer
        'Integer'
      when String
        'String'
      when NilClass
        'NilClass'
      else
        'unknown'
      end
    result.should == 'Integer'
    result =
      case 'foo'
      when Integer
        'Integer'
      when String
        'String'
      when NilClass
        'NilClass'
      else
        'unknown'
      end
    result.should == 'String'
    result =
      case nil
      when Integer
        'Integer'
      when String
        'String'
      when NilClass
        'NilClass'
      else
        'unknown'
      end
    result.should == 'NilClass'
  end

  it 'matches based on Regexp' do
    result =
      case 'tim'
      when /ti/
        'matched'
      end
    result.should == 'matched'
    result =
      case 'tim'
      when /xi/
        'matched'
      end
    result.should == nil
  end

  it 'does not require a value to match' do
    result =
      case
      when true
        'yep'
      when false
        'nope'
      end
    result.should == 'yep'
  end

  it 'does not compile the same when body more than once' do
    # the compiler will error if we are compiling the when body twice
    code = <<-END.gsub(/\n/, '; ')
      x = 1
      case x
      when 1, 2
        [1].each do
          puts 'hi'
        end
      end
    END

    result = `bin/natalie -e #{code.inspect} 2>&1`
    result.strip.should == 'hi'
    $?.to_i.should == 0
  end

  it 'calls #=== even if it is private' do
    o1 = PrivateCaseTestObj.new
    o2 = PrivateCaseTestObj.new
    -> {
      case 1
      when o1
        'one'
      when o2
        'two'
      end
    }.should_not raise_error
  end
end
