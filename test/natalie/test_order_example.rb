describe 'before/after' do
  before :all do
    puts '[[before all]]'
  end

  before :each do
    puts '[[before each]]'
  end

  it 'it 1' do
    puts '[[it 1]]'
  end

  it 'it 2' do
    puts '[[it 2]]'
  end

  context 'context 1' do
    before :all do
      puts '[[before all 1]]'
    end

    before :each do
      puts '[[before each 1]]'
    end

    it 'it 1' do
      puts '[[it 1]]'
    end

    it 'it 2' do
      puts '[[it 2]]'
    end

    after :each do
      puts '[[after each 1]]'
    end

    after :all do
      puts '[[after all 1]]'
    end
  end

  after :each do
    puts '[[after each]]'
  end

  after :all do
    puts '[[after all]]'
  end
end
