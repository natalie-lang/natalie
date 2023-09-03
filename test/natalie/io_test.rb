require_relative '../spec_helper'

# This is a placeholder for https://github.com/ruby/spec/pull/1068. As soon as that spec is merged, this one
# can be removed in favour of the upstream specs.
describe 'IO#getbyte' do
  it "raises an IOError if the stream is not readable" do
    name = tmp("io_getbyte.txt")
    io = new_io(name, 'w')
    -> { io.getbyte }.should raise_error(IOError)
    io.close
    rm_r name
  end
end
