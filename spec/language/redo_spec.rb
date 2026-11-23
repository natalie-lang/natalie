require_relative '../spec_helper'

describe "The redo statement" do
  it "restarts block execution if used within block" do
    a = []
    -> {
      a << 1
      redo if a.size < 2
      a << 2
    }.call
    a.should == [1, 1, 2]
  end

  it "re-executes the closest loop" do
    exist = [2,3]
    processed = []
    order = []
    [1,2,3,4].each do |x|
      order << x
      begin
        processed << x
        if exist.include?(x)
          raise StandardError, "included"
        end
      rescue StandardError
        exist.delete(x)
        redo
      end
    end
    processed.should == [1,2,2,3,3,4]
    exist.should == []
    order.should == [1,2,2,3,3,4]
  end

  it "re-executes the last step in enumeration" do
    list = []
    [1,2,3].each do |x|
      list << x
      break if list.size == 6
      redo if x == 3
    end
    list.should == [1,2,3,3,3,3]
  end

  it "triggers ensure block when re-executing a block" do
    list = []
    [1,2,3].each do |x|
      list << x
      begin
        list << 10*x
        redo if list.count(1) == 1
      ensure
        list << 100*x
      end
    end
    list.should == [1,10,100,1,10,100,2,20,200,3,30,300]
  end

  it "runs ensure before restarting a while iteration without checking the condition" do
    checks = 0
    steps = 0
    list = []
    while (checks += 1) < 2
      begin
        steps += 1
        list << :body
        redo if steps < 2
      ensure
        list << :ensure
      end
    end
    [checks, steps, list].should == [2, 2, [:body, :ensure, :body, :ensure]]
  end

  it "lets break from ensure cancel a pending redo" do
    list = []
    result = [1].each do
      raise 'redo did not exit' if list.size > 4
      begin
        list << :body
        redo
      ensure
        list << :ensure
        break :done
      end
    end
    result.should == :done
    list.should == [:body, :ensure]
  end

  it "lets return from ensure cancel a pending redo without replaying later blocks" do
    result = -> do
      begin
        redo
      ensure
        return :done
      end
    end.call
    list = []
    [1].each { |x| list << x }
    result.should == :done
    list.should == [1]
  end

  describe "in a method" do
    it "is invalid and raises a SyntaxError" do
      -> {
        eval("def m; redo; end")
      }.should raise_error(SyntaxError)
    end
  end
end
