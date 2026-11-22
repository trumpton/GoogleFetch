# Google Fetch
Google Access API Debug Tool

GoogleFetch sends requests to the Google APIs, using the same sign-in and
request code as ContactManager (a copy of it, in googleconnection.cpp), so you
can check what Google returns for a request.

## Settings

The Google client ID and secret are read from the `[google]` section of
`googlefetch.ini`, or failing that `ContactManager.ini`.  The first of these
folders that holds one is used:

- `~/ContactManager`
- `~/AccessibleApps`
- the folder of the program

```
[google]
clientid=...
secret=...
```

If no file is found, or the values are missing, the output box says what to fix.

## Building

- Linux: `./dobuildlin.sh GoogleFetch` from the AccessibleApps folder.
- Windows: `dobuildwin.bat GoogleFetch` from the AccessibleApps folder, after
  `winshell.bat`.

On Windows the program uses the Windows TLS library, so it needs no OpenSSL
files.
