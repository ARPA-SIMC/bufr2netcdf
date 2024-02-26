# New in version 1.7

* Discriminate messages by BUFR table version numbers, to avoid conflicts
  between messages with the same Data Descriptor Section but which expand to
  different data structures (#11, #13)
* Use doubles for latitude and longitude (#10)

# New in version 1.6

* Convert BUFR messages that use C03 reference value override codes (#7)

# New in version 1.2

* Deal with BUFR files with width/scale/string len changes
* Implemented support for C07yyy modifiers

# New in version 1.1

* Fixed getopt support
* Do not include empty dir "doc" into EXTRA_DIST
* Refined ignore lists in checks
* Added documentation with the differences with bufrx2netcdf

# New in version 1.0

* Updated tables to v1.1.5-10
* Changes needed to port on AIX
