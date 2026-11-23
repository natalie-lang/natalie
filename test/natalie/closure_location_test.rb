require_relative '../spec_helper'

TOP_LEVEL_CALLER_LABEL = -> { caller(0, 1).first.split(":in '", 2).last.delete_suffix("'") }

def invoke_top_level_caller_label
  TOP_LEVEL_CALLER_LABEL.call
end

describe 'closure source locations' do
  it 'retains the top-level source scope after the script finishes' do
    GC.start
    TOP_LEVEL_CALLER_LABEL.call.should == 'block in <main>'
  end

  it 'uses the captured source scope rather than the invoking method' do
    invoke_top_level_caller_label.should == 'block in <main>'
  end
end
