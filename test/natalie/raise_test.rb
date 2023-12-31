require_relative '../spec_helper'

describe 'raise' do
  it 'constructs an exception without a call to new' do
    begin
      raise SystemExit, 123
    rescue SystemExit => e
      e.status.should == 123
    end
  end
end
