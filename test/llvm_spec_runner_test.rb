require 'minitest/autorun'
require 'json'
require 'open3'
require 'rbconfig'
require 'tmpdir'

class LlvmSpecRunnerTest < Minitest::Test
  ROOT = File.expand_path('..', __dir__)
  RUNNER = File.join(ROOT, 'tools/llvm_spec_runner.rb')

  def run_fixture(code, *options)
    Dir.mktmpdir('natalie-spec-runner-test') do |dir|
      file = File.join(dir, 'fixture_spec.rb')
      report = File.join(dir, 'report.json')
      File.write(file, "require #{File.join(ROOT, 'spec/spec_helper').inspect}\n#{code}")
      stdout, stderr, status =
        Open3.capture3(RbConfig.ruby, RUNNER, '--jobs=1', '--report', report, *options, file, chdir: ROOT)
      yield JSON.parse(File.read(report)), stdout, stderr, status
    end
  end

  def test_matching_passes_are_counted_once
    run_fixture("describe('runner') { it('passes') { nil.should.nil? } }") do |report, _, stderr, status|
      assert status.success?, stderr
      assert_equal({ 'passed' => 1 }, report.fetch('summary'))
      assert_equal 1, report.fetch('matching_passed_examples')
      assert_equal %w[cpp llvm], report.fetch('backends')
    end
  end

  def test_child_ruby_programs_use_the_selected_backend
    run_fixture(<<~'RUBY') do |report, _, stderr, status|
      describe 'runner children' do
        it 'selects the child backend' do
          source = ruby_exe('puts 42', options: '-d code')
          puts source
          source.empty?.should == false
        end
      end
    RUBY
      assert status.success?, stderr
      assert_equal({ 'passed' => 1 }, report.fetch('summary'), JSON.pretty_generate(report))
      runs = report.fetch('results').first.fetch('runs')
      assert_includes runs.fetch('cpp').fetch('stdout'), '#include "natalie.hpp"'
      assert_includes runs.fetch('llvm').fetch('stdout'), 'define i64 @nat_llvm_eval'
      refute_includes runs.fetch('llvm').fetch('stdout'), '#include'
      assert_equal 1, report.fetch('matching_passed_examples')
    end
  end

  def test_deadlines_are_reported_as_failures
    run_fixture('sleep 10', '--backend=llvm', '--timeout=0.01') do |report, _, _, status|
      assert_equal 1, status.exitstatus
      result = report.fetch('results').first.fetch('runs').fetch('llvm')
      assert result.fetch('timed_out')
      refute result.fetch('success')
      assert_equal 0, report.fetch('matching_passed_examples')
    end
  end

  def test_build_directory_is_used_by_parent_and_child_compilers
    Dir.mktmpdir('natalie-runner-cache') do |cache|
      run_fixture(<<~'RUBY', "--build-dir=#{cache}") do |report, _, stderr, status|
        describe 'cached runner' do
          it 'compiles its child' do
            ruby_exe('puts 42').should == "42\n"
          end
        end
      RUBY
        assert status.success?, stderr
        assert_equal({ 'passed' => 1 }, report.fetch('summary'))
        objects = Dir[File.join(cache, 'llvm', 'llvm', '*', 'executable', 'main.o')]
        assert_equal 2, objects.size
        assert Dir.exist?(File.join(cache, 'cpp'))
      end
    end
  end
end
