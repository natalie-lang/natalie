require 'minitest/autorun'
require 'open3'
require 'tmpdir'
require_relative '../support/nat_binary'

class RuntimeRevisionTest < Minitest::Test
  def test_description_accepts_empty_and_short_build_revisions
    Dir.mktmpdir('natalie-runtime-revision') do |dir|
      executable = File.join(dir, 'program')
      fixture = File.expand_path('../support/runtime_revision.cpp', __dir__)
      code = "require 'natalie/inline'; require #{fixture.inspect}; puts RUBY_DESCRIPTION; puts RUBY_REVISION"
      stdout, stderr, status = Open3.capture3(NAT_BINARY, '-c', executable, '-e', code)
      assert status.success?, stdout + stderr
      ['', 'abc', '0123456789abcdef'].each do |revision|
        stdout, stderr, status = Open3.capture3({ 'NAT_TEST_RUBY_REVISION' => revision }, executable)
        assert status.success?, stderr
        assert_includes stdout, "revision #{revision[0, 10]})"
        assert_equal revision, stdout.lines.last.chomp
        assert_empty stderr
      end
    end
  end
end
