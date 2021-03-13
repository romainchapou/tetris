# A simple terminal implementation of tetris  

A C / ncurses tetris implementation inspired by the NES version that runs in
your terminal.

<img src="https://github.com/romainchapou/tetris/raw/master/screenshot.png"></img>


## Installation

Simply compile with `make` and run it with `./tetris`. You may want to move the
executable to `~/.local/bin` or something similar.


## Controls

- `left_arrow`, `h` / `right_arrow`, `l`: move to the left / right
- `down_arrow`, `j`: soft drop
- `up_arrow`, `k`, `c`: rotate the piece
- `e`, `x`: inverse rotation
- `p`: pause / resume the game
- `q`: quit the game

You can specify the level you want to start with as an argument (for example
`./tetris 7` to start at level 7).


## Notes

The rules are mostly the same as in the NES version, notably for rotations,
speed, scoring and random generation of tetriminos. For more information on
those, or if you'd like to build your own version, here are some links I found
useful:

- https://tetris.wiki/Tetris_(NES,_Nintendo)
- https://tetris.wiki/Nintendo_Rotation_System
- https://tetris.wiki/Scoring
- https://meatfighter.com/nintendotetrisai/#The_Mechanics_of_Nintendo_Tetris
