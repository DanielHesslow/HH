This is my attempt at making a code editor.

The goal is to be able to move from visual studio to this.
Currently it's nowhere near that goal.

Most importantly it needs to be 100% reliable.
now for example we insert \r before \n evey time getting \r\r\r\n after two saves.

We need to reintroduce the lexer. Maybe as it's own self contained 'plugin'
The memory handling stuff is completely shit.
I'll have to think about moving back to a basic malloc. (+ debug)

The features that are neccessary:
[x] unicode support
[x] multiple buffers
[x] multiple cursors
[x] subpixel font-rendering 
[x] copy paste
[ ] color markdown
[ ] customization
[ ] intellisence
[ ] search (file / folder / selection)
[ ] page up / down

At somepoint
[ ] csi mode  
[ ] networked mode
[ ] multiple files single buffer mode


Ports:
[x] Windows
[ ] Mac
[ ] Linux


