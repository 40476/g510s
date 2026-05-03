/*
 *  This file is part of g510s.
 *
 *  g510s is  free  software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published by
 *  the  Free Software Foundation; either version 3 of the License, or (at your
 *  option)  any later version.
 *
 *  g510s is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with g510s; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1335  USA
 *
 *  Copyright © 2015 John Augustine
 *  Copyright © 2025 usr_40476
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <pthread.h>
#include <libg15.h>
#include <libg15render.h>
#include <time.h>

#include "g510s.h"


static int uinp_fd = -1;

int init_uinput() {
  int i = 0;
  struct uinput_user_dev uinp;
  static const char *uinput_device_fn[] = { "/dev/uinput", "/dev/input/uinput", "/dev/misc/uinput", 0 };
  
  while (uinput_device_fn[i] && (uinp_fd = open(uinput_device_fn[i], O_RDWR)) < 0) {
    ++i;
  }
  
  if (uinp_fd < 0) {
    printf("G510s: failed to open uinput\n");
    return -1;
  }
  
  memset(&uinp, 0, sizeof(uinp));
  strncpy(uinp.name, "G15 Extra Keys", UINPUT_MAX_NAME_SIZE);
  
  uinp.id.version = 4;
  uinp.id.bustype = BUS_USB;
  
  ioctl(uinp_fd, UI_SET_EVBIT, EV_KEY);
  
  for (i = 0; i < 256; ++i) {
    ioctl(uinp_fd, UI_SET_KEYBIT, i);
  }
  
  write(uinp_fd, &uinp, sizeof(uinp));
  
  if (ioctl(uinp_fd, UI_DEV_CREATE)) {
    printf("G510s: failed to create uinput device\n");
    return -1;
  }
  
  return 0;
}

void exit_uinput() {
  ioctl(uinp_fd, UI_DEV_DESTROY);
  close(uinp_fd);
}

void uinput_keydown(unsigned char code) {
  struct input_event event;
  
  memset(&event, 0, sizeof(event));
  event.type = EV_KEY;
  event.code = code;
  event.value = 1;
  write(uinp_fd, &event, sizeof(event));
  
  memset(&event, 0, sizeof(event));
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  write(uinp_fd, &event, sizeof(event));
}

void uinput_keyup(unsigned char code) {
  struct input_event event;
  
  memset(&event, 0, sizeof(event));
  event.type = EV_KEY;
  event.code = code;
  event.value = 0;
  write(uinp_fd, &event, sizeof(event));
  
  memset(&event, 0, sizeof(event));
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  write(uinp_fd, &event, sizeof(event));
}

// Helper: get G-key command based on current mode and key index
static const char* get_gkey_cmd(int gkey) {
    switch(g510s_data.mkey_state) {
        case 1:
            switch(gkey) {
                case 1: return g510s_data.m1.g1;
                case 2: return g510s_data.m1.g2;
                case 3: return g510s_data.m1.g3;
                case 4: return g510s_data.m1.g4;
                case 5: return g510s_data.m1.g5;
                case 6: return g510s_data.m1.g6;
                case 7: return g510s_data.m1.g7;
                case 8: return g510s_data.m1.g8;
                case 9: return g510s_data.m1.g9;
                case 10: return g510s_data.m1.g10;
                case 11: return g510s_data.m1.g11;
                case 12: return g510s_data.m1.g12;
                case 13: return g510s_data.m1.g13;
                case 14: return g510s_data.m1.g14;
                case 15: return g510s_data.m1.g15;
                case 16: return g510s_data.m1.g16;
                case 17: return g510s_data.m1.g17;
                case 18: return g510s_data.m1.g18;
            }
        case 2:
            switch(gkey) {
                case 1: return g510s_data.m2.g1;
                case 2: return g510s_data.m2.g2;
                case 3: return g510s_data.m2.g3;
                case 4: return g510s_data.m2.g4;
                case 5: return g510s_data.m2.g5;
                case 6: return g510s_data.m2.g6;
                case 7: return g510s_data.m2.g7;
                case 8: return g510s_data.m2.g8;
                case 9: return g510s_data.m2.g9;
                case 10: return g510s_data.m2.g10;
                case 11: return g510s_data.m2.g11;
                case 12: return g510s_data.m2.g12;
                case 13: return g510s_data.m2.g13;
                case 14: return g510s_data.m2.g14;
                case 15: return g510s_data.m2.g15;
                case 16: return g510s_data.m2.g16;
                case 17: return g510s_data.m2.g17;
                case 18: return g510s_data.m2.g18;
            }
        case 3:
            switch(gkey) {
                case 1: return g510s_data.m3.g1;
                case 2: return g510s_data.m3.g2;
                case 3: return g510s_data.m3.g3;
                case 4: return g510s_data.m3.g4;
                case 5: return g510s_data.m3.g5;
                case 6: return g510s_data.m3.g6;
                case 7: return g510s_data.m3.g7;
                case 8: return g510s_data.m3.g8;
                case 9: return g510s_data.m3.g9;
                case 10: return g510s_data.m3.g10;
                case 11: return g510s_data.m3.g11;
                case 12: return g510s_data.m3.g12;
                case 13: return g510s_data.m3.g13;
                case 14: return g510s_data.m3.g14;
                case 15: return g510s_data.m3.g15;
                case 16: return g510s_data.m3.g16;
                case 17: return g510s_data.m3.g17;
                case 18: return g510s_data.m3.g18;
            }
        case 4:
            switch(gkey) {
                case 1: return g510s_data.mr.g1;
                case 2: return g510s_data.mr.g2;
                case 3: return g510s_data.mr.g3;
                case 4: return g510s_data.mr.g4;
                case 5: return g510s_data.mr.g5;
                case 6: return g510s_data.mr.g6;
                case 7: return g510s_data.mr.g7;
                case 8: return g510s_data.mr.g8;
                case 9: return g510s_data.mr.g9;
                case 10: return g510s_data.mr.g10;
                case 11: return g510s_data.mr.g11;
                case 12: return g510s_data.mr.g12;
                case 13: return g510s_data.mr.g13;
                case 14: return g510s_data.mr.g14;
                case 15: return g510s_data.mr.g15;
                case 16: return g510s_data.mr.g16;
                case 17: return g510s_data.mr.g17;
                case 18: return g510s_data.mr.g18;
            }
    }
    return NULL;
}

void process_keys(lcdlist_t *displaylist, unsigned int key, unsigned int key_state) {


  /*
   * Handle Extended Multimedia Keys
   */
  
  if ((key & G15_KEY_PLAY) && (key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_PLAY) && !(key_state & G15_EXTENDED_KEY)) {
    uinput_keydown(KEY_PLAYPAUSE);
  }
  if (!(key & G15_KEY_PLAY) && !(key & G15_EXTENDED_KEY)
            && (key_state & G15_KEY_PLAY) && (key_state & G15_EXTENDED_KEY)) {
    uinput_keyup(KEY_PLAYPAUSE);
  }
  if ((key & G15_KEY_STOP) && (key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_STOP) && !(key_state & G15_EXTENDED_KEY)) {
    uinput_keydown(KEY_STOPCD);
  }
  if (!(key & G15_KEY_STOP) && !(key & G15_EXTENDED_KEY)
            && (key_state & G15_KEY_STOP) && (key_state & G15_EXTENDED_KEY)) {
    uinput_keyup(KEY_STOPCD);
  }
  if ((key & G15_KEY_NEXT) && (key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_NEXT) && !(key_state & G15_EXTENDED_KEY)) {
    uinput_keydown(KEY_NEXTSONG);
  }
  if (!(key & G15_KEY_NEXT) && !(key & G15_EXTENDED_KEY)
            && (key_state & G15_KEY_NEXT) && (key_state & G15_EXTENDED_KEY)) {
    uinput_keyup(KEY_NEXTSONG);
  }
  if ((key & G15_KEY_PREV) && (key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_PREV) && !(key_state & G15_EXTENDED_KEY)) {
    uinput_keydown(KEY_PREVIOUSSONG);
  }
  if (!(key & G15_KEY_PREV) && !(key & G15_EXTENDED_KEY)
            && (key_state & G15_KEY_PREV) && (key_state & G15_EXTENDED_KEY)) {
    uinput_keyup(KEY_PREVIOUSSONG);
  }
  if ((key & G15_KEY_MUTE) && (key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_MUTE) && !(key_state & G15_EXTENDED_KEY)) {
    uinput_keydown(KEY_MUTE);
  }
  if (!(key & G15_KEY_MUTE) && !(key & G15_EXTENDED_KEY)
            && (key_state & G15_KEY_MUTE) && (key_state & G15_EXTENDED_KEY)) {
    uinput_keyup(KEY_MUTE);
  }
  if ((key & G15_KEY_RAISE_VOLUME) && (key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_RAISE_VOLUME) && !(key_state & G15_EXTENDED_KEY)) {
    uinput_keydown(KEY_VOLUMEUP);
  }
  if (!(key & G15_KEY_RAISE_VOLUME) && !(key & G15_EXTENDED_KEY)
            && (key_state & G15_KEY_RAISE_VOLUME) && (key_state & G15_EXTENDED_KEY)) {
    uinput_keyup(KEY_VOLUMEUP);
  }
  if ((key & G15_KEY_LOWER_VOLUME) && (key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_LOWER_VOLUME) && !(key_state & G15_EXTENDED_KEY)) {
    uinput_keydown(KEY_VOLUMEDOWN);
  }
  if (!(key & G15_KEY_LOWER_VOLUME) && !(key & G15_EXTENDED_KEY)
            && (key_state & G15_KEY_LOWER_VOLUME) && (key_state & G15_EXTENDED_KEY)) {
    uinput_keyup(KEY_VOLUMEDOWN);
  }
  
  /*
   * Handle Light Key
   */
  
  if ((key & G15_KEY_LIGHT) && !(key_state & G15_KEY_LIGHT)) {
    // color wont set if backlight is off
    // to workaround we set the color whenever we toggle the backlight
    set_color();
  }
  if (!(key & G15_KEY_LIGHT) && (key_state & G15_KEY_LIGHT)) {
    
  }
  
  /*
   * Handle M-Keys
   */
  
  if ((key & G15_KEY_M1) && !(key_state & G15_KEY_M1)) {
    set_mkey_state(1);
  }
  if (!(key & G15_KEY_M1) && (key_state & G15_KEY_M1)) {
    
  }
  if ((key & G15_KEY_M2) && !(key_state & G15_KEY_M2)) {
    set_mkey_state(2);
  }
  if (!(key & G15_KEY_M2) && (key_state & G15_KEY_M2)) {
    
  }
  if ((key & G15_KEY_M3) && !(key_state & G15_KEY_M3)) {
    set_mkey_state(3);
  }
  if (!(key & G15_KEY_M3) && (key_state & G15_KEY_M3)) {
    
  }
  if ((key & G15_KEY_MR) && !(key_state & G15_KEY_MR)) {
    set_mkey_state(4);
  }
  if (!(key & G15_KEY_MR) && (key_state & G15_KEY_MR)) {
    
  }
  
  /*
   * Handle L-Keys
   */
  
  // L1 key - short press toggles terminal keyboard mode, long press toggles terminal
  if ((key & G15_KEY_L1) && !(key_state & G15_KEY_L1)) {
    // Key pressed - record time
    gettimeofday(&l1_press_time, NULL);
    l1_pressed = 1;
  }
  if (!(key & G15_KEY_L1) && (key_state & G15_KEY_L1)) {
    // Key released - check if short or long press
    if (l1_pressed) {
      struct timeval now;
      gettimeofday(&now, NULL);
      
      long press_duration = (now.tv_sec - l1_press_time.tv_sec) * 1000 +
                          (now.tv_usec - l1_press_time.tv_usec) / 1000;
      
      if (press_duration >= L1_LONG_PRESS_MS) {
        // Long press - toggle terminal on/off
        if (terminal_mode) {
          terminal_mode = 0;
          close_terminal();
          printf("G510s: Terminal closed (long press L1)\n");
        } else {
          terminal_mode = 1;
          init_terminal();
          printf("G510s: Terminal opened (long press L1)\n");
        }
        displaylist->current->lcd->ident = 0; // Force redraw
      } else {
        // Short press - toggle terminal keyboard mode
        terminal_keyboard_mode = !terminal_keyboard_mode;
        printf("G510s: Terminal keyboard mode %s\n", 
               terminal_keyboard_mode ? "ON" : "OFF");
      }
      l1_pressed = 0;
    }
  }
  if ((key & G15_KEY_L2) && !(key_state & G15_KEY_L2)) {
    if (displaylist->tail == displaylist->current) {
      if (!g510s_data.clock_mode) {
        g510s_data.clock_mode = 1;
      } else {
        g510s_data.clock_mode = 0;
      }
      displaylist->current->lcd->ident = 0;
    } else {
      send_keystate(displaylist->current->lcd, key);
    }
  }
  if (!(key & G15_KEY_L2) && (key_state & G15_KEY_L2)) {
    if (displaylist->tail != displaylist->current) {
      send_keystate(displaylist->current->lcd, key);
    }
  }
  if ((key & G15_KEY_L3) && !(key_state & G15_KEY_L3)) {
    if (displaylist->tail == displaylist->current) {
      if (!g510s_data.show_date) {
        g510s_data.show_date = 1;
      } else {
        g510s_data.show_date = 0;
      }
      displaylist->current->lcd->ident = 0;
    } else {
      send_keystate(displaylist->current->lcd, key);
    }
  }
  if (!(key & G15_KEY_L3) && (key_state & G15_KEY_L3)) {
    if (displaylist->tail != displaylist->current) {
      send_keystate(displaylist->current->lcd, key);
    }
  }
  if ((key & G15_KEY_L4) && !(key_state & G15_KEY_L4)) {
    if (displaylist->tail != displaylist->current) {
      send_keystate(displaylist->current->lcd, key);
    }
  }
  if (!(key & G15_KEY_L4) && (key_state & G15_KEY_L4)) {
    if (displaylist->tail != displaylist->current) {
      send_keystate(displaylist->current->lcd, key);
    }
  }
  if ((key & G15_KEY_L5) && !(key_state & G15_KEY_L5)) {
    if (displaylist->tail != displaylist->current) {
      send_keystate(displaylist->current->lcd, key);
    }
  }
  if (!(key & G15_KEY_L5) && (key_state & G15_KEY_L5)) {
    if (displaylist->tail != displaylist->current) {
      send_keystate(displaylist->current->lcd, key);
    }
  }
  
  /*
   * Handle Extended G-Keys
   */
  
  if ((key & G15_KEY_G1) && !(key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_G1) && !(key_state & G15_EXTENDED_KEY)) {
    run_gkey_cmd(1);
  }
  
  if ((key & G15_KEY_G2) && !(key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_G2) && !(key_state & G15_EXTENDED_KEY)) {
    run_gkey_cmd(2);
  }

  if ((key & G15_KEY_G3) && !(key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_G3) && !(key_state & G15_EXTENDED_KEY)) {
    run_gkey_cmd(3);
  }

  if ((key & G15_KEY_G4) && !(key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_G4) && !(key_state & G15_EXTENDED_KEY)) {
    run_gkey_cmd(4);
  }
  if ((key & G15_KEY_G5) && !(key & G15_EXTENDED_KEY) 
            && !(key_state & G15_KEY_G5) && !(key_state & G15_EXTENDED_KEY)) {
    run_gkey_cmd(5);
  }
  if ((key & G15_KEY_G6) && !(key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_G6) && !(key_state & G15_EXTENDED_KEY)) {
    run_gkey_cmd(6);
  }
  if (!(key & G15_KEY_G6) && !(key & G15_EXTENDED_KEY)
            && (key_state & G15_KEY_G6) && !(key_state & G15_EXTENDED_KEY)) {
    
  }
  if ((key & G15_KEY_G7) && !(key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_G7) && !(key_state & G15_EXTENDED_KEY)) {
    run_gkey_cmd(7);
  }

  if ((key & G15_KEY_G8) && !(key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_G8) && !(key_state & G15_EXTENDED_KEY)) {
    run_gkey_cmd(8);
  }
  if ((key & G15_KEY_G9) && !(key & G15_EXTENDED_KEY)
            && !(key_state & G15_KEY_G9) && !(key_state & G15_EXTENDED_KEY)) {
    run_gkey_cmd(9);
  }
  
  /*
   * Handle Non-Extended G-Keys
   */
  
  if ((key & G15_KEY_G10) && !(key_state & G15_KEY_G10)) {
    run_gkey_cmd(10);
  }
  if ((key & G15_KEY_G11) && !(key_state & G15_KEY_G11)) {
    run_gkey_cmd(11);
  }
  if ((key & G15_KEY_G12) && !(key_state & G15_KEY_G12)) {
    run_gkey_cmd(12);
  }
  if ((key & G15_KEY_G13) && !(key_state & G15_KEY_G13)) {
    run_gkey_cmd(13);
  }
  if ((key & G15_KEY_G14) && !(key_state & G15_KEY_G14)) {
    run_gkey_cmd(14);
  }
  if ((key & G15_KEY_G15) && !(key_state & G15_KEY_G15)) {
    run_gkey_cmd(15);
  }
  if ((key & G15_KEY_G16) && !(key_state & G15_KEY_G16)) {
    run_gkey_cmd(16);
  }
  if ((key & G15_KEY_G17) && !(key_state & G15_KEY_G17)) {
    run_gkey_cmd(17);
  }
  if ((key & G15_KEY_G18) && !(key_state & G15_KEY_G18)) {
    run_gkey_cmd(18);
  }
}