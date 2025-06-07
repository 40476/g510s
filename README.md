# G510s

Graphical utility for Logitech G510 and G510s keyboards on Linux.

## Build Dependencies

* libg15-devel
* libg15render-devel
* libappindicator3-devel
* libgtk-3-dev

## TODO

* Valgrind
* DBUS IPC
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
* LCD display server
* Custom LCD configurations
* libg15daemon-client compatibility

## SystemD file (assumes keyboard is always connected)

```systemd
[Unit]
Description=Launch and monitor g510s program
After=default.target
Requires=default.target

[Service]
ExecStart=/bin/bash -c 'while true; do /usr/local/bin/g510s & PID=$!; while ps -p $PID > /dev/null; do CPU_USAGE=$(top -b -n1 -p $PID | awk "/$PID/ {print \$9}"); if (( $(echo "$CPU_USAGE > 90" | bc -l) )); then kill -9 $PID; fi; sleep 5; done; done'
Restart=always
RestartSec=1

[Install]
WantedBy=default.target
```



---

## Special Prefixes for Shape and Graph Parameters

You can use an exclamation mark (`!`) before certain parameters to control outlines and axis flipping:

* `!` before **x**: Draw only the left/right outline (for rectangles/bars/graphs).
* `!` before **y**: Draw only the top/bottom outline.
* `!` before **w**: Flip the X axis (draw right-to-left for bars/graphs).
* `!` before **h**: Flip the Y axis (draw bottom-to-top for bars/graphs).

**You can combine these as needed.**

---

## Complex Example: Multi-Data Dashboard with Outlines and Axis Flips

```plaintext
# CPU, RAM, and Disk as bars with outlines and axis flips
GRAPH,BAR,!5,!5,!50,6,// top -bn1 | grep "Cpu(s)" | awk '{print int($2 + $4)}' //
GRAPH,BAR,!60,!5,50,!6,// free | awk '/Mem:/ {print int($3/$2*100)}' //
GRAPH,BAR,115,!5,!40,!6,// df / | awk 'END {print int($5)}' //

# Labels under each bar
5,13,L,0,1,// echo CPU //
60,13,L,0,1,// echo RAM //
115,13,L,0,1,// echo DISK //

# Show current time and date, centered
80,30,C,0,1// date "+%H:%M:%S %a %b %d" //

# CPU use as a line graph, flipped X axis, with outline
GRAPH,LINE,!0,25,!157,10,100,// top -bn1 | grep "Cpu(s)" | awk '{print int($2 + $4)}' //

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

## More Ideas

* Show temperature:  
  `10,20,L,0,8,// sensors | awk '/^Package id 0:/ {print int($4)}' //`
* Show custom messages:  
  `80,10,C,0,8,// printf "Hello G510s!" //`
