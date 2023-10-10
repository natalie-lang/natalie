require_relative '../../spec_helper'
require_relative 'fixtures/classes'

ruby_version_is "3.2" do
  describe "Data.define" do
    it "accepts no arguments" do
      NATFIXME 'Implement Data.define', exception: NoMethodError, message: "undefined method `define' for Data:Class" do
        empty_data = Data.define
        empty_data.members.should == []
      end
    end

    it "accepts symbols" do
      NATFIXME 'Implement Data.define', exception: NoMethodError, message: "undefined method `define' for Data:Class" do
        movie = Data.define(:title, :year)
        movie.members.should == [:title, :year]
      end
    end

    it "accepts strings" do
      NATFIXME 'Implement Data.define', exception: NoMethodError, message: "undefined method `define' for Data:Class" do
        movie = Data.define("title", "year")
        movie.members.should == [:title, :year]
      end
    end

    it "accepts a mix of strings and symbols" do
      NATFIXME 'Implement Data.define', exception: NoMethodError, message: "undefined method `define' for Data:Class" do
        movie = Data.define("title", :year, "genre")
        movie.members.should == [:title, :year, :genre]
      end
    end

    it "accepts a block" do
      NATFIXME 'Implement Data.define', exception: NoMethodError, message: "undefined method `define' for Data:Class" do
        movie = Data.define(:title, :year) do
          def title_with_year
            "#{title} (#{year})"
          end
        end
        movie.members.should == [:title, :year]
        movie.new("Matrix", 1999).title_with_year.should == "Matrix (1999)"
      end
    end
  end
end
