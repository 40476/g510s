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


#define G510S_VERSION "0.0.5"

#ifndef SO_PRIORITY
#define SO_PRIORITY 12
#endif

#define LISTEN_ADDR "127.0.0.1"
#define LISTEN_PORT 15550

#define MAX_CLIENTS 10

#define CLIENT_CMD_GET_KEYSTATE 'k'
#define CLIENT_CMD_SWITCH_PRIORITIES 'p'
#define CLIENT_CMD_IS_FOREGROUND 'v'
#define CLIENT_CMD_IS_USER_SELECTED 'u'
#define CLIENT_CMD_BACKLIGHT 0x80
#define CLIENT_CMD_CONTRAST 0x40
#define CLIENT_CMD_MKEY_LIGHTS 0x20
#define CLIENT_CMD_KEY_HANDLER 0x10

#define SERV_HELO "G15 daemon HELLO"

#ifndef GKEY_STRLEN
#define GKEY_STRLEN 1024
#endif

typedef struct lcd_s {
  int lcd_type;
  unsigned char buf[1048];
  int max_x;
  int max_y;
  int connection;
  long int ident;
  unsigned int backlight_state;
  unsigned int mkey_state;
  unsigned int contrast_state;
  unsigned int state_changed;
  unsigned int usr_foreground;
} lcd_t;

typedef struct lcdnode_s lcdnode_t;
typedef struct lcdlist_s lcdlist_t;

struct lcdnode_s {
  lcdlist_t *list;
  lcdnode_t *prev;
  lcdnode_t *next;
  lcdnode_t *last_priority;
  lcd_t *lcd;
} lcdnode_s;

struct lcdlist_s {
  lcdnode_t *head;
  lcdnode_t *tail;
  lcdnode_t *current;
} lcdlist_s;

//pthread_mutex_t lcdlist_mutex;

struct m_data_s {
  int red;
  int green;
  int blue;
  char g1[GKEY_STRLEN];
  char g2[GKEY_STRLEN];
  char g3[GKEY_STRLEN];
  char g4[GKEY_STRLEN];
  char g5[GKEY_STRLEN];
  char g6[GKEY_STRLEN];
  char g7[GKEY_STRLEN];
  char g8[GKEY_STRLEN];
  char g9[GKEY_STRLEN];
  char g10[GKEY_STRLEN];
  char g11[GKEY_STRLEN];
  char g12[GKEY_STRLEN];
  char g13[GKEY_STRLEN];
  char g14[GKEY_STRLEN];
  char g15[GKEY_STRLEN];
  char g16[GKEY_STRLEN];
  char g17[GKEY_STRLEN];
  char g18[GKEY_STRLEN];
};

struct g510s_data_s {
  int gui_hidden;
  int mkey_state;
  struct m_data_s m1;
  struct m_data_s m2;
  struct m_data_s m3;
  struct m_data_s mr;
  int clock_mode;
  int show_date;
  int auto_save_on_quit;
} g510s_data;

int leaving;
int update;
int device_found;
unsigned int connected_clients;
unsigned int current_key_state;

int init_uinput();
void exit_uinput();
void process_keys(lcdlist_t *displaylist, unsigned int key, unsigned int key_state);

void digital_clock(lcd_t *lcd);

void init_data();
int check_dir();
int load_config();
int save_config();

lcdlist_t *lcdlist_init();
lcdnode_t *lcdnode_add(lcdlist_t **display_list);
void lcdnode_remove(lcdnode_t *oldnode);
void lcdlist_destroy(lcdlist_t **displaylist);

void send_keystate(lcd_t *client, unsigned int key);
int client_connect(lcdlist_t **lcdlist, int listening_socket);
int init_sockserver();
int g15_send(int sock, char *buf, unsigned int len);
int g15_recv(lcdnode_t *lcdnode, int sock, char *buf, unsigned int len);

int is_number(char number[]);
void convert_buf(lcd_t *lcd, unsigned char * orig_buf);
void set_mkey_state(int state);
void set_color();
void run_gkey_cmd(int gkey);

void *lcd_client_function(void *display);
void *key_function(void *lcdlist);
void *update_function(void *lcdlist);
void *server_function(void *lcdlist);
