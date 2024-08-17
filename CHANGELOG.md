# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

## Unreleased

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
