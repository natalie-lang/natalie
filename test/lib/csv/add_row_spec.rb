require_relative '../../spec_helper'
require_relative 'shared/add_row'

describe 'CSV#<<' do
  it_behaves_like :csv_add_row, :<<
end

describe 'CSV#add_row' do
  it_behaves_like :csv_add_row, :add_row
end
