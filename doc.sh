#!/bin/sh

(./paragraph.sh | ./line_break) << EOF
Lorem     ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor
  incididunt ut labore et dolore magna aliqua. Vel turpis nunc eget lorem dolor  
  sed viverra. Tellus rutrum tellus pellentesque eu tincidunt tortor aliquam.
  Pellentesque habitant morbi tristique senectus et netus et malesuada fames. Vel
  eros donec ac odio tempor orci. Senectus et netus et malesuada fames ac.
  Tristique senectus et netus et. Faucibus a pellentesque sit amet porttitor eget
  dolor morbi. Mauris commodo quis imperdiet massa tincidunt nunc pulvinar.
  Consectetur adipiscing elit pellentesque habitant morbi. Pulvinar elementum
  integer enim neque. Vitae tempus quam pellentesque nec nam aliquam sem. Turpis
  in eu mi bibendum neque egestas congue quisque.

Lorem     ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor
  incididunt ut labore et dolore magna aliqua. Vel turpis nunc eget lorem dolor  
  sed viverra. Tellus rutrum tellus pellentesque eu tincidunt tortor aliquam.
  Pellentesque habitant morbi tristique senectus et netus et malesuada fames. Vel
  eros donec ac odio tempor orci. Senectus et netus et malesuada fames ac.
  Tristique senectus et netus et. Faucibus a pellentesque sit amet porttitor eget
  dolor morbi. Mauris commodo quis imperdiet massa tincidunt nunc pulvinar.
  Consectetur adipiscing elit pellentesque habitant morbi. Pulvinar elementum
  integer enim neque. Vitae tempus quam pellentesque nec nam aliquam sem. Turpis
  in eu mi bibendum neque egestas congue quisque.

Lorem     ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor
  incididunt ut labore et dolore magna aliqua. Vel turpis nunc eget lorem dolor  
  sed viverra. Tellus rutrum tellus pellentesque eu tincidunt tortor aliquam.
  Pellentesque habitant morbi tristique senectus et netus et malesuada fames. Vel
  eros donec ac odio tempor orci. Senectus et netus et malesuada fames ac.
  Tristique senectus et netus et. Faucibus a pellentesque sit amet porttitor eget
  dolor morbi. Mauris commodo quis imperdiet massa tincidunt nunc pulvinar.
  Consectetur adipiscing elit pellentesque habitant morbi. Pulvinar elementum
  integer enim neque. Vitae tempus quam pellentesque nec nam aliquam sem. Turpis
  in eu mi bibendum neque egestas congue quisque.
EOF

./line_break << EOF
FONT Regular 12
STRING "regular text"
FONT Bold 20
STRING "bold text"
EOF
