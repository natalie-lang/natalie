require_relative '../spec_helper'

describe 'backtrace' do
  it 'records code locations in modules and classes' do
    bt = nil
    lambda {
      module Foo
        class Bar
          class Baz < Buz # Buz is undefined
          end
        end
      end
    }.should raise_error(NameError) { |e| bt = e.backtrace }
    bt.filter_map { |l| l.match(/<[^>]+>/)&.to_s }.grep_v(/<top \(required\)>/).uniq.should == %w[
      <class:Bar>
      <module:Foo>
      <main>
    ]
  end
end
