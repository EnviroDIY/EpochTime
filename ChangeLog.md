# ChangeLog<!--! {#change_log} -->

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) and its stricter, better defined, brother [Common Changelog](https://common-changelog.org/).

This project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

<!-- @tableofcontents{XML:1,HTML:1} -->

<!--! @m_footernavigation -->

<!--! @if GITHUB -->

- [ChangeLog](#changelog)
  - [Unreleased](#unreleased)
  - [1.1.0](#110)
  - [1.0.0](#100)

<!--! @endif -->

***

## [Unreleased]

### Changed

### Added

### Removed

### Fixed

***

## [1.1.0]

### Changed

- Use `#ifndef` around SECONDS_IN_DAY define
- Updated TimeFormatting example

### Added

- Added two functions for filling tm objects: `fillTimeParts(epochTime in_time, tm& timeParts)` and `fillTimeParts(timestamp_t in_timestamp, int32_t in_utcOffset, epochStart in_epoch, tm& timeParts)`
- Added two functions for getting a time_t `getTimeT(epochTime in_time)` and `getTimeT(timestamp_t in_timestamp, int32_t in_utcOffset = 0, epochStart in_epoch = epochStart::unix_epoch)`
- Added one function for getting an EpochTime from a tm struct: `epochTime tmToEpochTime(tm timeParts);`
- Added a function to compare two tm structs: `sameTime(const tm& a, const tm& b)`

### Removed

- Removed DS3231 example

***

## [1.0.0]

Initial release, taking code from the ModularSensors library

***

[Unreleased]: https://github.com/EnviroDIY/EpochTime/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/EnviroDIY/EpochTime/releases/tag/v1.1.0
[1.0.0]: https://github.com/EnviroDIY/EpochTime/releases/tag/v1.0.0

<!--! @tableofcontents{HTML:1} -->

<!--! @m_footernavigation -->

<!-- cspell:words -->
