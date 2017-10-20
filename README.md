This is my attempt at making a code editor. Or was. It's currently under a thick layer of ice. 

The goal is to be able to move from visual studio to this.
Currently it's nowhere near that goal.

Most importantly it needs to be 100% reliable.

We need to reintroduce the lexer and parser. Maybe as it's own self contained 'plugin'
The memory handling stuff is completely shit.
I'll have to think about moving back to a basic malloc. (+ debug)
Also probably use templates for hashtables, dynamic arrays etc. Good experiment but I'm not 100% a fan. 

The features that are important:
- [x] unicode support
- [x] multiple buffers
- [x] multiple cursors
- [x] multiple views of same file 
- [x] subpixel font-rendering 
- [x] copy paste
- [x] color markdown
- [x] customization (currently hotloading DLLs)
- [ ] intellisence
- [/] search (file / folder) 
- [x] smooth scrolling
- [ ] virtual whitespace?
- [x] i3-style window manager for buffers
- [/] tree based history ie. crtl-z doesn't remove stuff only branch, (currently buggy and complicated scheme to reduce size, maybe do simple thing but add an layer of compression) also not clear what even should happen for editions in mutliple views and undo in one.
- [x] title's ie. special comments with different font size


At somepoint
- [ ] AST-search / replace
- [ ] multibuffer
- [ ] csi mode (multiple people typing, same computer)
- [ ] networked mode
- [ ] multiple files single buffer mode
- [ ] hardware rendering 

Ports:
- [x] Windows
- [ ] Mac
- [ ] Linux
