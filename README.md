# roll
A command-line Tabletop RPG dice roller!

# usage
    roll <count>d<die> [+|- <count>d<die> | <constant>]... [--no-color]
allowed dice: d2 d4 d6 d8 d10 d12 d20 d100
    
# examples
    roll d20
    roll 5d6 + 4
    roll 2d8 + d10 - 2

# features
+ special message for Nat 20s or 1s if any operand is d20
+ no dependencies, fast build, 19K in size
+ should work on windows (untested)
+ probably doesnt work on mac

# install
    make
    sudo make install
