# skip-ruby

require_relative '../spec_helper'

require 'natalie/inline'

describe 'StringObject' do
  it 'should be able to append any type with StringObject::append' do
    o = Object.new
    class << o
      __define_method__ :stringobject_append, %i[str val], <<~END
        str.assert_type(env, Object::Type::String, "String");
        str.as_string()->append(val);
        return str;
      END
    end
    o.stringobject_append('foo ', false).should == 'foo false'
    o.stringobject_append('foo ', true).should == 'foo true'
    o.stringobject_append('foo ', nil).should == 'foo nil'
    o.stringobject_append('foo ', :bar).should == 'foo bar'
    o.stringobject_append('foo ', 'bar').should == 'foo bar'
    o.stringobject_append('foo ', 123).should == 'foo 123'
    o.stringobject_append('foo ', 123.4).should == 'foo 123.4'

    # This is the current behaviour, and probably not what we want
    o.stringobject_append('foo ', %w[a b]).should =~ /ArrayObject.*StringObject.*StringObject/m
    o.stringobject_append('foo ', { a: 'x' }).should =~ /HashObject.*SymbolObject.*StringObject/m
    o.stringobject_append('foo ', o).should =~ /^foo <Object 0x/
  end
end
