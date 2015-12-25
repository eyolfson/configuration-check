# Configuration Check

A tool to setup your Arch Linux system.

## Layout

This program looks for a `system` or `user` directory. The next directory
should match the hostname or `base`. The next directory should be the package
name. After that the directory should match the path of the file. System paths
map to starting at the root directory. User paths map to starting in the user's
home directory with a `.` prefixed to the path directly after the home
directory.

## License

All content in the `src` directory is distributed under the GPL-3.0.
