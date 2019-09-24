# Unix socket plugin for stingray

### Installation :

    make
    make install

No dependencies should be required. stingray's plugin header is copied in `include` dir.

### Debug

Compile with debug flags: 

    CPPFLAGS="-DDEBUG -g" make

### Usage

Send commands as text over a Unix socket, terminated with a newline (`\n`).

This pugin provides a raw interface to set the speed axis and a `QUIT` command.

It also provide a helper with a "degree" command `d45` which will aims at a smooth rotation of the object to the desired position. Require a video representing a linear rotation of 360 frames.
