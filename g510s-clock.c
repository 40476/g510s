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
#include <time.h>
#include <pthread.h>
#include <libg15.h>
#include <libg15render.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h> // For isalnum
#include <pty.h>
#include <utmp.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "g510s.h"

// Define terminal variables (declared as extern in g510s.h)
extern int terminal_mode;
extern char terminal_cmd[1024];

int terminal_fd = -1;
pid_t terminal_pid = 0;
char terminal_buffer[10240];
int terminal_buf_start = 0;
int terminal_buf_len = 0;
int terminal_cursor_row = 0;
int terminal_cursor_col = 0;
pthread_mutex_t terminal_mutex = PTHREAD_MUTEX_INITIALIZER;

int terminal_keyboard_mode = 0;

struct timeval l1_press_time;
int l1_pressed = 0;

#define MAX_SCRIPT_LINES 32
#define MAX_LINE_LEN 256
#define MAX_CMD_OUTPUT 256
#define MAX_SCRIPT_VARS 32
#define MAX_VAR_NAME 32
#define MAX_VAR_VALUE 256

// Terminal mode settings
#define TERMINAL_FONT_SIZE 0  // Smallest font (3px wide)
#define TERMINAL_CHAR_WIDTH 3  // Width of smallest font in pixels
#define TERMINAL_CHAR_HEIGHT 5 // Height of smallest font in pixels
#define DISPLAY_WIDTH 160      // G510s LCD width in pixels
#define DISPLAY_HEIGHT 43      // G510s LCD height in pixels
#define TERMINAL_COLS (DISPLAY_WIDTH / TERMINAL_CHAR_WIDTH)
#define TERMINAL_ROWS (DISPLAY_HEIGHT / TERMINAL_CHAR_HEIGHT)

typedef struct {
    char name[MAX_VAR_NAME];
    char value[MAX_VAR_VALUE];
} script_var_t;


// Simple hash function to uniquely identify a graph by its command string
static unsigned int hash_cmd(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; 
    return hash;
}

typedef struct {
    unsigned int id;           // Hash of the command
    int values[256];
    int pos;
    int filled;
    int last_update;           // To detect if a graph is no longer in the script
} graph_buffer_t;

static graph_buffer_t g_buffers[MAX_SCRIPT_LINES];
static int total_buffers = 0;


static void trim(char *str) {
    char *end;
    while(*str == ' ' || *str == '\t') str++;
    end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\t' || *end == '\n')) *end-- = 0;
}

static void exec_cmd(const char *cmd, char *output, size_t outlen) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        strncpy(output, "[err]", outlen);
        output[outlen-1] = 0;
        return;
    }
    if (fgets(output, outlen, fp) == NULL) {
        strncpy(output, "[none]", outlen);
        output[outlen-1] = 0;
    }
    pclose(fp);
    trim(output);
}

static int parse_int_flag(const char **str, int *flag) {
    *flag = 0;
    while (**str == '!') {
        *flag = 1;
        (*str)++;
    }
    return atoi(*str);
}

static int find_var(script_var_t *vars, int var_count, const char *name) {
    for (int i = 0; i < var_count; ++i) {
        if (strcmp(vars[i].name, name) == 0)
            return i;
    }
    return -1;
}

static void substitute_vars(char *line, script_var_t *vars, int var_count) {
    char buf[MAX_LINE_LEN * 2] = {0};
    char *src = line, *dst = buf;
    while (*src) {
        if (*src == '@' && *(src+1) == '@') {
            // @@ -> @
            *dst++ = '@';
            src += 2;
        } else if (*src == '@' && isalpha(*(src+1))) {
            src++;
            char varname[MAX_VAR_NAME] = {0};
            int vi = 0;
            while (*src && (isalnum(*src) || *src == '_') && vi < MAX_VAR_NAME-1) {
                varname[vi++] = *src++;
            }
            varname[vi] = 0;
            int idx = find_var(vars, var_count, varname);
            if (idx >= 0) {
                int vlen = strlen(vars[idx].value);
                if ((dst - buf) + vlen < (int)sizeof(buf) - 1) {
                    strcpy(dst, vars[idx].value);
                    dst += vlen;
                }
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
    strncpy(line, buf, MAX_LINE_LEN-1);
    line[MAX_LINE_LEN-1] = 0;
}

static int render_scripted_display(g15canvas *canvas, const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) return 0;

    char line[MAX_LINE_LEN];
    char raw_line_copy[MAX_LINE_LEN]; // Store original line for hashing
    int rendered = 0;

    // --- Variable storage ---
    script_var_t vars[MAX_SCRIPT_VARS];
    int var_count = 0;

    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (line[0] == 0 || line[0] == '#') continue;

        // --- 1. Save the RAW line before substitution for stable Hashing ---
        strncpy(raw_line_copy, line, MAX_LINE_LEN - 1);
        raw_line_copy[MAX_LINE_LEN - 1] = 0;

        // --- Variable definition ---
        if (line[0] == '%') {
            char *p = line + 1;
            char varname[MAX_VAR_NAME] = {0};
            int vi = 0;
            while (*p && (isalnum(*p) || *p == '_') && vi < MAX_VAR_NAME-1) {
                varname[vi++] = *p++;
            }
            varname[vi] = 0;
            while (*p == ' ' || *p == '\t') p++;
            char *cmd_start = strstr(p, "//");
            if (!*varname || !cmd_start) continue;
            cmd_start += 2;
            char *cmd_end = strstr(cmd_start, "//");
            if (!cmd_end) continue;
            *cmd_end = 0;
            char output[MAX_VAR_VALUE] = {0};
            exec_cmd(cmd_start, output, sizeof(output));
            size_t outlen = strlen(output);
            while (outlen > 0 && (output[outlen - 1] == '\n' || output[outlen - 1] == '\r')) {
                output[--outlen] = '\0';
            }
            if (var_count < MAX_SCRIPT_VARS) {
                strncpy(vars[var_count].name, varname, MAX_VAR_NAME-1);
                strncpy(vars[var_count].value, output, MAX_VAR_VALUE-1);
                var_count++;
            }
            continue;
        }

        // --- Variable substitution ---
        substitute_vars(line, vars, var_count);

        if (line[0] == 0 || line[0] == '#') continue;

        // Rectangle: RECT,x,y,w,h[,fillmode] (filled or outline, fill controlled by fillmode)
        int fill_shape = 0, black_fill = 0, fill_mode = 0, outline_black = 0;
        const char *shape_line = line;
        if (strncmp(shape_line, "RECT,", 5) == 0) {
            const char *p = shape_line + 5;
            char *tok;
            int x, y, w, h;
            fill_mode = 0;
            outline_black = 0;

            tok = strtok((char*)p, ",");
            if (!tok) continue;
            x = atoi(tok);

            tok = strtok(NULL, ",");
            if (!tok) continue;
            y = atoi(tok);

            tok = strtok(NULL, ",");
            if (!tok) continue;
            w = atoi(tok);

            tok = strtok(NULL, ",");
            if (!tok) continue;
            h = atoi(tok);

            tok = strtok(NULL, ",");
            if (tok) fill_mode = atoi(tok); // Optional fill mode

            if (fill_mode == 1) { fill_shape = 1; black_fill = 0; outline_black = 0; }
            else if (fill_mode == 2) { fill_shape = 1; black_fill = 1; outline_black = 0; }
            else if (fill_mode == 3) { fill_shape = 1; black_fill = 1; outline_black = 1; }
            else { fill_shape = 0; black_fill = 0; outline_black = 0; }

            int color = black_fill ? 0 : 1;
            if (fill_shape) {
                for (int dy = 0; dy < h; ++dy)
                    for (int dx = 0; dx < w; ++dx)
                        g15r_setPixel(canvas, x + dx, y + dy, color);
            }
            // Draw outline (always, but color depends on fill_mode)
            int outline_color = outline_black ? 0 : 1;
            for (int dx = 0; dx < w; ++dx) {
                g15r_setPixel(canvas, x + dx, y, outline_color);
                g15r_setPixel(canvas, x + dx, y + h - 1, outline_color);
            }
            for (int dy = 0; dy < h; ++dy) {
                g15r_setPixel(canvas, x, y + dy, outline_color);
                g15r_setPixel(canvas, x + w - 1, y + dy, outline_color);
            }
            rendered++;
            continue;
        }

        // Ellipse: ELLIPSE,x,y,rx,ry[,fillmode] (filled or outline, fill controlled by fillmode)
        fill_shape = 0; black_fill = 0; fill_mode = 0; outline_black = 0;
        shape_line = line;
        if (strncmp(shape_line, "ELLIPSE,", 8) == 0) {
            const char *p = shape_line + 8;
            char *tok;
            int x0, y0, rx, ry;
            fill_mode = 0;
            outline_black = 0;

            tok = strtok((char*)p, ",");
            if (!tok) continue;
            x0 = atoi(tok);

            tok = strtok(NULL, ",");
            if (!tok) continue;
            y0 = atoi(tok);

            tok = strtok(NULL, ",");
            if (!tok) continue;
            rx = atoi(tok);

            tok = strtok(NULL, ",");
            if (!tok) continue;
            ry = atoi(tok);

            tok = strtok(NULL, ",");
            if (tok) fill_mode = atoi(tok);

            if (fill_mode == 1) { fill_shape = 1; black_fill = 0; outline_black = 0; }
            else if (fill_mode == 2) { fill_shape = 1; black_fill = 1; outline_black = 0; }
            else if (fill_mode == 3) { fill_shape = 1; black_fill = 1; outline_black = 1; }
            else { fill_shape = 0; black_fill = 0; outline_black = 0; }

            int color = black_fill ? 0 : 1;
            if (fill_shape) {
                for (int y = -ry; y <= ry; ++y)
                    for (int x = -rx; x <= rx; ++x)
                        if ((x * x) * (ry * ry) + (y * y) * (rx * rx) <= (rx * rx) * (ry * ry))
                            g15r_setPixel(canvas, x0 + x, y0 + y, color);
            }
            // Draw outline (always, but color depends on fill_mode)
            int outline_color = outline_black ? 0 : 1;
            int a2 = rx * rx, b2 = ry * ry;
            int x = 0, y = ry;
            int dx = 2 * b2 * x, dy = 2 * a2 * y;
            int err = b2 - a2 * ry + a2 / 4;
            while (dx < dy) {
                g15r_setPixel(canvas, x0 + x, y0 + y, outline_color);
                g15r_setPixel(canvas, x0 - x, y0 + y, outline_color);
                g15r_setPixel(canvas, x0 + x, y0 - y, outline_color);
                g15r_setPixel(canvas, x0 - x, y0 - y, outline_color);
                if (err < 0) {
                    x++;
                    dx += 2 * b2;
                    err += dx + b2;
                } else {
                    x++; y--;
                    dx += 2 * b2;
                    dy -= 2 * a2;
                    err += dx - dy + b2;
                }
            }
            err = b2 * (x + 0.5) * (x + 0.5) + a2 * (y - 1) * (y - 1) - a2 * b2;
            while (y >= 0) {
                g15r_setPixel(canvas, x0 + x, y0 + y, outline_color);
                g15r_setPixel(canvas, x0 - x, y0 + y, outline_color);
                g15r_setPixel(canvas, x0 + x, y0 - y, outline_color);
                g15r_setPixel(canvas, x0 - x, y0 - y, outline_color);
                if (err > 0) {
                    y--;
                    dy -= 2 * a2;
                    err += a2 - dy;
                } else {
                    y--; x++;
                    dx += 2 * b2;
                    dy -= 2 * a2;
                    err += dx - dy + a2;
                }
            }
            rendered++;
            continue;
        }

        // Polygon: POLY,[x1,y1;x2,y2;...;xn,yn][,fillmode] (filled or outline, fill controlled by fillmode)
        fill_shape = 0; black_fill = 0; fill_mode = 0;
        shape_line = line;
        if (strncmp(shape_line, "POLY,[", 6) == 0) {
            // Example: POLY,[(10,10);(20,20);(30,10);(25,5)],fillmode
            const char *p = shape_line + 5;
            int points[64][2]; // max 64 points
            int n_points = 0;
            const char *s = strchr(p, '[');
            if (s) s++;
            while (s && *s && *s != ']') {
                int x = 0, y = 0;
                // Skip whitespace and separators
                while (*s && (*s == ' ' || *s == ';' || *s == ',')) s++;
                if (*s == '(') {
                    s++;
                    x = atoi(s);
                    s = strchr(s, ',');
                    if (!s) break;
                    s++;
                    y = atoi(s);
                    s = strchr(s, ')');
                    if (!s) break;
                    s++;
                    points[n_points][0] = x;
                    points[n_points][1] = y;
                    n_points++;
                    if (n_points >= 64) break;
                } else {
                    break;
                }
            }
            // Check for fillmode after ]
            fill_mode = 0;
            const char *after_bracket = strchr(p, ']');
            if (after_bracket && *(after_bracket+1) == ',') fill_mode = atoi(after_bracket+2);

            if (fill_mode == 1) { fill_shape = 1; black_fill = 0; }
            else if (fill_mode == 2) { fill_shape = 1; black_fill = 1; }
            else if (fill_mode == 3) { fill_shape = 1; black_fill = 1; } // for 3, fill and outline black
            else { fill_shape = 0; black_fill = 0; }

            int color = black_fill ? 0 : 1;
            int outline_color = (fill_mode == 3) ? 0 : 1;
            if (fill_shape) {
                // Simple polygon fill (scanline, not optimal for complex polygons)
                int min_y = points[0][1], max_y = points[0][1];
                for (int i = 1; i < n_points; ++i) {
                    if (points[i][1] < min_y) min_y = points[i][1];
                    if (points[i][1] > max_y) max_y = points[i][1];
                }
                for (int y = min_y; y <= max_y; ++y) {
                    int nodes = 0;
                    int node_x[64];
                    int j = n_points - 1;
                    for (int i = 0; i < n_points; ++i) {
                        if ((points[i][1] < y && points[j][1] >= y) ||
                            (points[j][1] < y && points[i][1] >= y)) {
                            node_x[nodes++] = points[i][0] + (y - points[i][1]) *
                                (points[j][0] - points[i][0]) / (points[j][1] - points[i][1]);
                        }
                        j = i;
                    }
                    // Sort the nodes
                    for (int i = 0; i < nodes - 1; ++i)
                        for (int k = i + 1; k < nodes; ++k)
                            if (node_x[i] > node_x[k]) {
                                int tmp = node_x[i];
                                node_x[i] = node_x[k];
                                node_x[k] = tmp;
                            }
                    for (int i = 0; i < nodes; i += 2)
                        if (i + 1 < nodes)
                            for (int x = node_x[i]; x <= node_x[i + 1]; ++x)
                                g15r_setPixel(canvas, x, y, color);
                }
            }
            // Always draw outline, color depends on fill_mode
            if (n_points == 2) {
                // Draw a single line if only two points
                g15r_drawLine(canvas, points[0][0], points[0][1], points[1][0], points[1][1], outline_color);
            } else {
                for (int i = 0; i < n_points; ++i) {
                    int x1 = points[i][0];
                    int y1 = points[i][1];
                    int x2 = points[(i + 1) % n_points][0];
                    int y2 = points[(i + 1) % n_points][1];
                    g15r_drawLine(canvas, x1, y1, x2, y2, outline_color);
                }
            }
            rendered++;
            continue;
        }

        // Graph/bar/pie/line: GRAPH,TYPE,x,y,w,h,// shell-cmd // (outline/axis swap and axis flip support)
        if (strncmp(line, "GRAPH,", 6) == 0) {
            char type[8] = {0};
            char *cmd_start = strstr(line, "//");
            if (!cmd_start) continue;

            // Parse fields with outline/invert/flip support
            char *fields = line + 6;
            char *tok;
            int x, y, w, h;
            int outline_x = 0, outline_y = 0, flip_x = 0, flip_y = 0;

            tok = strtok(fields, ",");
            if (!tok) continue;
            const char *ptok = tok;
            sscanf(ptok, "%7[^,]", type);

            tok = strtok(NULL, ",");
            if (!tok) continue;
            ptok = tok;
            x = parse_int_flag(&ptok, &outline_x);

            tok = strtok(NULL, ",");
            if (!tok) continue;
            ptok = tok;
            y = parse_int_flag(&ptok, &outline_y);

            tok = strtok(NULL, ",");
            if (!tok) continue;
            ptok = tok;
            w = parse_int_flag(&ptok, &flip_x);

            tok = strtok(NULL, ",");
            if (!tok) continue;
            ptok = tok;
            h = parse_int_flag(&ptok, &flip_y);

            char *cmd_end = strstr(cmd_start + 2, "//");
            if (cmd_end) {
                *cmd_end = 0;
                char *cmd = cmd_start + 2;
                char output[MAX_CMD_OUTPUT] = {0};
                exec_cmd(cmd, output, sizeof(output));
                int value = atoi(output);
                
                // Clamp to 0-100 only for BAR and PIE graphs
                int percent = value;
                if (strcasecmp(type, "BAR") == 0 || strcasecmp(type, "PIE") == 0) {
                    if (percent < 0) percent = 0;
                    if (percent > 100) percent = 100;
                }

                // Flip X axis if ! before w
                int x0 = x, y0 = y, w0 = w, h0 = h;
                if (flip_x) x0 = x + w - 1;
                if (flip_y) y0 = y + h - 1;

                if (strcasecmp(type, "BAR") == 0) {
                    int fill_w = (w * percent) / 100;
                    // Draw outline sides if !x, top/bottom if !y
                    if (outline_x) {
                        for (int dy = 0; dy < h; ++dy) {
                            g15r_setPixel(canvas, x, y + dy, 1);
                            g15r_setPixel(canvas, x + w - 1, y + dy, 1);
                        }
                    }
                    if (outline_y) {
                        for (int dx = 0; dx < w; ++dx) {
                            g15r_setPixel(canvas, x + dx, y, 1);
                            g15r_setPixel(canvas, x + dx, y + h - 1, 1);
                        }
                    }
                    // Fill only inside the outline (never on border pixels)
                    int fill_x0 = x + (outline_x ? 1 : 0);
                    int fill_x1 = x + fill_w - 1;
                    int fill_y0 = y + (outline_y ? 1 : 0);
                    int fill_y1 = y + h - 1 - (outline_y ? 1 : 0);

                    // Flip X
                    if (flip_x) {
                        fill_x0 = x + w - fill_w;
                        fill_x1 = x + w - 1 - (outline_x ? 1 : 0);
                    }
                    // Flip Y (for vertical bars, not standard, but for completeness)
                    if (flip_y) {
                        fill_y0 = y + h - 1 - (outline_y ? 1 : 0);
                        fill_y1 = y + h - fill_w;
                    }

                    if (fill_x1 >= fill_x0 && fill_y1 >= fill_y0) {
                        for (int dy = fill_y0; dy <= fill_y1; ++dy)
                            for (int dx = fill_x0; dx <= fill_x1 && dx < x + w - (outline_x ? 1 : 0); ++dx)
                                g15r_setPixel(canvas, dx, dy, 1);
                    }
                } else if (strcasecmp(type, "PIE") == 0) {
                    int cx = x, cy = y, r = w;
                    double angle = (percent / 100.0) * 2 * 3.14159265;
                    if (flip_x || flip_y) angle = 2 * 3.14159265 - angle;
                    if (outline_x || outline_y) {
                        if (outline_x) {
                            int px = (int)(cx + r * cos(angle));
                            int py = (int)(cy + r * sin(angle));
                            int dx = abs(px - cx), sx = cx < px ? 1 : -1;
                            int dy = -abs(py - cy), sy = cy < py ? 1 : -1;
                            int err = dx + dy, e2;
                            int x1 = cx, y1 = cy;
                            while (1) {
                                g15r_setPixel(canvas, x1, y1, 1);
                                if (x1 == px && y1 == py) break;
                                e2 = 2 * err;
                                if (e2 >= dy) { err += dy; x1 += sx; }
                                if (e2 <= dx) { err += dx; y1 += sy; }
                            }
                        }
                        if (outline_y) {
                            for (int deg = 0; deg <= (int)(percent * 3.6); ++deg) {
                                double rad = deg * 3.14159265 / 180.0;
                                if (flip_x || flip_y) rad = 2 * 3.14159265 - rad;
                                int px = (int)(cx + r * cos(rad));
                                int py = (int)(cy + r * sin(rad));
                                g15r_setPixel(canvas, px, py, 1);
                            }
                        }
                    } else {
                        for (int py = -r; py <= r; ++py) {
                            for (int px = -r; px <= r; ++px) {
                                double dist = sqrt(px * px + py * py);
                                double theta = atan2(py, px);
                                if (theta < 0) theta += 2 * 3.14159265;
                                if (flip_x || flip_y) theta = 2 * 3.14159265 - theta;
                                if (dist <= r && theta <= angle)
                                    g15r_setPixel(canvas, cx + px, cy + py, 1);
                            }
                        }
                    }
                } else if (strcasecmp(type, "LINE") == 0) {
                    // 1. Identify the buffer using the raw command string (before @ substitution)
                    unsigned int id = hash_cmd(raw_line_copy);
                    int buf_idx = -1;

                    for (int i = 0; i < total_buffers; i++) {
                        if (g_buffers[i].id == id) { buf_idx = i; break; }
                    }

                    if (buf_idx == -1 && total_buffers < MAX_SCRIPT_LINES) {
                        buf_idx = total_buffers++;
                        g_buffers[buf_idx].id = id;
                        g_buffers[buf_idx].pos = 0;
                        g_buffers[buf_idx].filled = 0;
                        memset(g_buffers[buf_idx].values, 0, sizeof(int) * 256);
                    }

                    if (buf_idx != -1) {
                        graph_buffer_t *gb = &g_buffers[buf_idx];
                        int cur_w = (w > 255) ? 255 : (w < 2 ? 2 : w);
                        int cur_h = (h < 2) ? 2 : h;

                        // Parse the optional max_val parameter (the value after h)
                        int max_val = 0;
                        tok = strtok(NULL, ",");
                        if (tok) max_val = atoi(tok);

                        // 2. Add current data point
                        gb->values[gb->pos] = percent;
                        gb->pos = (gb->pos + 1) % cur_w;
                        if (gb->pos == 0) gb->filled = 1;

                        // 3. Scaling Logic (0 = Auto, else = Fixed)
                        int current_min, current_max;
                        int limit = gb->filled ? cur_w : gb->pos;

                        if (max_val == 0) { 
                            // AUTO-SCALE MODE: Find range from history
                            current_min = gb->values[0];
                            current_max = gb->values[0];
                            for (int i = 1; i < limit; i++) {
                                if (gb->values[i] < current_min) current_min = gb->values[i];
                                if (gb->values[i] > current_max) current_max = gb->values[i];
                            }
                        } else {
                            // FIXED MODE: Scale 0 to max_val
                            current_min = 0;
                            current_max = max_val;
                        }

                        int range = (current_max <= current_min) ? 1 : (current_max - current_min);

                        // 4. Draw the lines
                        for (int i = 0; i < limit - 1; ++i) {
                            // Map sequence: Oldest to Newest
                            int p1 = (gb->filled) ? (gb->pos + i) % cur_w : i;
                            int p2 = (gb->filled) ? (gb->pos + i + 1) % cur_w : i + 1;

                            int v1 = gb->values[p1];
                            int v2 = gb->values[p2];

                            // Map values to pixel heights
                            int scaled1 = (v1 - current_min) * (cur_h - 1) / range;
                            int scaled2 = (v2 - current_min) * (cur_h - 1) / range;

                            // Handle Y-axis Flip (!)
                            int y1, y2;
                            if (flip_y) {
                                // Flipped: High values at bottom
                                y1 = y + scaled1;
                                y2 = y + scaled2;
                            } else {
                                // Standard: High values at top
                                y1 = y + cur_h - 1 - scaled1;
                                y2 = y + cur_h - 1 - scaled2;
                            }

                            // Handle X-axis Flip (!)
                            int x1 = x + (flip_x ? (limit - 1 - i) : i);
                            int x2 = x + (flip_x ? (limit - 2 - i) : i + 1);

                            // Clamping
                            if (y1 < y) y1 = y; if (y1 >= y + cur_h) y1 = y + cur_h - 1;
                            if (y2 < y) y2 = y; if (y2 >= y + cur_h) y2 = y + cur_h - 1;

                            g15r_drawLine(canvas, x1, y1, x2, y2, 1);
                        }
                    }
                }
                rendered++;
                continue;
            }
        }

        // Text line: x,y,align,angle,size,// cmd //
        int tx=0, ty=0, angle=0, size=10;
        char align_c = 'L';
        char *cmd_start = strstr(line, "//");
        if (!cmd_start) continue;
        char *cmd_end = strstr(cmd_start+2, "//");
        if (!cmd_end) continue;
        *cmd_end = 0;
        char *cmd = cmd_start+2;

        // --- Begin: Remove all newlines from command if ^ before first // ---
        int remove_newlines = 0;
        if (cmd_start > line && *(cmd_start-1) == '^') {
            remove_newlines = 1;
            *(cmd_start-1) = '\0'; // Remove the ^ from the line
        }
        // --- End: Remove all newlines from command if ^ before first // ---

        if (sscanf(line, "%d,%d,%c,%d,%d", &tx, &ty, &align_c, &angle, &size) == 5) {
            char output[MAX_CMD_OUTPUT] = {0};
            exec_cmd(cmd, output, sizeof(output));
            // Remove trailing newlines from output
            size_t outlen = strlen(output);
            while (outlen > 0 && (output[outlen - 1] == '\n' || output[outlen - 1] == '\r')) {
                output[--outlen] = '\0';
            }
            // Remove all newlines if requested
            if (remove_newlines) {
                char *src = output, *dst = output;
                while (*src) {
                    if (*src != '\n' && *src != '\r') {
                        *dst++ = *src;
                    }
                    src++;
                }
                *dst = '\0';
            }
            int text_len = strlen(output);

            // Font width logic
            int char_width;
            switch (size) {
                case 0: char_width = 3; break;
                case 1: char_width = 4; break;
                case 2: char_width = 7; break;
                default: char_width = 5; break; // fallback for other sizes
            }
            int spacing = 1;
            int total_width = text_len * (char_width + spacing) - spacing;
            int px = tx;
            if (align_c == 'C' || align_c == 'c') {
                px = tx - total_width / 2;
            } else if (align_c == 'R' || align_c == 'r') {
                px = tx - total_width;
            }
            g15r_renderString(canvas, (unsigned char *)output, 0, size, px, ty);
            rendered++;
        }
        if (rendered >= MAX_SCRIPT_LINES) break;
    }
    fclose(f);
    return rendered;
}

// Terminal emulator functions

// Ensure necessary fcntl constants are defined
#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 04000
#endif

void init_terminal() {
    pthread_mutex_lock(&terminal_mutex);
    
    if (terminal_fd >= 0) {
        pthread_mutex_unlock(&terminal_mutex);
        return; // Already initialized
    }

    // Set up PTY
    int master_fd;
    pid_t pid = forkpty(&master_fd, NULL, NULL, NULL);
    
    if (pid < 0) {
        printf("G510s: failed to forkpty\n");
        pthread_mutex_unlock(&terminal_mutex);
        return;
    }
    
    if (pid == 0) {
        // Child process - run shell
        setenv("TERM", "dumb", 1);  // Use "dumb" terminal to minimize escape sequences
        setenv("ROWS", "8", 1);
        setenv("COLS", "53", 1);
        setenv("PS1", "$ ", 1);  // Simple prompt
        setenv("PS2", "", 1);
        
        // Run the configured terminal command, or use $SHELL, or fall back to /bin/sh
        if (terminal_cmd[0]) {
            execl("/bin/sh", "sh", "-c", terminal_cmd, (char *)NULL);
        } else {
            char *shell = getenv("SHELL");
            if (shell && shell[0]) {
                // Try to avoid reading startup files for bash
                if (strstr(shell, "bash") != NULL) {
                    execl(shell, "bash", "--norc", "--noprofile", (char *)NULL);
                } else {
                    execl(shell, shell, (char *)NULL);
                }
            } else {
                execl("/bin/sh", "sh", (char *)NULL);
            }
        }
        _exit(1);
    }
    
    // Parent process
    terminal_fd = master_fd;
    terminal_pid = pid;
    terminal_buf_start = 0;
    terminal_buf_len = 0;
    terminal_cursor_row = 0;
    terminal_cursor_col = 0;
    
    // Clear the terminal buffer
    memset(terminal_buffer, 0, sizeof(terminal_buffer));
    
    // Set non-blocking
    int flags = fcntl(terminal_fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(terminal_fd, F_SETFL, flags | O_NONBLOCK);
    }
    
    printf("G510s: Terminal started with PID %d, FD %d\n", pid, master_fd);
    pthread_mutex_unlock(&terminal_mutex);
}

void close_terminal() {
    pthread_mutex_lock(&terminal_mutex);
    
    if (terminal_fd >= 0) {
        close(terminal_fd);
        if (terminal_pid > 0) {
            kill(terminal_pid, SIGTERM);
            terminal_pid = 0;
        }
        terminal_fd = -1;
        terminal_buf_len = 0;
        terminal_buf_start = 0;
    }
    
    pthread_mutex_unlock(&terminal_mutex);
}

static void read_terminal_output() {
    if (terminal_fd < 0) return;
    
    pthread_mutex_lock(&terminal_mutex);
    
    char buf[1024];
    ssize_t n;
    
    while ((n = read(terminal_fd, buf, sizeof(buf) - 1)) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            // Handle circular buffer
            int pos = (terminal_buf_start + terminal_buf_len) % sizeof(terminal_buffer);
            terminal_buffer[pos] = buf[i];
            if (terminal_buf_len < (int)sizeof(terminal_buffer)) {
                terminal_buf_len++;
            } else {
                terminal_buf_start = (terminal_buf_start + 1) % sizeof(terminal_buffer);
            }
        }
    }
    
    pthread_mutex_unlock(&terminal_mutex);
}

static void render_terminal(g15canvas *canvas) {
    pthread_mutex_lock(&terminal_mutex);
    
    if (terminal_fd < 0) {
        g15r_renderString(canvas, (unsigned char *)"Terminal closed", 0, TERMINAL_FONT_SIZE, 0, 0);
        pthread_mutex_unlock(&terminal_mutex);
        return;
    }
    
    // Read any new output
    pthread_mutex_unlock(&terminal_mutex);
    read_terminal_output();
    pthread_mutex_lock(&terminal_mutex);
    
    // Parse buffer into lines for display (filter ANSI escape sequences)
    char lines[TERMINAL_ROWS][TERMINAL_COLS + 1];
    for (int i = 0; i < TERMINAL_ROWS; i++) {
        memset(lines[i], 0, TERMINAL_COLS + 1);
    }
    
    int line_num = 0;
    int col_num = 0;
    int buf_pos = terminal_buf_start;
    int esc_state = 0;  // 0=normal, 1=got ESC, 2=got CSI [ , 3=got OSC ]
    // esc_state 2: skip until final byte (0x40-0x7e)
    
    for (int i = 0; i < terminal_buf_len && line_num < TERMINAL_ROWS; i++) {
        unsigned char c = (unsigned char)terminal_buffer[buf_pos];
        buf_pos = (buf_pos + 1) % sizeof(terminal_buffer);
        
        // Handle ANSI escape sequences
        if (esc_state == 1) {
            if (c == '[') {
                esc_state = 2;  // CSI sequence
                continue;
            } else if (c == ']') {
                esc_state = 3;  // OSC sequence
                continue;
            } else if (c == '(' || c == ')') {
                // Designate character set - skip next char
                esc_state = 4;
                continue;
            } else {
                // Not a recognized escape sequence, treat as normal
                esc_state = 0;
            }
        }
        
        if (esc_state == 2) {
            // CSI sequence: skip until we get a final byte (0x40-0x7e)
            // Parameter bytes: 0x30-0x3f, Intermediate: 0x20-0x2f
            if (c >= 0x40 && c <= 0x7e) {
                esc_state = 0;  // End of CSI
            }
            continue;
        }
        
        if (esc_state == 3) {
            // OSC mode - skip until BEL (0x07) or ST (ESC \)
            if (c == 0x07) {
                esc_state = 0;
            } else if (c == 0x1b) {
                // Possibly start of ST (ESC \)
                esc_state = 5;  // Expect backslash next
            }
            continue;
        }
        
        if (esc_state == 4) {
            // Skip one char after ESC ( or ESC )
            esc_state = 0;
            continue;
        }
        
        if (esc_state == 5) {
            if (c == '\\') {
                esc_state = 0;
            } else {
                // Not ST, maybe something else, reset
                esc_state = 0;
            }
            continue;
        }
        
        if (c == 0x1b) {
            // Start of escape sequence
            esc_state = 1;
            continue;
        }
        
        if (c == '\n' || c == '\r') {
            line_num++;
            col_num = 0;
            if (c == '\n') {
                // Skip possible \r after \n
                if (i + 1 < terminal_buf_len) {
                    char next = terminal_buffer[buf_pos];
                    if (next == '\r') {
                        buf_pos = (buf_pos + 1) % sizeof(terminal_buffer);
                        i++;
                    }
                }
            }
        } else if (c == '\t') {
            // Tab - move to next multiple of 4
            int spaces = 4 - (col_num % 4);
            for (int s = 0; s < spaces && col_num < TERMINAL_COLS; s++) {
                lines[line_num][col_num++] = ' ';
            }
        } else if (c >= 32 && c < 127) {
            // Printable character
            if (col_num < TERMINAL_COLS) {
                lines[line_num][col_num++] = c;
            }
        }
        // Ignore other control characters for simplicity
    }
    
    // Display the last TERMINAL_ROWS lines
    int start_line = (line_num >= TERMINAL_ROWS) ? line_num - TERMINAL_ROWS + 1 : 0;
    int display_row = 0;
    
    for (int i = start_line; i <= line_num && display_row < TERMINAL_ROWS; i++) {
        if (lines[i][0] || display_row == 0) { // Show empty lines at start for prompt
            g15r_renderString(canvas, (unsigned char *)lines[i], 0, TERMINAL_FONT_SIZE, 0, display_row * TERMINAL_CHAR_HEIGHT);
            display_row++;
        }
    }
    
    pthread_mutex_unlock(&terminal_mutex);
}

// Send input to terminal
void terminal_send_input(const char *input, size_t len) {
    if (terminal_fd >= 0) {
        write(terminal_fd, input, len);
    }
}

void digital_clock(lcd_t *lcd) {
    time_t currtime = time(NULL);

    // Redraw every 1 second
    if (lcd->ident != currtime) {
        g15canvas *canvas = (g15canvas *)malloc(sizeof(g15canvas));
        if (canvas == NULL) {
            printf("G510s: failed to create display canvas\n");
            return;
        }

        memset(canvas->buffer, 0, G15_BUFFER_LEN);
        canvas->mode_cache = 0;
        canvas->mode_reverse = 0;
        canvas->mode_xor = 0;

        // Terminal mode: run command and display output
        if (terminal_mode) {
            render_terminal(canvas);
        } else {
            // Try to render from script file
            char user[64] = {0};
            FILE *fp = popen("whoami", "r");
            if (fp && fgets(user, sizeof(user), fp)) {
                size_t len = strlen(user);
                if (len > 0 && (user[len - 1] == '\n' || user[len - 1] == '\r')) user[len - 1] = '\0';
            }
            if (fp) pclose(fp);
            char script_path[256];
            snprintf(script_path, sizeof(script_path), "/home/%s/.config/g510s/display.txt", user);
            int rendered = render_scripted_display(canvas, script_path);

            if (!rendered) {
                char hour_buf[3];
                char min_buf[3];
                memset(hour_buf, 0, 3);
                memset(min_buf, 0, 3);

                // 12 hour format
                if (!g510s_data.clock_mode) {
                    char ampm_buf[3];
                    memset(ampm_buf, 0, 3);
                    strftime(hour_buf, 3, "%l", localtime(&currtime));
                    strftime(ampm_buf, 3, "%p", localtime(&currtime));
                    g15r_renderString(canvas, (unsigned char *)ampm_buf, 0, G15_TEXT_LARGE, 135, 26);
                } else { // 24 hour format
                    strftime(hour_buf, 3, "%H", localtime(&currtime));
                }
                g15r_renderString(canvas, (unsigned char *)hour_buf, 0, 39, 30, 2);
                //g15r_G15FPrint(canvas, ":", 77, -3, 39, 0, 1, 0);

                // minute
                strftime(min_buf, 3, "%M", localtime(&currtime));
                g15r_renderString(canvas, (unsigned char *)min_buf, 0, 39, 86, 2);

                // date string
                if (g510s_data.show_date) {
                    char date_buf[40];
                    memset(date_buf, 0, 40);
                    strftime(date_buf, 40, "%A %B %e %Y", localtime(&currtime));
                    g15r_renderString(canvas, (unsigned char *)date_buf, 0, G15_TEXT_MED, 80 - ((strlen(date_buf) * 5) / 2), 35);
                }
            }
        }

        memset(lcd->buf, 0, G15_BUFFER_LEN);
        memcpy(lcd->buf, canvas->buffer, G15_BUFFER_LEN);
        lcd->ident = currtime;

        free(canvas);
    }
    return;
}
