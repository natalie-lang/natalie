require_relative '../spec_helper'
require 'fiddle'

describe 'Fiddle' do
  it 'can open a library and call a function' do
    lib = Fiddle.dlopen("build/librubyparser.#{RbConfig::CONFIG['SOEXT']}")
    fn = Fiddle::Function.new(lib['pm_version'], [], Fiddle::TYPE_VOIDP)
    fn.call.to_s.should =~ /^\d+.\d+\.\d+$/
  end
end
