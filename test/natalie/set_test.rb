# frozen_string_literal: true

require_relative '../spec_helper'

describe 'Set' do
  describe 'eql?' do
    it 'should not seet two sets equal if compare_by_identity is different' do
      Set.new([1, 2]).should_not.eql?(Set.new([1, 2]).compare_by_identity)
    end
  end
end
