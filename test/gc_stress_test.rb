# This convoluted mess stress the garbage collector when lots of threads
# are creating objects.

require_relative './spec_helper'
require 'thread_pool'

class SomeObject
  def initialize(data)
    @data = data
  end

  attr_reader :data

  def foo(val) = bar(val)
  def bar(val) = baz(val)
  def baz(val) = val
end

JOBS = Etc.nprocessors * 4

describe 'GC' do
  it 'does not crash' do
    work = []
    hash = {}
    objects = []
    data = []
    complete = 0
    pool = ThreadPool.new(Etc.nprocessors)
    JOBS.times do
      pool.schedule do
        # This looks dumb but it's just a bunch of busy work to create lots of objects. :-)
        o1 = SomeObject.new('foo')
        o2 = SomeObject.new('bar')
        o3 = SomeObject.new('baz')
        o4 = SomeObject.new('foo')
        o5 = SomeObject.new('bar')
        o6 = SomeObject.new('baz')
        o7 = SomeObject.new('foo')
        o8 = SomeObject.new('bar')
        o9 = SomeObject.new('baz')
        (Etc.nprocessors * 40).times do
          r = rand(1000)
          work << r
          obj = SomeObject.new("bar#{r}")
          objects << obj
          hash["foo#{r}"] = obj.foo obj.foo obj.foo obj.foo obj.foo obj.foo obj.foo obj.foo obj.foo obj.foo obj # lol
          data << objects.sample&.data
          work << hash
        end
        work << o1.inspect
        work << o2.inspect
        work << o3.inspect
        work << o4.inspect
        work << o5.inspect
        work << o6.inspect
        work << o7.inspect
        work << o8.inspect
        work << o9.inspect
        complete += 1
      end
    end
    sleep 0.1 while work.empty?
    while complete < JOBS
      GC.start
      puts "#{complete} of #{JOBS} complete"
    end
    pool.shutdown
  end
end
