module Natalie
  class Compiler
    class CppBackend
      module PkgConfig
        PACKAGES_REQUIRING_PKG_CONFIG = %w[openssl libffi yaml-0.1].freeze

        def package_compile_flags
          PACKAGES_REQUIRING_PKG_CONFIG.flat_map do |package|
            flags_for_package(package, :inc)
          end.compact
        end

        def package_link_flags
          PACKAGES_REQUIRING_PKG_CONFIG.flat_map do |package|
            flags_for_package(package, :lib)
          end.compact
        end

        # FIXME: We should run this on any system (not just Darwin), but only when one
        # of the packages in PACKAGES_REQUIRING_PKG_CONFIG are used.
        def flags_for_package(package, type)
          return unless DARWIN

          @flags_for_package ||= {}
          existing_flags = @flags_for_package[package]
          return existing_flags[type] if existing_flags

          unless system("pkg-config --exists #{package}")
            @flags_for_package[package] = { inc: [], lib: [] }
            return []
          end

          flags = @flags_for_package[package] = {}
          unless (inc_result = `pkg-config --cflags #{package}`.strip).empty?
            flags[:inc] = inc_result.sub(/^-I/, '')
          end
          unless (lib_result = `pkg-config --libs-only-L #{package}`.strip).empty?
            flags[:lib] = lib_result.sub(/^-L/, '')
          end

          flags[type]
        end
      end
    end
  end
end
