require_relative '../spec_helper'

describe 'Queue' do
  describe '#freeze' do
    ruby_version_is ''...'3.3' do
      it 'can be frozen' do
        queue = Queue.new
        -> {
          queue.freeze
        }.should_not raise_error(TypeError)
      end
    end

    ruby_version_is '3.3' do
      it 'cannot be frozen' do
        queue = Queue.new
        -> {
          queue.freeze
        }.should raise_error(TypeError, "cannot freeze #{queue}")
      end
    end
  end
end
