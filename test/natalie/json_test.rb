require_relative '../spec_helper'
require 'json'

describe 'JSON' do
  describe '.parse' do
    it 'parses booleans' do
      JSON.parse('true').should == true
      JSON.parse('false').should == false
    end

    it 'parses null' do
      JSON.parse('null').should == nil
    end

    it 'parses integers' do
      JSON.parse('1234').should == 1234
      JSON.parse('-3000').should == -3000
      JSON.parse('0').should == 0
      JSON.parse('-0').should == 0
      -> { JSON.parse('01') }.should raise_error(JSON::ParserError)
      -> { JSON.parse('-01') }.should raise_error(JSON::ParserError)
    end

    it 'parses floats' do
      JSON.parse('1.1').should be_close(1.1, TOLERANCE)
      JSON.parse('1.2e3').should be_close(1200.0, TOLERANCE)
      JSON.parse('123e3').should be_close(123000.0, TOLERANCE)
      JSON.parse('123E3').should be_close(123000.0, TOLERANCE)
      JSON.parse('-123.4').should be_close(-123.4, TOLERANCE)
    end

    it 'parses strings' do
      JSON.parse('""').should == ''
      JSON.parse('"foo"').should == 'foo'
      JSON.parse('"foo\"bar"').should == 'foo"bar'
      JSON.parse('"\u002598"').should == '%98'
      JSON.parse('"\uf600"').should == ''
      JSON.parse('"\uF600"').should == ''
      -> { JSON.parse(%("foo\nbar")) }.should raise_error(JSON::ParserError)
      -> { JSON.parse('"\u00"') }.should raise_error(JSON::ParserError)
      # not valid according to Spec, but Ruby does this:
      JSON.parse('"\y"').should == 'y'
    end

    it 'parses arrays' do
      JSON.parse('[]').should == []
      JSON.parse('[1]').should == [1]
      JSON.parse('[1,2]').should == [1,2]
      -> { JSON.parse('[1,]') }.should raise_error(JSON::ParserError)
      -> { JSON.parse('[') }.should raise_error(JSON::ParserError)
    end

    it 'parses objects' do
      JSON.parse('{}').should == {}
      JSON.parse('{"foo":"bar"}').should == { 'foo' => 'bar' }
      JSON.parse('{"1":"foo","2":"bar"}').should == { '1' => 'foo', '2' => 'bar' }
      JSON.parse('{ "1" : "foo" , "2" : "bar" }').should == { '1' => 'foo', '2' => 'bar' }
      JSON.parse(%({\n"foo"\n:\n"bar"\n})).should == { 'foo' => 'bar' }
      -> { JSON.parse('{') }.should raise_error(JSON::ParserError)
      -> { JSON.parse('{1:"foo"}') }.should raise_error(JSON::ParserError)
      -> { JSON.parse('{"foo":}') }.should raise_error(JSON::ParserError)
    end

    it 'raise an error for unknown literals' do
      -> { JSON.parse('stuff') }.should raise_error(JSON::ParserError)
      -> { JSON.parse('truestuff') }.should raise_error(JSON::ParserError)
    end

    it 'parses a complex object' do
      result = JSON.parse(<<-END)
        {
          "Image": {
            "Width":  800,
            "Height": 600,
            "Title":  "View from 15th Floor",
            "Thumbnail": {
              "Url":    "http://www.example.com/image/481989943",
              "Height": 125,
              "Width":  100
            },
            "Animated" : false,
            "IDs": [116, 943, 234, 38793]
          }
        }
      END
      result.should == {
        'Image' => {
          'Width' => 800,
          'Height' => 600,
          'Title' => 'View from 15th Floor',
          'Thumbnail' => {
            'Url' => 'http://www.example.com/image/481989943',
            'Height' => 125,
            'Width' => 100
          },
          'Animated' => false,
          'IDs' => [116, 943, 234, 38793]
        }
      }
    end

    context 'with symbolize_names argument' do
      it 'returns symbol keys' do
        JSON.parse('{"foo":"bar"}', symbolize_names: true).should == { foo: 'bar' }
        JSON.parse('{"1":"foo","2":"bar"}', symbolize_names: true).should == { :'1' => 'foo', :'2' => 'bar' }
      end
    end
  end

  describe '.generate' do
    it 'generates booleans' do
      JSON.generate(true).should == 'true'
      JSON.generate(false).should == 'false'
    end

    it 'generates null' do
      JSON.generate(nil).should == 'null'
    end

    it 'generates integers' do
      JSON.generate(1234).should == '1234'
      JSON.generate(-3000).should == '-3000'
      JSON.generate(0).should == '0'
    end

    it 'generates floats' do
      JSON.generate(1.1).should == '1.1'
      JSON.generate(1.2e3).should == '1200.0'
      JSON.generate(-123.4).should == '-123.4'
    end

    it 'generates strings' do
      JSON.generate('').should == '""'
      JSON.generate('foo').should == '"foo"'
      JSON.generate(:foo).should == '"foo"'
      JSON.generate('foo"bar').should == '"foo\"bar"'
      JSON.generate('\\').should == '"\\\"'
      JSON.generate("\b").should == '"\\b"'
      JSON.generate("\t").should == '"\\t"'
      JSON.generate("\n").should == '"\\n"'
      JSON.generate("\f").should == '"\\f"'
      JSON.generate("\r").should == '"\\r"'
      JSON.generate("\u0000").should == '"\u0000"'
      JSON.generate("\u001f").should == '"\u001f"'
      JSON.generate("\u0020").should == '" "'
    end

    it 'generates arrays' do
      JSON.generate([]).should == '[]'
      JSON.generate([1]).should == '[1]'
      JSON.generate([1, 2]).should == '[1,2]'
    end

    it 'generates objects' do
      JSON.generate({}).should == '{}'
      JSON.generate({ foo: 'bar' }).should == '{"foo":"bar"}'
      JSON.generate({ 'foo' => 'bar' }).should == '{"foo":"bar"}'
      JSON.generate({ '1' => 'foo', '2' => 'bar' }).should == '{"1":"foo","2":"bar"}'
    end
  end
end
