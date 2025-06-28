# G510s

Graphical utility for Logitech G510 and G510s keyboards on Linux.

## Build Dependencies

### OpenSUSE

`sudo zypper in g510s libg15-devel libg15-1 libg15render3 libg15render-devel libappindicator3-devel`

## TODO

* Valgrind
* Save on system shutdown
* Change screens from GUI

## FEATURES

* libg15 and libusb debugging via cli switch
* Four profiles, selectable with M1, M2, M3 and MR keys
* Custom LED backlight colors per profile
* Custom G-Key script/command execution per profile
* Persistent sessions
* Executable as user (instead of root -- **DO NOT RUN AS ROOT**)
* GUI and AppIndicator for desktop integration
* Handles device hotplug events (on-device audio)
* DBUS profile and color control
* Advanced custom LCD configurations
* libg15daemon-client compatibility

## SystemD file

```systemd
[Unit]
Description=Launch and monitor g510s program
After=default.target
Requires=default.target

[Service]
ExecStart=/bin/sh -c 'while true; do if lsusb | grep -q "046d:c22d"; then /usr/local/bin/g510s & PID=$!; while ps -p $PID > /dev/null; do CPU_USAGE=$(top -b -n1 -p $PID | awk "/$PID/ {print \$9}"); if (( $(echo "$CPU_USAGE > 90" | bc -l) )); then kill -9 $PID; fi; sleep 5; done; else sleep 10; fi; done'
Restart=always
RestartSec=1

[Install]
WantedBy=default.target
```

---

## Special Prefixes

* `!` before **x**: Draw only the left/right outline (for rectangles/bars/graphs).
* `!` before **y**: Draw only the top/bottom outline.
* `!` before **w**: Flip the X axis (draw right-to-left for bars/graphs).
* `!` before **h**: Flip the Y axis (draw bottom-to-top for bars/graphs).
* `^` removes all newlines from a command output
* `%` defines a variable
* `@` calls a variable
* `@@` is just a normal `@` sign

**You can combine these as needed.**

---

## Complex Example by copilot cuz lazy

```plaintext
# Variables
%cpuval // top -bn1 | grep "Cpu(s)" | awk '{print int($2 + $4)}' //
%ramval // free | awk '/Mem:/ {print int($3/$2*100)}' //
%gpuval // amdgpu_top -d --json | jq ".[0].gpu_activity.GFX.value" //

# CPU usage graph
GRAPH,LINE,0,4,160,35,100,// printf @cpuval //

# CPU meter
5,13,L,0,0,// printf CPU //
GRAPH,BAR,!5,!5,50,6,100,// printf @cpuval //
55,13,R,0,0,^// echo -n "@cpuval%" //

# RAM usage
60,13,L,0,0,// printf RAM //
GRAPH,BAR,!60,!5,50,6,100,// printf @ramval //
110,13,R,0,0,^// echo -n "@ramval%" //

# GPU usage
115,13,L,0,0,// printf GPU //
GRAPH,BAR,!115,!5,40,6,100,// printf @gpuval //
155,13,R,0,0,^// echo -n "@gpuval%" //

# Date and time
78,30,C,0,1,// date "+%H:%M:%S %a %b %d" //
```

## Fill Modes for Shapes

For `RECT`, `ELLIPSE`, and `POLY`, the last argument (optional) controls the fill mode:

* `0` or omitted: Outline only (default)
* `1`: Filled (white)
* `2`: Filled (black)
* `3`: (POLY only) Filled and outline both black

**Examples:**

```plaintext
# Rectangle examples
RECT,5,5,30,10           # Outline rectangle at (5,5), size 30x10
RECT,40,5,30,10,1        # Filled rectangle (white)
RECT,75,5,30,10,2        # Filled rectangle (black)

# Ellipse examples
ELLIPSE,20,25,12,6       # Outline ellipse
ELLIPSE,55,25,12,6,1     # Filled ellipse (white)
ELLIPSE,90,25,12,6,2     # Filled ellipse (black)

# Polygon examples (triangle)
POLY,[(80,5);(100,20);(60,20)]           # Outline triangle
POLY,[(120,5);(140,20);(100,20)],1       # Filled triangle (white)
POLY,[(120,25);(140,40);(100,40)],2      # Filled triangle (black)
POLY,[(120,45);(140,60);(100,60)],3      # Filled and outline black (POLY only)
```

---

## Tips

* All graphing related shell commands must output a single integer value.
* For `LINE` graphs, the optional `max` parameter sets the maximum value for scaling. Use `0` for autoscale.
* Make sure your graph/shape fits on the screen: `x + w <= 160`, `y + h <= 43`.
* Use `awk`, `grep`, `cut`, etc., to extract numbers from system commands.
* You can combine as many elements as you want, up to 32 lines per script.
* Use `!` prefixes to control outlines and axis flipping as described above.

---

## Troubleshooting

* If a graph or shape is not visible, check your coordinates and sizes.
* If a command outputs nothing or an error, you'll see `[none]` or `[err]` on the display.

---

## More Stuff

* Show temperature:  
  `10,20,L,0,8,// sensors | awk '/^Package id 0:/ {print int($4)}' //`
* Show custom messages:  
  `80,10,C,0,8,// printf "Hello G510s!" //`

## sus \/

```plaintext
RECT,40,15,10,15,0
ELLIPSE,53,30,4,6,0
ELLIPSE,60,30,4,6,0
ELLIPSE,57,14,7,6,0
RECT,49,15,16,17,2
ELLIPSE,64,17,5,3,2
POLY,[(50,15);(59,15)],3
```