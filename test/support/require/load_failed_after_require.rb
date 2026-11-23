$load_after_require_runs = ($load_after_require_runs || 0) + 1
raise 'explicit load failed' if $load_after_require_runs > 1
