require_relative '../spec_helper'

describe 'implicit conversions' do
  describe 'to array' do
    it 'raises no implicit conversion of class into Array when converting object' do
      -> { [].replace(1) }.should raise_error(TypeError, 'no implicit conversion of Integer into Array')
    end

    it 'raises no implicit conversion nil into Array when converting nil' do
      -> { [].replace(nil) }.should raise_error(TypeError, 'no implicit conversion of nil into Array')
    end

    it 'shows the class of #to_ary result' do
      x = Object.new
      def x.to_ary
        1
      end

      -> { [].replace(x) }.should raise_error(TypeError, "can't convert Object to Array (Object#to_ary gives Integer)")
    end

    it 'shows that #to_ary gives NilClass' do
      x = Object.new
      def x.to_ary
        nil
      end

      -> { [].replace(x) }.should raise_error(TypeError, "can't convert Object to Array (Object#to_ary gives NilClass)")
    end
  end
end
