require_relative '../spec_helper'

describe "Once-only regexp interpolation" do
  it "evaluates interpolation once and keeps the cached object alive" do
    count = 0
    build = proc { /#{count += 1}/o }
    first = build.call
    GC.start
    second = build.call
    count.should == 1
    second.should equal(first)
    second.source.should == "1"
    second.should.frozen?
  end

  it "retries interpolation after an exception" do
    count = 0
    build = proc { /#{count += 1; raise 'first' if count == 1; count}/o }
    -> { build.call }.should raise_error(RuntimeError, 'first')
    build.call.source.should == "2"
    build.call.source.should == "2"
    count.should == 2
  end

  it "evaluates interpolation once across concurrent callers" do
    count = 0
    build = proc { /#{count += 1; sleep 0.02; count}/o }
    results = 2.times.map { Thread.new { build.call } }.map(&:value)
    count.should == 1
    results[0].should equal(results[1])
  end

  it "allows recursive interpolation and caches the outer result" do
    depth = 0
    build = nil
    inner = nil
    build = proc do
      /#{depth += 1; if depth == 1; inner = build.call; 'outer'; else; 'inner'; end}/o
    end
    build.call.source.should == "outer"
    inner.source.should == "inner"
    build.call.source.should == "outer"
    depth.should == 2
  end

  it "does not cache a return from inside interpolation" do
    klass = Class.new do
      def regexp(early)
        /#{return :early if early; 1}/o
      end
    end
    object = klass.new
    object.regexp(true).should == :early
    first = object.regexp(false)
    object.regexp(true).should equal(first)
  end

  it "discards an inner cached value when an outer recursive evaluation raises" do
    depth = 0
    build = nil
    build = proc do
      /#{depth += 1; if depth == 1; build.call; raise 'outer'; else; "inner#{depth}"; end}/o
    end
    -> { build.call }.should raise_error(RuntimeError, 'outer')
    build.call.source.should == "inner3"
    depth.should == 3
  end

  it "does not cache next from a block inside interpolation" do
    build = proc { |early| /#{next :early if early; 1}/o }
    build.call(true).should == :early
    first = build.call(false)
    build.call(true).should equal(first)
  end

  it "freezes interpolated literals without the once flag" do
    /#{'pattern'}/.should.frozen?
  end
end

describe "Regexp literal caching" do
  it "keeps a literal alive when only its object id escapes a method call" do
    klass = Class.new do
      def literal
        /persistent/
      end
    end
    object = klass.new
    id = object.literal.object_id
    5.times { GC.start }
    object.literal.object_id.should == id
  end
end
