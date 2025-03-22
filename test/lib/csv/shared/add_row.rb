require 'csv'

describe :csv_add_row, shared: true do
  it 'appends array to self' do
    CSV
      .generate do |csv|
        csv.send(@method, [1, 'a', :z])
        csv.send(@method, [2, 'b', :y])
        csv.send(@method, [3, nil, :x])
        csv.send(@method, [])
      end
      .should == "1,a,z\n2,b,y\n3,,x\n\n"
  end

  it 'appends CSV::Row to self' do
    CSV
      .generate do |csv|
        csv.send(@method, CSV::Row.new([], [1, 'a', :z]))
        csv.send(@method, CSV::Row.new([], [2, 'b', :y]))
        csv.send(@method, CSV::Row.new([], [3, 'c', :x]))
      end
      .should == "1,a,z\n2,b,y\n3,c,x\n"
  end

  it 'does not append CSV::Row headers' do
    headers = %w[Name Count]
    CSV
      .generate do |csv|
        csv.send(@method, CSV::Row.new(headers, ['foo', 0]))
        csv.send(@method, CSV::Row.new(headers, ['bar', 1]))
        csv.send(@method, CSV::Row.new(headers, ['baz', 2]))
      end
      .should == "foo,0\nbar,1\nbaz,2\n"
  end

  it 'appends hash of headers and values' do
    CSV
      .generate(headers: %w[Name Count]) do |csv|
        csv.send(@method, { 'Name' => 'foo', 'Count' => 2, 'Other' => 4 })
        csv.send(@method, { 'Other' => 4 })
        csv.send(@method, {})
      end
      .should == "foo,2\n,\n,\n"
  end

  it 'increments lineno' do
    csv = CSV.new('')
    csv.lineno.should == 0
    csv.send(@method, [1, 2, 3])
    csv.lineno.should == 1
  end

  it 'sets headers of self if headers is true but not set yet' do
    CSV
      .generate(headers: true) do |csv|
        csv.send(@method, %w[Name Count])
        csv.send(@method, ['foo', 1])
        csv.headers.should == %w[Name Count]
      end
      .should == "Name,Count\nfoo,1\n"
  end

  it 'appends any object answering #collect' do
    x = mock('collectable')
    x.should_receive(:collect).and_return([1, 2, 3])

    CSV.generate { |csv| csv.send(@method, x) }.should == "1,2,3\n"
  end

  it 'converts each item in the array to a string' do
    ary = [1, 2, 3]
    str = 'foo'
    str.should_not_receive(:to_str)

    o1 = mock('both')
    o1.should_receive(:to_str).and_return('to_str')
    o1.should_not_receive(:to_s)

    o2 = mock('to_s')
    o2.should_receive(:to_s).and_return('to_s')

    csv_str = CSV.generate { |csv| csv.send(@method, [str, ary, o1, o2]) }
    csv_str.should == "foo,\"[1, 2, 3]\",to_str,to_s\n"
  end

  it 'respects column separator' do
    CSV.generate(col_sep: ';') { |csv| csv.send(@method, [1, 2, 3]) }.should == "1;2;3\n"
  end

  it 'respects row separator' do
    CSV
      .generate(row_sep: ';') do |csv|
        csv.send(@method, [1, 2, 3])
        csv.send(@method, [4, 5, 6])
      end
      .should == '1,2,3;4,5,6;'
  end

  it 'respects encoding' do
    # NATFIXME
  end

  it 'respects write converter' do
    upcase_converter = proc { |field| field.upcase }
    write_converters = [upcase_converter]

    CSV.generate(write_converters: write_converters) { |csv| csv.send(@method, %w[a b c]) }.should == "A,B,C\n"
  end

  it 'raises NoMethodError if row is not an Array or CSV::Row' do
    -> { CSV.new('').send(@method, :foo) }.should raise_error(
                 NoMethodError,
                 "undefined method `collect' for :foo:Symbol",
               )
    -> { CSV.new('').send(@method, 1) }.should raise_error(NoMethodError, "undefined method `collect' for 1:Integer")
    -> { CSV.new('').send(@method, 'foo') }.should raise_error(
                 NoMethodError,
                 "undefined method `collect' for \"foo\":String",
               )
    -> { CSV.new('').send(@method, nil) }.should raise_error(
                 NoMethodError,
                 "undefined method `collect' for nil:NilClass",
               )
  end

  it 'raises IOError if underlying IO is not opened for writing' do
    io = StringIO.new('', 'r')
    csv = CSV.new(io)
    -> { csv.send(@method, ['a', 'b', 1, 2]) }.should raise_error(IOError, 'not opened for writing')
  end
end
