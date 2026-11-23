# MRI Onigmo

These sources come from the Ruby repository, rather than the standalone
Onigmo repository. `upstream.json` records the exact commit and the mapping
from vendored paths to upstream paths, including generated Unicode and JIS
tables. The accompanying `BSDL`, `COPYING`, and `LEGAL` files retain upstream
license information.

Natalie builds this snapshot directly with `NOT_RUBY` and the standalone headers
in `compat/`. No configure step, Ruby checkout, or source generation is
needed to build Natalie.

To reproduce the recorded import from a local Ruby checkout:

```sh
ruby tools/vendor_onigmo.rb ../ruby
```

To update to another Ruby commit:

```sh
ruby tools/vendor_onigmo.rb ../ruby <commit>
```

The importer reads committed Git objects, applies `ext/onigmo.patch` in a
temporary directory, and updates the vendored sources and revision only
after the patch succeeds. Local changes in the Ruby checkout are ignored.
When updating, review the file mapping and the Unicode include path in
`Rakefile` if upstream changes its generated table layout.

`ext/onigmo.patch` contains Natalie's standalone adaptations: public-header
integer types, Ruby search-range and group-name behavior, compatibility
includes, and `onig_st_*` names for MRI's hash table implementation to avoid
collisions with the Ruby C extension ABI. Keep changes to imported files in
this patch so imports remain reproducible. The `compat/` headers are
maintained locally and are not overwritten by the importer.

After refreshing, run:

```sh
bundle exec rake build/onigmo/lib/libonigmo.a
bundle exec rake test
```
