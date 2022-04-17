## What is it?

This is a simulation game called Game Of Life designed by Jhon Conway. More information of it you can find here - https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life.

My implemetation uses terminal as a drawing destination. Therefore you can change display "resolution" by changing terminal's size and its font's size.

## What you can do with it?
Well, there are to options to run it:
1. You can either run with no arguments or with just one argument - frequency. This randomly generates some number of cells alive and this number is        defined by frequncy number. Frequency number is an integer between 1 and 10. It defines how much living cells will be generated on the screen. If frequency number is not provided, it is set to 5 - middle value.
   ```
   ./gof
   ./gof 2
   ```
2. The second option supposes providing a file with an initial world setup. The syntax is:
   ```
   ./gof *file* *y* *x*
   ```
   Where y and x are the coordinates of top left corner of the desired position on the screen. For example:
   ```
   ./gof gosper_gun.gof 10 10
   ```
Also in either options you can append run command with a "-m" argument which enables manual mode. I'll explain modes later.

## Files
Instead of randomly filling your world with living cells, you can provide an initial generation (or setup). It becomes interesting if you want to examine behaviour of some figures which become alive from time to time. Some of these figures are provided within this repo. But you also can create your own. The .gof file format is as simple as it could be. 1 means living cell and 0 means dead cell (of just empty space). See examples for more details.

## Modes and keys
There are 3 modes:
1. Automatic mode. This mode is enabled automatically if you run without "-m" argument. Cells just die and arise infinitely.
2. Pause mode. This mode is enabled by pressing "p" key. It does what its name says. Just pauses producing new generations until you press "p" again.
3. Manual mode. As was said previously you can enter this mode by appending your run command with "-m" argument. But you cat also enable it by pressing "m" key when you're in automatic mode. In this mode you jump to next generation by pressing "n" key. This actually helps observing processes step-by-step. To exit this mode you need to press "x" key.

Also when you're in automatic mode you can change the speed of producing new generation by pressing "f" key to make it faster ans "s" key to make it slower respectively.

And the last but not least, in any mode you can quit program by pressing "q" key.

## Logging

I implemented simple logging system. All errors and infos will be written to file "log". File can be changed via the source code.

## Preview

![](https://i.ibb.co/TmMnDZ4/Screenshot-from-2022-04-17-20-57-45.png)

## Dependecies
All of the drawing to terminal is made vie ncurses, so before compiling you should run:
```
sudo apt-get install libncurses5-dev
```

## Compiling
To compile just link to ncurses. On GCC (I haven't checked other compilers):
```
gcc gof.c -o gof -lncurses
```
