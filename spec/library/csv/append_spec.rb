require_relative '../../spec_helper'
require 'csv'

describe "CSV#<<" do
  it "appends a row to the CSV" do
    csv_str = CSV.generate do |csv|
      csv << ["foo", 2, 3]
      csv << ["bar", 4]
      csv << [5, nil, 6]
      csv << []
    end
    csv_str.should == "foo,2,3\nbar,4\n5,,6\n\n"
  end

  it "converts each item in the array to a string" do
    ary = [1, 2, 3]
    str = 'foo'
    def str.to_str; 'to_str'; end
    o1 = Object.new
    def o1.to_s; 'to_s'; end
    def o1.to_str; 'to_str'; end
    o2 = Object.new
    def o2.to_s; 'to_s'; end
    csv_str = CSV.generate do |csv|
      csv << [str, ary, o1, o2]
    end
    csv_str.should == "foo,\"[1, 2, 3]\",to_str,to_s\n"
  end

  it "raises an error if the given value is not an array" do
    (-> do
      CSV.generate do |csv|
        csv << 1
      end
    end).should raise_error(NoMethodError)
    (-> do
      CSV.generate do |csv|
        csv << 'foo'
      end
    end).should raise_error(NoMethodError)
    (-> do
      CSV.generate do |csv|
        csv << nil
      end
    end).should raise_error(NoMethodError)
  end
end
