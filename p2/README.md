Name: Bidhan Mainali
V-number: V01060563

## What it does
mts simulates an automated control system for a single shared railway track.
Each train is a POSIX thread that loads, waits for permission, then crosses.
The main thread is the dispatcher and decides which train crosses next based
on priority, direction, and the anti-starvation rule (spec Section 3.2). Only
one train is on the track at a time.

## Build
    make

This produces an executable called mts.

## Run
    ./mts input.txt

Output is written to output.txt (overwritten on each run).

## Implementation status
Fully implemented and working. Tested against the sample input from the
assignment spec and the output matches exactly. Priority, opposite-direction
tie-breaking, and anti-starvation are all handled.

