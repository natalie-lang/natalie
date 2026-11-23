# Contributing to Natalie

Everyone is welcome to contribute to Natalie!

We are building a kind of Ruby implementation, following as closely as possible to what Matz's
Ruby Interpreter (MRI) does. Where this is not possible, we do the best we can.

When contributing to Natalie, make sure that the changes you make are in line with the project
direction. If you are not sure about this, open an issue first so we can discuss it.

## Code Formatting

- Use `clang-format` to automatically format C++ files and `syntax_tree` for Ruby files. You can run `rake format`.

### Pre-commit formatting check

Enable the repository's hook once per checkout:

```sh
git config core.hooksPath .githooks
```

The hook rejects commits when staged Ruby or C/C++ files would change under
Syntax Tree or clang-format. It checks a temporary copy of the index, including
staged formatter settings, without modifying your working tree or staged files.
Ruby exclusions in `.streerc`, bundled dependencies in `ext/`, build output, and
generated encoding tables are skipped.

The hook uses `nix develop` when Nix is installed. Otherwise, install the project's
Ruby gems and clang-format on `PATH`. Fix formatting with `bundle exec stree write
path/to/file.rb` or `clang-format -i path/to/file.cpp`, then stage the files again.
Run the check manually with `ruby tools/check_staged_format.rb` in your development
shell.

## Tests

- We prefer specs be brought over from [ruby/spec](https://github.com/ruby/spec) so we are sure
  to match MRI's behavior.
- As we are working to get a spec passing, we inevitably will need to disable certain
  parts of the spec that aren't yet passing. We prefer to use the `NATFIXME` helper to do this,
  which is [documented here](https://github.com/natalie-lang/natalie/blob/master/test/support/spec.rb#L1448).
  (We usually don't comment out failing tests.)
- If existing Ruby specs are not enough to fully define a feature, we try to make adjustments
  to ruby/spec and upstream the changes with a pull request to that project.
- As a last resort, or during certain difficult bootstrapping processes, we sometimes add
  custom tests to the `tests/` directory, especially if such tests are specific to a Natalie
  feature or quirk.

## Code submission policy

- Split your changes into separate, atomic commits (i.e. A commit per feature or fix, where the
  tests all pass).
- Wrap your commit messages at 72 characters (even the subject).
- Write the commit message subject line in the imperative mood, starting with a capital letter
  ("Fix Array#first", not "fixed Array#first" and not just "Array#first").
- Write your commit messages in proper English, with care and punctuation.
- Squash your commits when making revisions after a PR review.
- Check the spelling of your code, comments and commit messages.
