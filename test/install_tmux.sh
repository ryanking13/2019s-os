#!/bin/sh

cp tmux.tar.gz /tmp
cp tmux.tar.gz /tmp
mount -o remount,exec /tmp
cd /tmp
tar -xvf tmux.tar.gz

stty cols 100

export LD_LIBRARY_PATH=/tmp/tmux/lib
alias tmux="/tmp/tmux/bin/tmux"
