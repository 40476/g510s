#!/usr/bin/env zsh

steps=360
delay=0
increment=1  # Add a configurable increment

while true; do
  for ((i=0; i<steps; i+=increment)); do  # Use increment here
    hue=$(echo "$i / $steps" | bc -l)
    rgb=($(python3 -c "import colorsys; r,g,b=[int(x*255) for x in colorsys.hsv_to_rgb($hue,1,1)]; print(r, g, b)"))
    r=${rgb[1]}
    g=${rgb[2]}
    b=${rgb[3]}
    qdbus6 org.g510s.control /org/g510s/control org.g510s.control.SetColor $r $g $b false > /dev/null
    sleep $delay
  done
done
