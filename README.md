This is my attempt at making a code editor.

The goal is to be able to move from visual studio to this.
Currently it's nowhere near that goal.

Most importantly it needs to be 100% reliable.

We need to reintroduce the lexer. Maybe as it's own self contained 'plugin'
The memory handling stuff is completely shit.
I'll have to think about moving back to a basic malloc. (+ debug)

The features that are neccessary:
- [x] unicode support
- [x] multiple buffers
- [x] multiple cursors
- [x] subpixel font-rendering 
- [x] copy paste
- [ ] color markdown
- [ ] customization
- [ ] intellisence
- [ ] search (file / folder / selection)
- [ ] page up / down
- [ ] handle \r\n lineenindgs correctly and use the one that is in the file unless specified otherwise. (maybe normalize as well if it differs)

At somepoint
- [ ] csi mode  
- [ ] networked mode
- [ ] multiple files single buffer mode


Ports:
- [x] Windows
- [ ] Mac
- [ ] Linux


Known Bugs:
if given file does not exsit we fail.
\r\n renders as double line breaks. This is not ok.
