#!/bin/bash
killall switcher
switcher -d &
sleep 1

switcher-ctrl -C uridecodebin video
switcher-ctrl -s video loop true
switcher-ctrl -s video uri `echo file://$1`
switcher-ctrl -s video play true
