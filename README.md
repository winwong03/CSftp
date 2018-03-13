# CSftp
CPSC 317 A3

A command line FTP client written in C

To run: make run

The FTP client supports the following commands as well as a variety of error messages:
user [username] Provides the FTP server a username.
pw [password] Provides the FTP server a password.
dir Opens a new data TCP connection and lists the contents of the current directory.
cd [directoryName] Navigates to the given directoryName.
get [fileName] Opens a new data TCP connection and retrieves the file named fileName.
quit Closes the current FTP session.
