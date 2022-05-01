#!/bin/bash

function check {
    ./bzip3 -e -b 900 corpus/$1 corpus/$1.bz3
    ./bzip3 -d corpus/$1.bz3 corpus/$1.orig
    wc -c corpus/$1.bz3
    rm -f corpus/$1.bz3 corpus/$1.orig
}

check book1 &
check enwik8 &
check lisp.mb &

wait
