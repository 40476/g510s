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

#include "g510s.h"

#define MAX_SCRIPT_LINES 32
#define MAX_LINE_LEN 256
#define MAX_CMD_OUTPUT 256
#define MAX_SCRIPT_VARS 32
#define MAX_VAR_NAME 32
#define MAX_VAR_VALUE 256

typedef struct {
    char name[MAX_VAR_NAME];
    char value[MAX_VAR_VALUE];
} script_var_t;

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
    if (!f) return 0; // file not found

    char line[MAX_LINE_LEN];
    int rendered = 0;
    static int last_line_y[MAX_SCRIPT_LINES] = {0}; // For line graph state

    // --- Variable storage ---
    script_var_t vars[MAX_SCRIPT_VARS];
    int var_count = 0;
    // --- End variable storage ---

    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (line[0] == 0 || line[0] == '#') continue;
        
        // --- Variable definition: %varname // command // ---
        if (line[0] == '%') {
            char *p = line + 1;
            char varname[MAX_VAR_NAME] = {0};
            int vi = 0;
            while (*p && (isalnum(*p) || *p == '_') && vi < MAX_VAR_NAME-1) {
                varname[vi++] = *p++;
            }
            varname[vi] = 0;
            while (*p == ' ' || *p == '\t') p++;
            // Look for // ... //
            char *cmd_start = strstr(p, "//");
            if (!*varname || !cmd_start) continue;
            cmd_start += 2;
            char *cmd_end = strstr(cmd_start, "//");
            if (!cmd_end) continue;
            *cmd_end = 0;
            char *cmd = cmd_start;
            char output[MAX_VAR_VALUE] = {0};
            exec_cmd(cmd, output, sizeof(output));
            // Remove trailing newlines
            size_t outlen = strlen(output);
            while (outlen > 0 && (output[outlen - 1] == '\n' || output[outlen - 1] == '\r')) {
                output[--outlen] = '\0';
            }
            // Store variable
            if (var_count < MAX_SCRIPT_VARS) {
                strncpy(vars[var_count].name, varname, MAX_VAR_NAME-1);
                vars[var_count].name[MAX_VAR_NAME-1] = 0;
                strncpy(vars[var_count].value, output, MAX_VAR_VALUE-1);
                vars[var_count].value[MAX_VAR_VALUE-1] = 0;
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
                int percent = atoi(output);
                if (percent < 0) percent = 0;
                if (percent > 100) percent = 100;

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
                        int tmp = fill_y0;
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
                    static int line_points[MAX_SCRIPT_LINES][256] = {{0}};
                    static int line_pos[MAX_SCRIPT_LINES] = {0};
                    int idx = rendered % MAX_SCRIPT_LINES;
                    int px = x;
                    int py = y + h - (h * percent / 100);

                    int pos = line_pos[idx];
                    if (w > 255) w = 255;
                    line_points[idx][pos] = py;
                    line_pos[idx] = (pos + 1) % w;

                    // Flip X axis for line graph
                    for (int i = 0; i < w - 1; ++i) {
                        int p1 = (pos + 1 + i) % w;
                        int p2 = (pos + 2 + i) % w;
                        int x1 = x + (flip_x ? (w - 1 - i) : i);
                        int x2 = x + (flip_x ? (w - 2 - i) : i + 1);
                        int y1 = line_points[idx][p1];
                        int y2 = line_points[idx][p2];
                        int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
                        int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
                        int err = dx + dy, e2;
                        int cx = x1, cy = y1;
                        while (1) {
                            g15r_setPixel(canvas, cx, cy, 1);
                            if (cx == x2 && cy == y2) break;
                            e2 = 2 * err;
                            if (e2 >= dy) { err += dy; cx += sx; }
                            if (e2 <= dx) { err += dx; cy += sy; }
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

        memset(lcd->buf, 0, G15_BUFFER_LEN);
        memcpy(lcd->buf, canvas->buffer, G15_BUFFER_LEN);
        lcd->ident = currtime;

        free(canvas);
    }
    return;
}