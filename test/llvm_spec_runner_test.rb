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
    # Cached separate compilation remains unsupported by LLVM. A C++ fallback in ruby_exe
    # would hide that restriction in the child compiler.
    run_fixture(<<~'RUBY') do |report, _, stderr, status|
      require 'tmpdir'
      describe 'runner children' do
        it 'selects the child backend' do
          dir = Dir.mktmpdir
          begin
            ruby_exe(
              'puts 1',
              options: "--build-dir=#{dir} --build-quietly",
            ).should == "1\n"
          ensure
            FileUtils.rm_rf(dir)
          end
        end
      end
    RUBY
      assert_equal 1, status.exitstatus, stderr
      assert_equal({ 'llvm_failed' => 1 }, report.fetch('summary'), JSON.pretty_generate(report))
      runs = report.fetch('results').first.fetch('runs')
      assert runs.fetch('cpp').fetch('success')
      refute runs.fetch('llvm').fetch('success')
      assert_includes runs.fetch('llvm').fetch('stderr'), '--build-dir'
      assert_equal 0, report.fetch('matching_passed_examples')
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
end
