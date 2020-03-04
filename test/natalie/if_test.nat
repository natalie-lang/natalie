require_relative '../spec_helper'

describe 'if' do
  it 'handles the true case' do
    r = if true
          1
        end
    r.should == 1
    r = if true
          1
        else
          2
        end
    r.should == 1
  end

  it 'handles the else case' do
    r = if false
          1
        else
          2
        end
    r.should == 2
  end

  it 'handles the elsif case' do
    r = if false
          1
        elsif true
          2
        else
          3
        end
    r.should == 2
    r = if false
          1
        elsif nil
          2
        elsif 1
          3
        else
          4
        end
    r.should == 3
  end

  it 'can see variables outside of it' do
    outside = 10
    r = if true
          outside
        end
    r.should == 10
  end

  it 'can be expressed as postfix' do
    x = 1 if nil
    x.should == nil
    x = 1 if false
    x.should == nil
    x = 1 if true
    x.should == 1
  end

  describe 'not' do
    it 'negates the boolean value of the expression' do
      r = if not true
            1
          else
            2
          end
      r.should == 2
      r = if !true
            3
          else
            4
          end
      r.should == 4
    end
  end
end

describe 'unless' do
  it 'handles the true case' do
    r = unless false
          1
        end
    r.should == 1
    r = unless false
          1
        else
          2
        end
    r.should == 1
  end

  it 'handles the else case' do
    r = unless true
          1
        else
          2
        end
    r.should == 2
  end

  it 'can see variables outside of it' do
    outside = 10
    r = unless false
          outside
        end
    r.should == 10
  end

  it 'can be expressed as postfix' do
    x = 1 unless true
    x.should == nil
    x = 1 unless 1
    x.should == nil
    x = 1 unless false
    x.should == 1
  end

  describe 'not' do
    it 'negates the boolean value of the expression' do
      r = unless not false
            1
          else
            2
          end
      r.should == 2
      r = unless !false
            3
          else
            4
          end
      r.should == 4
    end
  end
end
