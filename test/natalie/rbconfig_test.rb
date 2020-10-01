require_relative '../spec_helper'

require 'rbconfig'

describe 'RbConfig' do
  describe 'CONFIG' do
    it 'has a bindir that points to natalie' do
      RbConfig::CONFIG[:bindir].should == File.expand_path('../../../bin', __FILE__)
    end
  end
end
