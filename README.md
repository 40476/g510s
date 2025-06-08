# G510s

Graphical utility for Logitech G510 and G510s keyboards on Linux.

## Build Dependencies

* libg15-devel
* libg15render-devel
* libappindicator3-devel
* libgtk-3-dev

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
* Custom LCD configurations
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

## Special Prefixes for Shape and Graph Parameters

* `!` before **x**: Draw only the left/right outline (for rectangles/bars/graphs).
* `!` before **y**: Draw only the top/bottom outline.
* `!` before **w**: Flip the X axis (draw right-to-left for bars/graphs).
* `!` before **h**: Flip the Y axis (draw bottom-to-top for bars/graphs).
* `^` removes all newlines from a command output
**You can combine these as needed.**

---

## Complex Example by copilot cuz lazy

```plaintext
# CPU use as a line graph
GRAPH,LINE,0,4,157,35,100,// top -bn1 | grep "Cpu(s)" | awk '{print int($2 + $4)}' //

# CPU, RAM, and GPU as bars with outlines
GRAPH,BAR,!5,!5,50,6,100,// top -bn1 | grep "Cpu(s)" | awk '{print int($2 + $4)}' //
GRAPH,BAR,!60,!5,50,6,100,// free | awk '/Mem:/ {print int($3/$2*100)}' //
GRAPH,BAR,!115,!5,40,6,100,// amdgpu_top -d --json | jq ".[0].gpu_activity.GFX.value" //

# Labels under each bar
5,13,L,0,0,// printf CPU //
60,13,L,0,0,// printf RAM //
115,13,L,0,0,// printf GPU //

# Show current time and date, centered (spaces needed cuz alignment is broken rn)
78,30,C,0,1,// printf "    " && date "+%H:%M:%S %a %b %d" //
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
