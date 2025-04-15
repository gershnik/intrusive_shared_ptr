# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

## Unreleased

## [1.8] - 2025-04-14

### Added
- Support for `std::format` (if available). Instances of `intrusive_shared_ptr` can be passed to `std::format`
  and will be printed like a `void *`.
- Support for `std::hash<intrusive_shared_ptr>`. 
- `get_inout_param()` method. This provides the same functionality as `std::inout_ptr` but is available for pre-C++23 compilation.

### Changed
- Performance improvements for `std::out_ptr` etc.
- C++ module is no longer using CMake library mechanism. See [README](README.md) for details on how to use module.

### Fixed
- Some internal helpers moved into `internal` namespace where they always should have been
- Internal macros are now properly cleared at the end of the header files 

## [1.7] - 2025-04-09

### Added
- Full support for `std::out_ptr` and `std::inout_ptr` when provided by the standard library.
  Older `get_output_param()` continues to work and its semantics have been aligned with `std::out_ptr`
  behavior.

### Changed
- C++ module CMake targets now include standard version suffixes. Instead of `isptrm::isptrm` you need
  to use `isptrm-20::isptrm-20` or `isptrm-23::isptrm-23` to match the version of C++ your client code 
  is compiled under. (Apparently you cannot use a module built with a different C++ version from the
  client code. Sigh.)

## [1.6] - 2025-02-11

### Changed
- Test targets are now excluded from default build
- Doctest is now included via `<doctest/doctest.h>` to make it possible to build 
  tests against an external Doctest not brought via FetchContent. 

### Fixed
- `common.h` header is now correctly installed by CMake install

## [1.5] - 2024-08-16

### Added
- C++ module support (experimental). The library can now be used as C++ module. See [README](https://github.com/gershnik/intrusive_shared_ptr/) for details.

### Fixed
- `weak_reference::single_threaded` flag is made `constexpr` rather than `static const`

## [1.4] - 2023-07-22

### Changed
- Updated CMake configuration to modernize it and allow local installation

## [1.3] - 2023-03-31

### Added
- Added single threaded mode support to `ref_counted`

### Changed
- Updated documentation

## [1.2] - 2023-03-17

### Added
- Pre-defined specialization for Python object and type pointer for use in Python extensions. See python_ptr.h header for details.


## [1.1] - 2022-06-09

### Changed
- CMake configuration modified to mark this library sources as PRIVATE rather than INTERFACE. This plays nicer with IDEs and avoid polluting library clients with its headers

## [1.0] - 2022-05-22

### Added
- First release

[1.0]: https://github.com/gershnik/intrusive_shared_ptr/releases/v1.0
[1.1]: https://github.com/gershnik/intrusive_shared_ptr/releases/v1.1
[1.2]: https://github.com/gershnik/intrusive_shared_ptr/releases/v1.2
[1.3]: https://github.com/gershnik/intrusive_shared_ptr/releases/v1.3
[1.4]: https://github.com/gershnik/intrusive_shared_ptr/releases/v1.4
[1.5]: https://github.com/gershnik/intrusive_shared_ptr/releases/v1.5
[1.6]: https://github.com/gershnik/intrusive_shared_ptr/releases/v1.6
[1.7]: https://github.com/gershnik/intrusive_shared_ptr/releases/v1.7
[1.8]: https://github.com/gershnik/intrusive_shared_ptr/releases/v1.8
