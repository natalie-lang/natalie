require_relative '../spec_helper'

describe 'flip-flop state' do
  it 'evaluates both conditions on consecutive inclusive single-element ranges' do
    events = []
    4.times do
      if (
           events << :start
           true
         ) .. (
           events << :stop
           true
         )
        events << :body
      end
    end
    events.should == %i[start stop body] * 4
  end

  it 'keeps separate method activations and their closures independent' do
    factory =
      Class
        .new do
          def make
            proc do |i|
              if (i == 1) .. (i == 3)
                true
              else
                false
              end
            end
          end
        end
        .new
    first = factory.make
    second = factory.make
    first.call(1).should == true
    second.call(2).should == false
    first.call(2).should == true
  end

  it 'does not alias user locals with its internal state' do
    ruby_exe(<<~RUBY).should == "[1]\n:user\n"
      __flip_flop_1 = :user
      hits = []
      3.times { |i| hits << i if (i == 1)..(i == 1) }
      p hits
      p __flip_flop_1
    RUBY
  end

  it 'skips the end condition when an exclusive range starts again' do
    events = []
    4.times do
      if (
           events << :start
           true
         ) ... (
           events << :stop
           true
         )
        events << :body
      end
    end
    events.should == %i[start body stop body] * 2
  end
end
