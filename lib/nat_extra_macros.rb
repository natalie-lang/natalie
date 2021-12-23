# This script fills in macros that only exist inside the ruby compiler

# Marks a require that natalie should skip but MRI shouldn't
alias nat_ignore_require require
alias nat_ignore_require_relative require_relative