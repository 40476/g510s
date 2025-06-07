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
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "g510s.h"

void init_data() {
  // gui
  g510s_data.gui_hidden = 0;
  
  // function
  g510s_data.mkey_state = 1;
  
  // color
  g510s_data.m1.red = 255;
  g510s_data.m1.green = 255;
  g510s_data.m1.blue = 255;
  g510s_data.m2.red = 255;
  g510s_data.m2.green = 255;
  g510s_data.m2.blue = 255;
  g510s_data.m3.red = 255;
  g510s_data.m3.green = 255;
  g510s_data.m3.blue = 255;
  g510s_data.mr.red = 255;
  g510s_data.mr.green = 255;
  g510s_data.mr.blue = 255;
  
  // cmd strings
  memset(g510s_data.m1.g1, 0, sizeof(g510s_data.m1.g1));
  memset(g510s_data.m1.g2, 0, sizeof(g510s_data.m1.g2));
  memset(g510s_data.m1.g3, 0, sizeof(g510s_data.m1.g3));
  memset(g510s_data.m1.g4, 0, sizeof(g510s_data.m1.g4));
  memset(g510s_data.m1.g5, 0, sizeof(g510s_data.m1.g5));
  memset(g510s_data.m1.g6, 0, sizeof(g510s_data.m1.g6));
  memset(g510s_data.m1.g7, 0, sizeof(g510s_data.m1.g7));
  memset(g510s_data.m1.g8, 0, sizeof(g510s_data.m1.g8));
  memset(g510s_data.m1.g9, 0, sizeof(g510s_data.m1.g9));
  memset(g510s_data.m1.g10, 0, sizeof(g510s_data.m1.g10));
  memset(g510s_data.m1.g11, 0, sizeof(g510s_data.m1.g11));
  memset(g510s_data.m1.g12, 0, sizeof(g510s_data.m1.g12));
  memset(g510s_data.m1.g13, 0, sizeof(g510s_data.m1.g13));
  memset(g510s_data.m1.g14, 0, sizeof(g510s_data.m1.g14));
  memset(g510s_data.m1.g15, 0, sizeof(g510s_data.m1.g15));
  memset(g510s_data.m1.g16, 0, sizeof(g510s_data.m1.g16));
  memset(g510s_data.m1.g17, 0, sizeof(g510s_data.m1.g17));
  memset(g510s_data.m1.g18, 0, sizeof(g510s_data.m1.g18));
  
  memset(g510s_data.m2.g1, 0, sizeof(g510s_data.m2.g1));
  memset(g510s_data.m2.g2, 0, sizeof(g510s_data.m2.g2));
  memset(g510s_data.m2.g3, 0, sizeof(g510s_data.m2.g3));
  memset(g510s_data.m2.g4, 0, sizeof(g510s_data.m2.g4));
  memset(g510s_data.m2.g5, 0, sizeof(g510s_data.m2.g5));
  memset(g510s_data.m2.g6, 0, sizeof(g510s_data.m2.g6));
  memset(g510s_data.m2.g7, 0, sizeof(g510s_data.m2.g7));
  memset(g510s_data.m2.g8, 0, sizeof(g510s_data.m2.g8));
  memset(g510s_data.m2.g9, 0, sizeof(g510s_data.m2.g9));
  memset(g510s_data.m2.g10, 0, sizeof(g510s_data.m2.g10));
  memset(g510s_data.m2.g11, 0, sizeof(g510s_data.m2.g11));
  memset(g510s_data.m2.g12, 0, sizeof(g510s_data.m2.g12));
  memset(g510s_data.m2.g13, 0, sizeof(g510s_data.m2.g13));
  memset(g510s_data.m2.g14, 0, sizeof(g510s_data.m2.g14));
  memset(g510s_data.m2.g15, 0, sizeof(g510s_data.m2.g15));
  memset(g510s_data.m2.g16, 0, sizeof(g510s_data.m2.g16));
  memset(g510s_data.m2.g17, 0, sizeof(g510s_data.m2.g17));
  memset(g510s_data.m2.g18, 0, sizeof(g510s_data.m2.g18));
  
  memset(g510s_data.m3.g1, 0, sizeof(g510s_data.m3.g1));
  memset(g510s_data.m3.g2, 0, sizeof(g510s_data.m3.g2));
  memset(g510s_data.m3.g3, 0, sizeof(g510s_data.m3.g3));
  memset(g510s_data.m3.g4, 0, sizeof(g510s_data.m3.g4));
  memset(g510s_data.m3.g5, 0, sizeof(g510s_data.m3.g5));
  memset(g510s_data.m3.g6, 0, sizeof(g510s_data.m3.g6));
  memset(g510s_data.m3.g7, 0, sizeof(g510s_data.m3.g7));
  memset(g510s_data.m3.g8, 0, sizeof(g510s_data.m3.g8));
  memset(g510s_data.m3.g9, 0, sizeof(g510s_data.m3.g9));
  memset(g510s_data.m3.g10, 0, sizeof(g510s_data.m3.g10));
  memset(g510s_data.m3.g11, 0, sizeof(g510s_data.m3.g11));
  memset(g510s_data.m3.g12, 0, sizeof(g510s_data.m3.g12));
  memset(g510s_data.m3.g13, 0, sizeof(g510s_data.m3.g13));
  memset(g510s_data.m3.g14, 0, sizeof(g510s_data.m3.g14));
  memset(g510s_data.m3.g15, 0, sizeof(g510s_data.m3.g15));
  memset(g510s_data.m3.g16, 0, sizeof(g510s_data.m3.g16));
  memset(g510s_data.m3.g17, 0, sizeof(g510s_data.m3.g17));
  memset(g510s_data.m3.g18, 0, sizeof(g510s_data.m3.g18));
  
  memset(g510s_data.mr.g1, 0, sizeof(g510s_data.mr.g1));
  memset(g510s_data.mr.g2, 0, sizeof(g510s_data.mr.g2));
  memset(g510s_data.mr.g3, 0, sizeof(g510s_data.mr.g3));
  memset(g510s_data.mr.g4, 0, sizeof(g510s_data.mr.g4));
  memset(g510s_data.mr.g5, 0, sizeof(g510s_data.mr.g5));
  memset(g510s_data.mr.g6, 0, sizeof(g510s_data.mr.g6));
  memset(g510s_data.mr.g7, 0, sizeof(g510s_data.mr.g7));
  memset(g510s_data.mr.g8, 0, sizeof(g510s_data.mr.g8));
  memset(g510s_data.mr.g9, 0, sizeof(g510s_data.mr.g9));
  memset(g510s_data.mr.g10, 0, sizeof(g510s_data.mr.g10));
  memset(g510s_data.mr.g11, 0, sizeof(g510s_data.mr.g11));
  memset(g510s_data.mr.g12, 0, sizeof(g510s_data.mr.g12));
  memset(g510s_data.mr.g13, 0, sizeof(g510s_data.mr.g13));
  memset(g510s_data.mr.g14, 0, sizeof(g510s_data.mr.g14));
  memset(g510s_data.mr.g15, 0, sizeof(g510s_data.mr.g15));
  memset(g510s_data.mr.g16, 0, sizeof(g510s_data.mr.g16));
  memset(g510s_data.mr.g17, 0, sizeof(g510s_data.mr.g17));
  memset(g510s_data.mr.g18, 0, sizeof(g510s_data.mr.g18));
  
  // clock settings
  g510s_data.clock_mode = 0;
  g510s_data.show_date = 1;
}

int check_dir() {
  char home_path[255];
  char g510s_dir[] = "/.config/g510s";
  char *full_path;
  DIR *dir;

  strncpy(home_path, getenv("HOME"), sizeof(home_path));
  
  if (home_path == NULL) {
    printf("G510s: failed to find $HOME directory, using default settings\n");
    return -1;
  }

  full_path = malloc(strlen(home_path) + strlen(g510s_dir) + 1);
  strcpy(full_path, home_path);
  strcat(full_path, g510s_dir);

  if ((dir = opendir(full_path)) == NULL) {
    if (mkdir(full_path, 0777) == -1) {
      printf("G510s: failed to create directory $HOME/.config/g510s\n");
      free(full_path);
      return -1;
    }
  }

  free(full_path);

  return 0;
}

// Helper: Write config to .conf file (text)
int write_conf(const char *path) {
    FILE *file = fopen(path, "w");
    if (!file) return -1;

    // gui
    fprintf(file, "gui_hidden=%d\n", g510s_data.gui_hidden);

    // function
    fprintf(file, "mkey_state=%d\n", g510s_data.mkey_state);

    // color
    fprintf(file, "m1_red=%d\n", g510s_data.m1.red);
    fprintf(file, "m1_green=%d\n", g510s_data.m1.green);
    fprintf(file, "m1_blue=%d\n", g510s_data.m1.blue);

    fprintf(file, "m2_red=%d\n", g510s_data.m2.red);
    fprintf(file, "m2_green=%d\n", g510s_data.m2.green);
    fprintf(file, "m2_blue=%d\n", g510s_data.m2.blue);

    fprintf(file, "m3_red=%d\n", g510s_data.m3.red);
    fprintf(file, "m3_green=%d\n", g510s_data.m3.green);
    fprintf(file, "m3_blue=%d\n", g510s_data.m3.blue);

    fprintf(file, "mr_red=%d\n", g510s_data.mr.red);
    fprintf(file, "mr_green=%d\n", g510s_data.mr.green);
    fprintf(file, "mr_blue=%d\n", g510s_data.mr.blue);

    // cmd strings for all modes and G-keys
    // Write G-keys for all modes using arrays
    fprintf(file, "m1_g1=%s\n", g510s_data.m1.g1);
    fprintf(file, "m1_g2=%s\n", g510s_data.m1.g2);
    fprintf(file, "m1_g3=%s\n", g510s_data.m1.g3);
    fprintf(file, "m1_g4=%s\n", g510s_data.m1.g4);
    fprintf(file, "m1_g5=%s\n", g510s_data.m1.g5);
    fprintf(file, "m1_g6=%s\n", g510s_data.m1.g6);
    fprintf(file, "m1_g7=%s\n", g510s_data.m1.g7);
    fprintf(file, "m1_g8=%s\n", g510s_data.m1.g8);
    fprintf(file, "m1_g9=%s\n", g510s_data.m1.g9);
    fprintf(file, "m1_g10=%s\n", g510s_data.m1.g10);
    fprintf(file, "m1_g11=%s\n", g510s_data.m1.g11);
    fprintf(file, "m1_g12=%s\n", g510s_data.m1.g12);
    fprintf(file, "m1_g13=%s\n", g510s_data.m1.g13);
    fprintf(file, "m1_g14=%s\n", g510s_data.m1.g14);
    fprintf(file, "m1_g15=%s\n", g510s_data.m1.g15);
    fprintf(file, "m1_g16=%s\n", g510s_data.m1.g16);
    fprintf(file, "m1_g17=%s\n", g510s_data.m1.g17);
    fprintf(file, "m1_g18=%s\n", g510s_data.m1.g18);

    fprintf(file, "m2_g1=%s\n", g510s_data.m2.g1);
    fprintf(file, "m2_g2=%s\n", g510s_data.m2.g2);
    fprintf(file, "m2_g3=%s\n", g510s_data.m2.g3);
    fprintf(file, "m2_g4=%s\n", g510s_data.m2.g4);
    fprintf(file, "m2_g5=%s\n", g510s_data.m2.g5);
    fprintf(file, "m2_g6=%s\n", g510s_data.m2.g6);
    fprintf(file, "m2_g7=%s\n", g510s_data.m2.g7);
    fprintf(file, "m2_g8=%s\n", g510s_data.m2.g8);
    fprintf(file, "m2_g9=%s\n", g510s_data.m2.g9);
    fprintf(file, "m2_g10=%s\n", g510s_data.m2.g10);
    fprintf(file, "m2_g11=%s\n", g510s_data.m2.g11);
    fprintf(file, "m2_g12=%s\n", g510s_data.m2.g12);
    fprintf(file, "m2_g13=%s\n", g510s_data.m2.g13);
    fprintf(file, "m2_g14=%s\n", g510s_data.m2.g14);
    fprintf(file, "m2_g15=%s\n", g510s_data.m2.g15);
    fprintf(file, "m2_g16=%s\n", g510s_data.m2.g16);
    fprintf(file, "m2_g17=%s\n", g510s_data.m2.g17);
    fprintf(file, "m2_g18=%s\n", g510s_data.m2.g18);

    fprintf(file, "m3_g1=%s\n", g510s_data.m3.g1);
    fprintf(file, "m3_g2=%s\n", g510s_data.m3.g2);
    fprintf(file, "m3_g3=%s\n", g510s_data.m3.g3);
    fprintf(file, "m3_g4=%s\n", g510s_data.m3.g4);
    fprintf(file, "m3_g5=%s\n", g510s_data.m3.g5);
    fprintf(file, "m3_g6=%s\n", g510s_data.m3.g6);
    fprintf(file, "m3_g7=%s\n", g510s_data.m3.g7);
    fprintf(file, "m3_g8=%s\n", g510s_data.m3.g8);
    fprintf(file, "m3_g9=%s\n", g510s_data.m3.g9);
    fprintf(file, "m3_g10=%s\n", g510s_data.m3.g10);
    fprintf(file, "m3_g11=%s\n", g510s_data.m3.g11);
    fprintf(file, "m3_g12=%s\n", g510s_data.m3.g12);
    fprintf(file, "m3_g13=%s\n", g510s_data.m3.g13);
    fprintf(file, "m3_g14=%s\n", g510s_data.m3.g14);
    fprintf(file, "m3_g15=%s\n", g510s_data.m3.g15);
    fprintf(file, "m3_g16=%s\n", g510s_data.m3.g16);
    fprintf(file, "m3_g17=%s\n", g510s_data.m3.g17);
    fprintf(file, "m3_g18=%s\n", g510s_data.m3.g18);

    fprintf(file, "mr_g1=%s\n", g510s_data.mr.g1);
    fprintf(file, "mr_g2=%s\n", g510s_data.mr.g2);
    fprintf(file, "mr_g3=%s\n", g510s_data.mr.g3);
    fprintf(file, "mr_g4=%s\n", g510s_data.mr.g4);
    fprintf(file, "mr_g5=%s\n", g510s_data.mr.g5);
    fprintf(file, "mr_g6=%s\n", g510s_data.mr.g6);
    fprintf(file, "mr_g7=%s\n", g510s_data.mr.g7);
    fprintf(file, "mr_g8=%s\n", g510s_data.mr.g8);
    fprintf(file, "mr_g9=%s\n", g510s_data.mr.g9);
    fprintf(file, "mr_g10=%s\n", g510s_data.mr.g10);
    fprintf(file, "mr_g11=%s\n", g510s_data.mr.g11);
    fprintf(file, "mr_g12=%s\n", g510s_data.mr.g12);
    fprintf(file, "mr_g13=%s\n", g510s_data.mr.g13);
    fprintf(file, "mr_g14=%s\n", g510s_data.mr.g14);
    fprintf(file, "mr_g15=%s\n", g510s_data.mr.g15);
    fprintf(file, "mr_g16=%s\n", g510s_data.mr.g16);
    fprintf(file, "mr_g17=%s\n", g510s_data.mr.g17);
    fprintf(file, "mr_g18=%s\n", g510s_data.mr.g18);

    // clock settings
    fprintf(file, "clock_mode=%d\n", g510s_data.clock_mode);
    fprintf(file, "show_date=%d\n", g510s_data.show_date);

    fclose(file);
    return 0;
}

// Helper: Read config from .conf file (text)
int read_conf(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) return -1;

    char key[64], value[256];
    while (fgets(key, sizeof(key), file)) {
        // Skip empty lines or lines without '='
        if (key[0] == '\n' || strchr(key, '=') == NULL)
            continue;

        // Split key and value
        char *eq = strchr(key, '=');
        if (!eq) continue;
        *eq = '\0';
        char *val = eq + 1;

        // Remove trailing newline from value
        char *newline = strchr(val, '\n');
        if (newline) *newline = '\0';

        // If value is empty, skip
        if (val[0] == '\0')
            continue;

        // gui
        if (strcmp(key, "gui_hidden") == 0) g510s_data.gui_hidden = atoi(val);

        // function
        else if (strcmp(key, "mkey_state") == 0) g510s_data.mkey_state = atoi(val);

        // color
        else if (strcmp(key, "m1_red") == 0) g510s_data.m1.red = atoi(val);
        else if (strcmp(key, "m1_green") == 0) g510s_data.m1.green = atoi(val);
        else if (strcmp(key, "m1_blue") == 0) g510s_data.m1.blue = atoi(val);

        else if (strcmp(key, "m2_red") == 0) g510s_data.m2.red = atoi(val);
        else if (strcmp(key, "m2_green") == 0) g510s_data.m2.green = atoi(val);
        else if (strcmp(key, "m2_blue") == 0) g510s_data.m2.blue = atoi(val);

        else if (strcmp(key, "m3_red") == 0) g510s_data.m3.red = atoi(val);
        else if (strcmp(key, "m3_green") == 0) g510s_data.m3.green = atoi(val);
        else if (strcmp(key, "m3_blue") == 0) g510s_data.m3.blue = atoi(val);

        else if (strcmp(key, "mr_red") == 0) g510s_data.mr.red = atoi(val);
        else if (strcmp(key, "mr_green") == 0) g510s_data.mr.green = atoi(val);
        else if (strcmp(key, "mr_blue") == 0) g510s_data.mr.blue = atoi(val);

        // G-keys for all modes
        else {
            int found = 0;
            for (int i = 1; i <= 18; ++i) {
                char gkey[16];
                snprintf(gkey, sizeof(gkey), "m1_g%d", i);
                if (strcmp(key, gkey) == 0) {
                    char *fields[] = {g510s_data.m1.g1, g510s_data.m1.g2, g510s_data.m1.g3, g510s_data.m1.g4, g510s_data.m1.g5, g510s_data.m1.g6, g510s_data.m1.g7, g510s_data.m1.g8, g510s_data.m1.g9, g510s_data.m1.g10, g510s_data.m1.g11, g510s_data.m1.g12, g510s_data.m1.g13, g510s_data.m1.g14, g510s_data.m1.g15, g510s_data.m1.g16, g510s_data.m1.g17, g510s_data.m1.g18};
                    strncpy(fields[i-1], val, sizeof(g510s_data.m1.g1)-1);
                    fields[i-1][sizeof(g510s_data.m1.g1)-1] = '\0';
                    found = 1;
                    break;
                }
                snprintf(gkey, sizeof(gkey), "m2_g%d", i);
                if (strcmp(key, gkey) == 0) {
                    char *fields[] = {g510s_data.m2.g1, g510s_data.m2.g2, g510s_data.m2.g3, g510s_data.m2.g4, g510s_data.m2.g5, g510s_data.m2.g6, g510s_data.m2.g7, g510s_data.m2.g8, g510s_data.m2.g9, g510s_data.m2.g10, g510s_data.m2.g11, g510s_data.m2.g12, g510s_data.m2.g13, g510s_data.m2.g14, g510s_data.m2.g15, g510s_data.m2.g16, g510s_data.m2.g17, g510s_data.m2.g18};
                    strncpy(fields[i-1], val, sizeof(g510s_data.m2.g1)-1);
                    fields[i-1][sizeof(g510s_data.m2.g1)-1] = '\0';
                    found = 1;
                    break;
                }
                snprintf(gkey, sizeof(gkey), "m3_g%d", i);
                if (strcmp(key, gkey) == 0) {
                    char *fields[] = {g510s_data.m3.g1, g510s_data.m3.g2, g510s_data.m3.g3, g510s_data.m3.g4, g510s_data.m3.g5, g510s_data.m3.g6, g510s_data.m3.g7, g510s_data.m3.g8, g510s_data.m3.g9, g510s_data.m3.g10, g510s_data.m3.g11, g510s_data.m3.g12, g510s_data.m3.g13, g510s_data.m3.g14, g510s_data.m3.g15, g510s_data.m3.g16, g510s_data.m3.g17, g510s_data.m3.g18};
                    strncpy(fields[i-1], val, sizeof(g510s_data.m3.g1)-1);
                    fields[i-1][sizeof(g510s_data.m3.g1)-1] = '\0';
                    found = 1;
                    break;
                }
                snprintf(gkey, sizeof(gkey), "mr_g%d", i);
                if (strcmp(key, gkey) == 0) {
                    char *fields[] = {g510s_data.mr.g1, g510s_data.mr.g2, g510s_data.mr.g3, g510s_data.mr.g4, g510s_data.mr.g5, g510s_data.mr.g6, g510s_data.mr.g7, g510s_data.mr.g8, g510s_data.mr.g9, g510s_data.mr.g10, g510s_data.mr.g11, g510s_data.mr.g12, g510s_data.mr.g13, g510s_data.mr.g14, g510s_data.mr.g15, g510s_data.mr.g16, g510s_data.mr.g17, g510s_data.mr.g18};
                    strncpy(fields[i-1], val, sizeof(g510s_data.mr.g1)-1);
                    fields[i-1][sizeof(g510s_data.mr.g1)-1] = '\0';
                    found = 1;
                    break;
                }
            }
            if (!found) {
                if (strcmp(key, "clock_mode") == 0) g510s_data.clock_mode = atoi(val);
                else if (strcmp(key, "show_date") == 0) g510s_data.show_date = atoi(val);
            }
        }
    }
    fclose(file);
    return 0;
}

// Export .dat to .conf if needed
void export_dat_to_conf(const char *dat_path, const char *conf_path) {
    FILE *file = fopen(dat_path, "rb");
    if (!file) return;
    fread(&g510s_data, sizeof(g510s_data), 1, file);
    fclose(file);
    write_conf(conf_path);
}

// Replace load_config and save_config
int load_config() {
    char home_path[255];
    char conf_name[] = "/.config/g510s/g510s.conf";
    char dat_name[] = "/.config/g510s/g510s.dat";
    char *conf_path, *dat_path;

    strncpy(home_path, getenv("HOME"), sizeof(home_path));
    if (home_path == NULL) {
        printf("G510s: failed to find $HOME directory, using default settings\n");
        return -1;
    }

    conf_path = malloc(strlen(home_path) + strlen(conf_name) + 1);
    dat_path = malloc(strlen(home_path) + strlen(dat_name) + 1);
    strcpy(conf_path, home_path);
    strcat(conf_path, conf_name);
    strcpy(dat_path, home_path);
    strcat(dat_path, dat_name);

    // If .conf does not exist but .dat does, export
    FILE *conf_file = fopen(conf_path, "r");
    if (!conf_file) {
        FILE *dat_file = fopen(dat_path, "rb");
        if (dat_file) {
            fclose(dat_file);
            export_dat_to_conf(dat_path, conf_path);
            printf("G510s: Exported legacy .dat to .conf\n");
        }
    } else {
        fclose(conf_file);
    }

    int result = read_conf(conf_path);

    free(conf_path);
    free(dat_path);

    return result;
}

int save_config() {
    char home_path[255];
    char conf_name[] = "/.config/g510s/g510s.conf";
    char *conf_path;

    strncpy(home_path, getenv("HOME"), sizeof(home_path));
    if (home_path == NULL) {
        printf("G510s: failed to find $HOME directory, skipping save\n");
        return -1;
    }

    conf_path = malloc(strlen(home_path) + strlen(conf_name) + 1);
    strcpy(conf_path, home_path);
    strcat(conf_path, conf_name);

    int result = write_conf(conf_path);

    free(conf_path);

    return result;
}
