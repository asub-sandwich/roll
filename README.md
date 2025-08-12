# roll
A command-line Tabletop RPG dice roller!

# usage
    
    roll [<count>d<die>] [+|- <count>d<die> | <constant>]...
        [--no-color | -n] [--quiet | -q]
allowed dice: d2 d4 d6 d8 d10 d12 d20 d100
    
# examples
    roll d20
    roll 5d6 + 4
    roll 2d8 + d10 - 2

# features
+ REPL for more immersion I guess!
+ special message for Nat 20s or 1s if any operand is d20
+ use "quiet" mode to output only the total (for stats purposes)
+ no dependencies
+ should work on windows (untested)
+ probably doesnt work on mac

# install
    make
    sudo make install

# probability testing
    make test
    
    TRIALS=100 make test
    DICE_TYPES="4 12" make test

Default: 10,000 trials; all dice
