#!/usr/bin/env python3

import socket
import sys
import curses
import time
from collections import deque

class HokmClient:

    SU_UNC = ("\u2660", "\u2665", "\u2663", "\u2666", "-") 
    
    def __init__(self, stdscr, name, ip_addr='localhost', port=23345):
        self.stdscr = stdscr
        self.name = name
        self.ip_addr = ip_addr
        self.port = port  # single shared port
        self.player_id = -1  # will be set after assignment from server
        self.team_id = -1
        self.tcp_rcv_sz = 4 * 1024
        self.inf_hist = deque(maxlen=5)
        self.game_scores = [0, 0]
        self.hand_scores = [0, 0]
        self.trump = -1

        self.info_height = 5
        self.top_off = 1
        self.left_off = 1
        self.rigth_off = self.left_off
        self.mrg = 1
        self.head_height = 1
        self.table_height = 3
        self.hand_height = 1
        self.pan_width = 56  # curses.COLS - rigth_off - left_off
        self.alert_height = 1
        
        self.head_pan = None
        self.hand_pan = None
        self.info_pan = None
        self.table_pan = None
        self.alert_pan = None
        
    @staticmethod
    def infer_suit(suit_str):
        if len(suit_str) == 0:
            return 4
        if suit_str == 'S':
            return 0
        elif suit_str == 'H':
            return 1
        elif suit_str == 'C':
            return 2
        elif suit_str == 'D':
            return 3
        else:
            return 4

    @staticmethod
    def infer_rnk(rnk_str):
        if rnk_str == 'A':
            return 12
        elif rnk_str == 'K':
            return 11
        elif rnk_str == 'Q':
            return 10
        elif rnk_str == 'J':
            return 9
        elif rnk_str == 'X':
            return 8
        else:
            return int(rnk_str) - 2

    @staticmethod
    def suit_str_attrib(s):
        if 0 <= s < 4:
            attrib = curses.color_pair(1) if s % 2 == 0 else curses.color_pair(2)
        else:
            attrib = curses.A_NORMAL
        return attrib
    
    def update_head(self):
        _, w = self.head_pan.getmaxyx()
        hed_attr = curses.color_pair(4)
        sct = f"Hand Scores {self.hand_scores[0]} : {self.hand_scores[1]}"
        HokmClient.win_write(self.head_pan, sct, 0, 0, hed_attr)
        i = (w - 3)//2
        HokmClient.win_write(self.head_pan, " " + self.SU_UNC[self.trump] + " ", 0, i, HokmClient.suit_str_attrib(self.trump))
        sm = f"Game scores {self.game_scores[0]} : {self.game_scores[1]}"
        HokmClient.win_write(self.head_pan, sm, 0, w - len(sm) - 1, hed_attr)              

    @staticmethod
    def card_unc_att(card_str):
        s = HokmClient.infer_suit(card_str[1]) if len(card_str) >= 2 else 4
        s = 4 if s < 0 or s > 3 else s
        first_char = card_str[0] if len(card_str) > 0 else "-"
        card_unc = first_char + HokmClient.SU_UNC[s]
        return card_unc, HokmClient.suit_str_attrib(s)

    def update_hand_pan(self, hand_str):
        self.hand_pan.clear()
        j = (self.pan_width - len(hand_str.replace(' ', ''))) // 2
        for su_h in hand_str.split(","):
            if len(su_h) == 0:
                continue
            for c_s in su_h.split():
                self.hand_pan.addstr(0, j, *HokmClient.card_unc_att(c_s))
                j += 2
            self.hand_pan.addstr(0, j, " ")
            j += 1
        self.hand_pan.refresh()

    def update_table(self, table_str):
        u = self.player_id
        c = (u + 2) % 4
        a = (u + 1) % 4
        b = (u + 3) % 4
        lo = (self.pan_width - 6) // 2
        tbl = table_str.split()
        self.table_pan.clear()
        while len(tbl) < 4:
            tbl.append("--")
        self.table_pan.addstr(0, lo + 2, *HokmClient.card_unc_att(tbl[c]))
        self.table_pan.addstr(1, lo + 0, *HokmClient.card_unc_att(tbl[b]))
        self.table_pan.addstr(1, lo + 4, *HokmClient.card_unc_att(tbl[a]))
        self.table_pan.addstr(2, lo + 2, *HokmClient.card_unc_att(tbl[u]))
        self.table_pan.refresh()
    
    @staticmethod
    def user_input(curs_win, i=0, j=0, prompt="", max_len=0, attr=curses.A_NORMAL):
        height, width = curs_win.getmaxyx()
        curses.curs_set(1)
        u_input = ""
        curs_win.move(i, j)
        curs_win.clrtoeol()
        curs_win.addstr(i, j, prompt, attr)
        J = j + len(prompt)
        curs_win.move(i, J)
        while True:
            curs_win.refresh()
            key = 0
            try:
                key = curs_win.getch()
            except curses.error as e:
                print("inp err: " + str(e), file=sys.stderr)
            if key == 10:  # Enter key
                break
            if key in {ord('_'), ord('-'), ord('.'), ord('@'), ord(' ')} or \
                    ord('a') <= key <= ord('z') or \
                    ord('A') <= key <= ord('Z') or \
                    ord('0') <= key <= ord('9'):
                if J + len(u_input) < width - 1 and (max_len == 0 or len(u_input) < max_len):
                    u_input += chr(key)
            elif key in (8, curses.KEY_BACKSPACE, 127):
                u_input = u_input[:-1]
            else:
                pass
            try:
                HokmClient.win_write(curs_win, u_input, i, J, attr)
                curs_win.move(i, J + len(u_input))
                curs_win.clrtoeol()
            except curses.error as e:
                print("err: " + str(e), file=sys.stderr)
        curs_win.move(i, j)
        curs_win.clrtoeol()
        return u_input

    @staticmethod
    def win_write(curs_win, st="", i=0, j=0, attrib=curses.A_NORMAL):
        height, width = curs_win.getmaxyx()
        try:
            if 0 <= i < height:
                curs_win.addnstr(i, j, st, width - j - 1, attrib)
                curs_win.refresh()
        except curses.error as e:
            print("err: " + str(e), file=sys.stderr)
                # self.stdscr.addnstr(curses.LINES - 1, 0, "err: " + str(e), self.pan_width - 1, curses.A_NORMAL)
                # self.stdscr.refresh()

    @staticmethod
    def win_clr_write(curs_win, st="", i=0, j=0, attrib=curses.A_NORMAL):
        curs_win.clear()
        HokmClient.win_write(curs_win, st, i, j, attrib)

    @staticmethod
    def _recv_line(sc):
        # Read until '\n'
        data = bytearray()
        while True:
            ch = sc.recv(1)
            if not ch:
                return None
            if ch == b'\n':
                break
            data += ch
            if len(data) > 65536:
                break
        try:
            return data.decode('ascii', errors='ignore')
        except Exception:
            return data.decode('utf-8', errors='ignore')

    def connect_run(self, inp_i, inp_j, debug=False):
        w_l = (self.head_pan, self.info_pan, self.alert_pan, self.hand_pan, self.table_pan, self.stdscr)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sc:
            sc.connect((self.ip_addr, self.port))

            # Expect server to assign player id by order and send "OK <pid>"
            assigned = False
            tries = 0
            HokmClient.win_clr_write(self.alert_pan, "Connected. Waiting for assignment...", attrib=curses.color_pair(3))
            while not assigned and tries < 8:
                line = HokmClient._recv_line(sc)
                if line is None:
                    HokmClient.win_clr_write(self.alert_pan, "Server closed during assignment.", attrib=curses.color_pair(3))
                    return
                if line.startswith("OK "):
                    try:
                        self.player_id = int(line.split()[1])
                    except Exception:
                        self.player_id = -1
                    self.team_id = self.player_id % 2
                    assigned = True
                    HokmClient.win_clr_write(self.alert_pan, f"Assigned player_id: {self.player_id}, team_id: {self.team_id}", attrib=curses.color_pair(3))
                    break
                else:
                    HokmClient.win_clr_write(self.alert_pan, line, attrib=curses.color_pair(3))
                tries += 1

            if not assigned:
                HokmClient.win_clr_write(self.alert_pan, "Assignment failed.", attrib=curses.color_pair(3))
                return
            
            if not self.name:
                self.name = f"Player_{self.player_id}"
            
            stop = False
            recv_buf = ""  # buffer for [SEP]-delimited messages

            while not stop:
                curses.curs_set(0)
                if curses.is_term_resized(curses.LINES, curses.COLS):
                    curses.resizeterm(curses.LINES, curses.COLS)
                HokmClient.refresh_all(w_l)

                try:
                    b_srv_msg = sc.recv(self.tcp_rcv_sz)
                except Exception as e:
                    HokmClient.win_clr_write(self.alert_pan, f"Recv error: {e}", attrib=curses.color_pair(3))
                    break

                if not b_srv_msg:
                    HokmClient.win_clr_write(self.alert_pan, "Disconnected from server.", attrib=curses.color_pair(3))
                    break

                try:
                    chunk = b_srv_msg.decode('ascii', errors='ignore')
                except Exception:
                    chunk = b_srv_msg.decode('utf-8', errors='ignore')

                recv_buf += chunk

                # Process complete messages separated by [SEP]
                while True:
                    idx = recv_buf.find('[SEP]')
                    if idx == -1:
                        break
                    srv_msg = recv_buf[:idx]
                    recv_buf = recv_buf[idx + 5:]  # len("[SEP]") == 5

                    if len(srv_msg) < 4:
                        continue
                    com = srv_msg[:4]
                    msg = srv_msg[4:]
                    if debug:
                        print("srv_msg: ", srv_msg, file=sys.stderr)
                    self.stdscr.refresh()

                    if com == '/RSC':
                        hsc = msg.split(':')
                        if len(hsc) < 2:
                            hsc += ['0'] * (2 - len(hsc))
                        self.hand_scores = [int(hsc[0]), int(hsc[1])] if self.team_id == 0 else [int(hsc[1]), int(hsc[0])]
                        self.update_head()
                    elif com == '/GSC':
                        gsc = msg.split(':')
                        if len(gsc) < 2:
                            gsc += ['0'] * (2 - len(gsc))
                        self.game_scores = [int(gsc[0]), int(gsc[1])] if self.team_id == 0 else [int(gsc[1]), int(gsc[0])]
                        self.update_head()
                    elif com == '/TRM':
                        self.trump = HokmClient.infer_suit(msg)
                        self.update_head()
                    elif com == '/INF':
                        self.info_pan.clear()
                        self.inf_hist.append(msg)
                        for i, m in enumerate(self.inf_hist):
                            HokmClient.win_write(self.info_pan, m, i, 0, curses.color_pair(6))
                    elif com == '/ALR':
                        HokmClient.win_clr_write(self.alert_pan, msg, attrib=curses.color_pair(3))
                    elif com == '/TBL':
                        self.update_table(msg)
                    elif com == '/HND':
                        self.update_hand_pan(msg)
                    elif com == '/INP':
                        if msg.startswith("Enter your name"):
                            inp_str = self.name
                        else:
                            inp_str = HokmClient.user_input(self.stdscr, inp_i, inp_j, msg, 20, curses.color_pair(5))
                        b_inp_str = (inp_str + '\n').encode('ascii', errors='ignore')
                        try:
                            sc.sendall(b_inp_str)
                        except Exception as e:
                            HokmClient.win_clr_write(self.alert_pan, f"Send error: {e}", attrib=curses.color_pair(3))
                            stop = True
                    elif com == '/WIT':
                        try:
                            time.sleep(float(msg))
                        except Exception:
                            pass
                    elif com == '/END':
                        stop = True
                        self.table_pan.clear()
                        self.table_pan.refresh()
                        self.hand_pan.clear()
                        self.hand_pan.refresh()
                        self.stdscr.getch()
                    else:
                        HokmClient.win_clr_write(self.alert_pan, f"Err: com: {com} msg: {msg}", attrib=curses.color_pair(3))

    @staticmethod
    def refresh_all(w_l):
        for w in w_l:
            w.refresh()

    def main(self):
        curses.start_color()
        _, attr = self.stdscr.getyx()
        bkg_color = attr & curses.A_COLOR
        curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_WHITE)
        curses.init_pair(2, curses.COLOR_RED, curses.COLOR_WHITE)
        curses.init_pair(3, 13, bkg_color)
        curses.init_pair(4, curses.COLOR_CYAN, bkg_color)
        curses.init_pair(5, curses.COLOR_GREEN, bkg_color)
        curses.init_pair(6, 8, bkg_color)

        self.stdscr.clear()
        self.stdscr.refresh()

        i = self.top_off
        self.head_pan = curses.newwin(self.head_height, self.pan_width, i, self.left_off)
        i += self.head_height
        self.stdscr.hline(i, 0, curses.ACS_HLINE, self.pan_width)
        i += self.mrg * 2
        self.table_pan = curses.newwin(self.table_height, self.pan_width, i, 0)
        i += self.table_height + self.mrg
        self.hand_pan = curses.newwin(self.hand_height, self.pan_width, i, 0)
        i += self.hand_height + self.mrg
        self.info_pan = curses.newwin(self.info_height, self.pan_width, i, self.left_off)
        i += self.info_height
        self.alert_pan = curses.newwin(self.alert_height, self.pan_width, i, self.left_off)
        i += self.alert_height

        self.connect_run(i, self.left_off, debug=False)

def _main(stdscr):
    # Arg order: server_ip first, name second (optional), port third (optional)
    argc = len(sys.argv)
    ip_addr = sys.argv[1] if argc > 1 else 'localhost'
    name = sys.argv[2] if argc > 2 else ''
    port = int(sys.argv[3]) if argc > 3 else 23345
    client = HokmClient(stdscr, name, ip_addr, port)
    client.main()

if __name__ == '__main__':
    curses.wrapper(_main)
