require_relative '../spec_helper'
require 'rbconfig'

describe 'RbConfig::CONFIG' do
  platform_is :linux do
    it 'uses the correct values for DLEXT and SOEXT' do
      RbConfig::CONFIG['DLEXT'].should == 'so'
      RbConfig::CONFIG['SOEXT'].should == 'so'
    end
  end

  platform_is :darwin do
    it 'uses the correct values for DLEXT and SOEXT' do
      RbConfig::CONFIG['DLEXT'].should == 'bundle'
      RbConfig::CONFIG['SOEXT'].should == 'dylib'
    end
  end
end
