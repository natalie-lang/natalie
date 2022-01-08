# Contributing to Natalie

Everyone is welcome to contribute to Natalie!

We are building a kind of Ruby implementation, following as closely as possible to what Matz's
Ruby Interpreter (MRI) does. Where this is not possible, we do the best we can.

When contributing to Natalie, make sure that the changes you make are in line with the project
direction. If you are not sure about this, open an issue first so we can discuss it.

## Code submission policy

- Use `clang-format` to automatically format C++ files. You can run `rake format`.
- Split your changes into separate, atomic commits (i.e. A commit per feature or fix, where the
  tests all pass).
- Wrap your commit messages at 72 characters (even the subject).
- Write the commit message subject line in the imperative mood, starting with a capital letter
  ("Fix Array#first", not "fixed Array#first" and not just "Array#first").
- Write your commit messages in proper English, with care and punctuation.
- Squash your commits when making revisions after a PR review.
- Check the spelling of your code, comments and commit messages.
