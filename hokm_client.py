import socket
import sys
import curses
import time
from collections import deque


info_height = 5
top_off = 1
left_off = 1
rigth_off = left_off
mrg = 1
head_height = 1
table_height = 3
hand_height = 1
pan_width = 56 # curses.COLS - rigth_off - left_off
alert_height = 1
 
argc = len(sys.argv)
ip_addr = sys.argv[1] if argc > 1 else 'localhost'
player_id = int(sys.argv[2]) if argc > 2 else 1

port = 23345 + player_id
tcp_rcv_sz = 4*1024

SU_UNC = ("\u2660", "\u2665", "\u2663", "\u2666", "-")

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

def connect_run(stdscr, head_pan, info_pan, alert_pan, hand_pan, table_pan, inp_i, inp_j):
	w_l = (head_pan, info_pan, alert_pan, hand_pan, table_pan, stdscr)
	with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sc:
		sc.connect((ip_addr, port))
		inf_hist = deque(maxlen=info_height)
		win_write(alert_pan, "Connected to the Hokm server. Wait...", attrib=curses.color_pair(3))
		stop = False
		while (not stop):
			curses.curs_set(0)
			if curses.is_term_resized(curses.LINES, curses.COLS):
				curses.resizeterm(curses.LINES, curses.COLS)
			refresh_all(w_l)
			b_srv_msg = sc.recv(tcp_rcv_sz)
			srv_msg_raw = b_srv_msg.decode('ascii')
			srv_msg_list = srv_msg_raw.split('[SEP]')
			for srv_msg in srv_msg_list:
				if len(srv_msg) < 4:
					continue
				com = srv_msg[:4]
				msg = srv_msg[4:]
				stdscr.refresh()
				if com == '/HED':
					_, w = head_pan.getmaxyx()
					hed_attr = curses.color_pair(4)
					if msg[:4] == '/RSC':
						sct = "Scores " + msg[4] + " : " + msg[6] + " "
						win_write(head_pan, sct, 0, (w - len(sct))//2 - 3, hed_attr)
					elif msg[:4] == '/GSC':
						gscl = msg[4:].split(':')
						sm = "Game scores " + gscl[0] + " : " + gscl[1] + " "
						win_write(head_pan, sm, 0, w - len(sm) - 1, hed_attr, stdscr)
					elif msg[:4] == '/TRM':
						s = infer_suit(msg[4:])
						t_str = "Trump "
						win_write(head_pan, t_str, 0, 0, hed_attr)
						win_write(head_pan, " " + SU_UNC[s] + " ", 0, len(t_str), suit_str_attrib(s))
					else:
						win_clr_write(head_pan, msg)
				elif com == '/INF':
					info_pan.clear()
					inf_hist.append(msg)
					for i, m in enumerate(inf_hist):
						win_write(info_pan, m, i, 0, curses.color_pair(6))
				elif com == '/ALR':
					win_clr_write(alert_pan, msg, attrib=curses.color_pair(3))
				elif com == '/TBL':
					show_table(table_pan, msg, player_id)
				elif com == '/HND':
					show_hand(hand_pan, msg)
				elif com == '/INP':
					inp_str = user_input(stdscr, inp_i, inp_j, msg, 16, curses.color_pair(5))
					inp_str += '\0'
					b_inp_str = inp_str.encode('ascii')
					sc.sendall(b_inp_str)
				elif com == '/WIT':
					time.sleep(float(msg))
				elif com == '/END':
					stop = True
					table_pan.clear()
					table_pan.refresh()
					hand_pan.clear()
					hand_pan.refresh()
					stdscr.getch()
				else:
					pass

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
	
def suit_str_attrib(s):
	if s >= 0 and s < 4:
		attrib = curses.color_pair(1) if s % 2 == 0 else curses.color_pair(2)
	else:
		attrib = curses.A_NORMAL
	return attrib 
	
def card_unc_att(card_str):
	s = infer_suit(card_str[1])
	s = 4 if s < 0 or s > 3 else s
	card_unc = card_str[0] + SU_UNC[s]
	return card_unc, suit_str_attrib(s)

def show_hand(hand_pan, hand_str):
	hand_pan.clear()
	j = (pan_width - len(hand_str.replace(' ', ''))) // 2
	for su_h in hand_str.split(","):
		if len(su_h) == 0:
			continue
		for c_s in su_h.split():
			hand_pan.addstr(0, j, *card_unc_att(c_s))
			j += 2
		hand_pan.addstr(0, j, " ")
		j += 1
	hand_pan.refresh()

def show_table(table_pan, table_str, player_id):
	u = (player_id + 4) % 4
	c = (u + 2) % 4
	a = (u + 1) % 4
	b = (u + 3) % 4
	lo = (pan_width - 6)//2
	tbl = table_str.split()
	table_pan.clear()
	table_pan.addstr(0, lo + 2, *card_unc_att(tbl[c]))
	table_pan.addstr(1, lo + 0, *card_unc_att(tbl[b]))
	table_pan.addstr(1, lo + 4, *card_unc_att(tbl[a]))
	table_pan.addstr(2, lo + 2, *card_unc_att(tbl[u]))
	table_pan.refresh()
	
def user_input(curs_win, i=0, j=0, prompt="", max_len=0, attr=curses.A_NORMAL, stdscr=None):
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
			if stdscr is not None:
				stdscr.addstr(curses.LINES - 1, 0, "err: " + str(e), curses.A_NORMAL)
		if key == 10: # Enter key
			break
		if key in {ord('_'), ord('-'), ord('.'), ord('@')} or\
			key >= ord('a') and key <= ord('z') or\
				key >= ord('A') and key <= ord('Z') or\
					key >= ord('0') and key <= ord('9'):
			if J + len(u_input) < width - 1 and (max_len == 0 or len(u_input) < max_len):
				u_input += chr(key)
		elif key == 8 or key == curses.KEY_BACKSPACE:
			u_input = u_input[:-1]
		else:
			if stdscr is not None:
				stdscr.addstr(curses.LINES - 1, 0, "key: " + str(key), curses.A_NORMAL)
		try:
			win_write(curs_win, u_input, i, J, attr, stdscr)
			curs_win.move(i, J + len(u_input))
			curs_win.clrtoeol()
		except curses.error as e:
			if stdscr is not None:
				stdscr.addstr(curses.LINES - 1, 0, "err: " + str(e), curses.A_NORMAL)
	curs_win.move(i, j)
	curs_win.clrtoeol()
	return u_input

def win_write(curs_win, st="", i=0, j=0, attrib=curses.A_NORMAL, stdscr=None):
	height, width = curs_win.getmaxyx()
	try:
		if i < height and i >= 0:
			curs_win.addnstr(i, j, st, width-j-1, attrib)
			curs_win.refresh()
	except curses.error as e:
		if stdscr is not None:
			stdscr.addstr(curses.LINES - 1, 0, "err: " + str(e), curses.A_NORMAL)

def win_clr_write(curs_win, st="", i=0, j=0, attrib=curses.A_NORMAL):
	curs_win.clear()
	win_write(curs_win, st, i, j, attrib)

def main(stdscr):
	curses.start_color()
	_, attr = stdscr.getyx()
	bkg_color = attr & curses.A_COLOR
	curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_WHITE)
	curses.init_pair(2, curses.COLOR_RED, curses.COLOR_WHITE)
	curses.init_pair(3, 13, bkg_color)
	curses.init_pair(4, curses.COLOR_CYAN, bkg_color)
	curses.init_pair(5, curses.COLOR_GREEN, bkg_color)
	curses.init_pair(6, 8, bkg_color)
	
	stdscr.clear()
	stdscr.refresh()
	

	i = top_off
	head_pan = curses.newwin(head_height, pan_width, i, left_off)
	i += head_height
	stdscr.hline(i, 0, curses.ACS_HLINE, pan_width)
	i += mrg*2
	table_pan = curses.newwin(table_height, pan_width, i, 0)
	i += table_height + mrg
	hand_pan = curses.newwin(hand_height, pan_width, i, 0)
	i += hand_height + mrg
	info_pan = curses.newwin(info_height, pan_width, i, left_off)
	i += info_height
	alert_pan = curses.newwin(alert_height, pan_width, i, left_off)
	i += alert_height
	
	connect_run(stdscr, head_pan, info_pan, alert_pan, hand_pan, table_pan, i, left_off)

def refresh_all(w_l):
    for w in w_l:
        w.refresh()

# Run the curses application
curses.wrapper(main)
