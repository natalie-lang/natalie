require_relative '../spec_helper'

describe 'source symbols' do
  it 'interns a literal in a method before that method is defined or called' do
    Symbol.all_symbols.any? { |symbol| symbol.to_s == 'source_symbols_method_literal_927' }.should == true

    def source_symbols_unused_method_927
      :source_symbols_method_literal_927
    end
  end

  it 'interns a literal in a block before that block is created or called' do
    Symbol.all_symbols.any? { |symbol| symbol.to_s == 'source_symbols_block_literal_927' }.should == true
    block = -> { :source_symbols_block_literal_927 }
  end

  it 'interns parameter and local names before their method is defined or called' do
    symbols = Symbol.all_symbols.map(&:to_s)
    symbols.should include('source_symbols_parameter_928')
    symbols.should include('source_symbols_local_928')

    def source_symbols_unused_method_928(source_symbols_parameter_928)
      source_symbols_local_928 = nil
    end
  end
end
