require_relative '../spec_helper'

describe 'Random' do
  it 'should extend Random::Formatter' do
    Random.should be_kind_of(Random::Formatter)
  end

  describe 'Random.random_number' do
    it "generates a random float number between 0.0 and 1.0 if argument is negative" do
      num = Random.random_number(-10)
      num.should be_kind_of(Float)
      0.0.should <= num
      num.should < 1.0
    end

    it "generates a random float number between 0.0 and 1.0 if argument is negative float" do
      num = Random.random_number(-11.1)
      num.should be_kind_of(Float)
      0.0.should <= num
      num.should < 1.0
    end
  end
end
