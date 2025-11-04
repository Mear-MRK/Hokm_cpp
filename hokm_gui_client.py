#!/usr/bin/env python3
# hokm_gui_client.py

import sys
import time
import socket
import threading
import re
import getpass
import argparse
import os
from dataclasses import dataclass
from typing import List, Optional, Callable, Dict, Tuple

from PySide6.QtCore import (
    Qt, QSize, QRectF, Signal, Slot, QObject, QThread, QEvent, QTimer
)
from PySide6.QtGui import (
    QColor, QPainter, QPen, QFont, QGuiApplication
)
from PySide6.QtWidgets import (
    QApplication, QWidget, QMainWindow, QPushButton, QHBoxLayout,
    QVBoxLayout, QLabel, QLineEdit, QListWidget, QListWidgetItem,
    QStatusBar
)

DEFAULT_PORT = 23345

SUITS = ["S", "H", "C", "D"]
RANKS = ["2", "3", "4", "5", "6", "7", "8", "9", "X", "J", "Q", "K", "A"]

SUIT_SYMBOL = {"C": "♣", "D": "♦", "H": "♥", "S": "♠"}
SUIT_COLOR = {
    "C": QColor("#111111"),
    "S": QColor("#111111"),
    "D": QColor("#C21807"),
    "H": QColor("#C21807"),
}

RANK_TEXT = {
    "2": "2", "3": "3", "4": "4", "5": "5", "6": "6", "7": "7",
    "8": "8", "9": "9", "X": "10", "J": "J", "Q": "Q", "K": "K", "A": "A"
}

def normalize_card_code(code: str) -> Optional[str]:
    if not code:
        return None
    s = code.strip().upper()
    if s == "--":
        return None
    if len(s) == 3 and s[:2] == "10":
        s = "X" + s[2]
    if len(s) >= 2 and s[0] == "T":
        s = "X" + s[1]
    if len(s) >= 2:
        r, u = s[0], s[1]
        if r in RANKS and u in SUITS:
            return r + u
    return None

@dataclass(frozen=True)
class Card:
    rank: str
    suit: str

    @staticmethod
    def from_code(code: Optional[str]) -> "Card":
        norm = normalize_card_code(code or "")
        if not norm:
            raise ValueError(f"Invalid card code: {code}")
        return Card(rank=norm[0], suit=norm[1])

    @property
    def code(self) -> str:
        return f"{self.rank}{self.suit}"

def card_to_human(code: str) -> str:
    try:
        c = Card.from_code(code)
        return f"{RANK_TEXT[c.rank]}{SUIT_SYMBOL[c.suit]}"
    except Exception:
        return code or "--"

def paint_card_face(p: QPainter, rect: QRectF, card: Card):
    rank = RANK_TEXT[card.rank]
    suit = SUIT_SYMBOL[card.suit]
    color = SUIT_COLOR[card.suit]

    p.setPen(QPen(QColor("#444444"), 2))
    p.setBrush(QColor("#FFFFFF"))
    p.drawRoundedRect(rect, 8, 8)

    p.setPen(color)
    f_rank = QFont(); f_rank.setPointSizeF(rect.height() * 0.11); f_rank.setBold(True)
    f_s_corner = QFont(); f_s_corner.setPointSizeF(rect.height() * 0.10)
    f_s_center = QFont(); f_s_center.setPointSizeF(rect.height() * 0.22)

    p.setFont(f_rank)
    p.drawText(QRectF(rect.x() + rect.width() * 0.10, rect.y() + rect.height() * 0.06,
                      rect.width() * 0.34, rect.height() * 0.18),
               Qt.AlignLeft | Qt.AlignTop, rank)
    p.setFont(f_s_corner)
    p.drawText(QRectF(rect.x() + rect.width() * 0.10, rect.y() + rect.height() * 0.20,
                      rect.width() * 0.34, rect.height() * 0.18),
               Qt.AlignLeft | Qt.AlignTop, suit)

    p.setFont(f_rank)
    p.drawText(QRectF(rect.right() - rect.width() * 0.44, rect.bottom() - rect.height() * 0.38,
                      rect.width() * 0.34, rect.height() * 0.18),
               Qt.AlignRight | Qt.AlignBottom, rank)
    p.setFont(f_s_corner)
    p.drawText(QRectF(rect.right() - rect.width() * 0.44, rect.bottom() - rect.height() * 0.24,
                      rect.width() * 0.34, rect.height() * 0.18),
               Qt.AlignRight | Qt.AlignBottom, suit)

    p.setFont(f_s_center)
    p.drawText(rect, Qt.AlignCenter, suit)

class TrumpIcon(QWidget):
    def __init__(self, parent: Optional[QWidget] = None):
        super().__init__(parent)
        self._suit: Optional[str] = None
        self.setFixedHeight(64)

    def setSuit(self, suit: Optional[str]):
        self._suit = suit if suit in SUITS else None
        self.update()

    def sizeHint(self) -> QSize:
        return QSize(200, 64)

    def paintEvent(self, event) -> None:
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing, True)
        rect = self.rect()

        bg_rect = rect.adjusted(rect.width() * 0.35, 8, -rect.width() * 0.35, -8)
        if bg_rect.width() < 56:
            bg_rect = QRectF(rect.center().x() - 28, rect.y() + 8, 56, rect.height() - 16)

        p.setPen(QPen(QColor("#CFD8DC"), 2.0))
        p.setBrush(QColor("#FFFFFF"))
        p.drawRoundedRect(bg_rect, 10, 10)

        f = QFont(); f.setBold(True); f.setPointSize(24)
        p.setFont(f)
        if not self._suit:
            p.setPen(QColor("#90A4AE"))
            p.drawText(bg_rect, Qt.AlignCenter, "?")
            return
        p.setPen(SUIT_COLOR[self._suit])
        p.drawText(bg_rect, Qt.AlignCenter, SUIT_SYMBOL[self._suit])

class WinnerBadge(QWidget):
    def __init__(self, parent: Optional[QWidget] = None):
        super().__init__(parent)
        self._text: str = ""
        self._color: QColor = QColor("#2E7D32")
        self.setFixedHeight(54)
        self.setVisible(True)

    def clear_text(self):
        self._text = ""
        self.update()

    def show_round(self, who: str):
        self._text = "Round winner: " + ("Your team" if who == "you" else "Opponent team")
        self._color = QColor("#2E7D32") if who == "you" else QColor("#E53935")
        self.update()

    def show_game(self, who: str):
        self._text = "Game winner: " + ("Your team" if who == "you" else "Opponent team")
        self._color = QColor("#2E7D32") if who == "you" else QColor("#E53935")
        self.update()

    def paintEvent(self, event) -> None:
        if not self._text:
            return
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing, True)
        rect = self.rect()

        f = QFont(); f.setBold(True); f.setPointSize(12)
        p.setFont(f)
        fm = p.fontMetrics()
        tw = fm.horizontalAdvance(self._text) + 28
        th = fm.height() + 12
        bw = max(160, min(rect.width() - 24, tw))
        bh = min(rect.height() - 8, th)
        bx = rect.center().x() - bw / 2
        by = rect.center().y() - bh / 2

        p.setPen(QPen(QColor("#CFD8DC"), 2.0))
        p.setBrush(QColor("#FFFFFF"))
        p.drawRoundedRect(QRectF(bx, by, bw, bh), 10, 10)

        p.setPen(self._color)
        p.drawText(QRectF(bx, by, bw, bh), Qt.AlignCenter, self._text)

class ScoreLabel(QWidget):
    def __init__(self, title: str, parent: Optional[QWidget] = None):
        super().__init__(parent)
        self._title = title
        self._us = 0
        self._opp = 0
        self.setFixedHeight(30)

    def setScores(self, us: int, opp: int):
        self._us = int(us)
        self._opp = int(opp)
        self.update()

    def sizeHint(self) -> QSize:
        return QSize(260, 30)

    def paintEvent(self, event) -> None:
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing, False)
        rect = self.rect()
        left_pad = 6
        right_pad = 6

        p.setPen(QColor("#CFD8DC"))
        f_title = QFont(); f_title.setPointSize(12); f_title.setBold(True)
        p.setFont(f_title)
        title_text = self._title
        p.drawText(QRectF(rect.x() + left_pad, rect.y(), rect.width() // 2, rect.height()),
                   Qt.AlignVCenter | Qt.AlignLeft, title_text)

        f_num = QFont(); f_num.setPointSize(14); f_num.setBold(True)
        p.setFont(f_num)
        fm = p.fontMetrics()
        s_us = str(self._us)
        s_opp = str(self._opp)
        colon_w = fm.horizontalAdvance(":")
        space_w = fm.horizontalAdvance(" ")
        us_w = fm.horizontalAdvance(s_us)
        opp_w = fm.horizontalAdvance(s_opp)
        block_w = us_w + space_w + colon_w + space_w + opp_w

        bx = rect.right() - right_pad - block_w
        y = rect.y()
        h = rect.height()

        p.setPen(QColor("#2E7D32"))
        p.drawText(QRectF(bx, y, us_w, h), Qt.AlignVCenter | Qt.AlignLeft, s_us)
        bx += us_w + space_w

        p.setPen(QColor("#B0BEC5"))
        p.drawText(QRectF(bx, y, colon_w, h), Qt.AlignVCenter | Qt.AlignLeft, ":")

        bx += colon_w + space_w

        p.setPen(QColor("#E53935"))
        p.drawText(QRectF(bx, y, opp_w, h), Qt.AlignVCenter | Qt.AlignLeft, s_opp)

class CardWidget(QWidget):
    clicked = Signal(str)

    def __init__(self, card: Card, parent: Optional[QWidget] = None):
        super().__init__(parent)
        self.card = card
        self._hover = False
        self._selected = False
        self.setFixedSize(QSize(80, 120))
        self.setCursor(Qt.PointingHandCursor)

    def setSelected(self, sel: bool):
        self._selected = sel
        self.update()

    def enterEvent(self, event: QEvent) -> None:
        self._hover = True
        self.update()

    def leaveEvent(self, event: QEvent) -> None:
        self._hover = False
        self.update()

    def mousePressEvent(self, event) -> None:
        if event.button() == Qt.LeftButton:
            self.clicked.emit(self.card.code)

    def paintEvent(self, event) -> None:
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing, True)
        rect = self.rect().adjusted(3, 3, -3, -3)

        border = QColor("#444444")
        if self._hover:
            border = QColor("#1976D2")
        if self._selected:
            border = QColor("#43A047")

        p.setPen(QPen(border, 2))
        p.setBrush(Qt.NoBrush)
        p.drawRoundedRect(rect, 8, 8)
        face = rect.adjusted(1.5, 1.5, -1.5, -1.5)
        paint_card_face(p, face, self.card)

class FlowHand(QWidget):
    playRequested = Signal(str)

    def __init__(self, parent: Optional[QWidget] = None):
        super().__init__(parent)
        self._codes: List[str] = []
        self._enabled = True
        self._selected: Optional[str] = None
        self._turn_active = False
        self.setMinimumHeight(160)

    def setTurnActive(self, active: bool):
        self._turn_active = active
        self.update()

    def setCards(self, codes: List[str]):
        self._codes = [c for c in codes if normalize_card_code(c)]
        if self._selected and self._selected not in self._codes:
            self._selected = None
        self._rebuild_children()
        self.updateGeometry()
        self.update()

    def setInteractive(self, enabled: bool):
        self._enabled = enabled
        for child in self.findChildren(CardWidget):
            child.setEnabled(enabled)
        self.setEnabled(enabled)
        self.update()

    def clearSelection(self):
        self._selected = None
        self._apply_selection_state()
        self._reposition_children()

    def resizeEvent(self, event) -> None:
        super().resizeEvent(event)
        self._reposition_children()

    def paintEvent(self, event) -> None:
        if self._turn_active:
            p = QPainter(self)
            p.setRenderHint(QPainter.Antialiasing, True)
            rect = self.rect().adjusted(3, 3, -3, -3)
            pen = QPen(QColor("#DAA520"), 6)
            p.setPen(pen)
            p.setBrush(Qt.NoBrush)
            p.drawRoundedRect(rect, 10, 10)
        super().paintEvent(event)

    def _on_card_clicked(self, code: str):
        if not self._enabled:
            return
        if self._selected == code:
            self.playRequested.emit(code)
            return
        self._selected = code
        self._apply_selection_state()
        self._reposition_children()

    def _apply_selection_state(self):
        for child in self.findChildren(CardWidget):
            child.setSelected(child.card.code == self._selected)

    def _rebuild_children(self):
        for child in list(self.children()):
            if isinstance(child, CardWidget):
                child.setParent(None)
                child.deleteLater()

        if not self._codes:
            return

        card_w, card_h = 80, 120
        overlap = 24
        total = len(self._codes)
        total_width = card_w + (total - 1) * overlap
        x = max(10, (self.width() - total_width) // 2)
        y = (self.height() - card_h) // 2

        for c in self._codes:
            w = CardWidget(Card.from_code(c), parent=self)
            w.setEnabled(self._enabled)
            w.move(x, y)
            w.show()
            w.clicked.connect(self._on_card_clicked)
            x += overlap

        self._apply_selection_state()

    def _reposition_children(self):
        kids = [child for child in self.children() if isinstance(child, CardWidget)]
        if not kids:
            return
        card_w, card_h = kids[0].width(), kids[0].height()
        overlap = 24
        total = len(kids)
        total_width = card_w + (total - 1) * overlap
        x0 = max(10, (self.width() - total_width) // 2)
        y0 = (self.height() - card_h) // 2
        raise_px = 14
        for child in kids:
            y = y0 - (raise_px if child.card.code == self._selected else 0)
            child.move(x0, y)
            x0 += overlap

class TableArea(QWidget):
    def __init__(self, parent: Optional[QWidget] = None):
        super().__init__(parent)
        self._cards: Dict[str, Optional[Card]] = {"u": None, "c": None, "a": None, "b": None}
        self._raw: Dict[str, Optional[str]] = {"u": None, "c": None, "a": None, "b": None}
        self._z_order: List[str] = ["c", "u", "b", "a"]
        self._names: Dict[str, str] = {"u": "You", "a": "Right", "c": "Across", "b": "Left"}
        self.setMinimumHeight(260)

    def setPlayerName(self, key: str, name: str):
        if key in self._names:
            self._names[key] = name or self._names[key]
            self.update()

    def setPlayerNames(self, names: Dict[str, str]):
        for k, v in names.items():
            if k in self._names and v is not None:
                self._names[k] = v
        self.update()

    def setPositions(self, positions: Dict[str, Optional[str]]):
        changed: List[str] = []
        for k in ("u", "c", "a", "b"):
            new_norm = normalize_card_code(positions.get(k) or "")
            if new_norm != self._raw.get(k):
                if new_norm is not None:
                    changed.append(k)
                self._raw[k] = new_norm
                self._cards[k] = Card.from_code(new_norm) if new_norm else None

        for k in changed:
            if k in self._z_order:
                self._z_order.remove(k)
            self._z_order.append(k)

        self._z_order = [k for k in self._z_order if k in ("u", "c", "a", "b")]
        for k in ("c", "u", "b", "a"):
            if k not in self._z_order:
                self._z_order.insert(0, k)

        self.update()

    def clear(self):
        self._cards = {"u": None, "c": None, "a": None, "b": None}
        self._raw = {"u": None, "c": None, "a": None, "b": None}
        self._z_order = ["c", "u", "b", "a"]
        self.update()

    def paintEvent(self, event) -> None:
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing, True)
        rect = self.rect()
        p.fillRect(rect, QColor("#14493E"))

        w, h = 80, 120
        cx, cy = rect.center().x(), rect.center().y()

        gap_v = max(8, min(16, int(rect.height() * 0.015)))
        gap_h = 16

        top_r = QRectF(cx - w / 2, cy - h - gap_v, w, h)
        bot_r = QRectF(cx - w / 2, cy + gap_v, w, h)
        left_r = QRectF(cx - w - gap_h - 6, cy - h / 2, w, h)
        right_r = QRectF(cx + gap_h + 6, cy - h / 2, w, h)

        rects = {"c": top_r, "u": bot_r, "b": left_r, "a": right_r}

        order = self._z_order if self._z_order else ["c", "u", "b", "a"]
        for key in order:
            card = self._cards.get(key)
            if card:
                paint_card_face(p, rects[key].adjusted(2, 2, -2, -2), card)

        f = QFont(); f.setPointSize(12); f.setBold(True)
        p.setFont(f)
        p.setPen(QColor("#FFFFFF"))
        fm = p.fontMetrics()

        name_top = self._names["c"]
        top_w = max(60, fm.horizontalAdvance(name_top) + 12)
        top_h = fm.height() + 6
        top_box = QRectF(top_r.center().x() - top_w / 2,
                         max(4, top_r.top() - top_h - 8),
                         top_w, top_h)
        p.drawText(top_box, Qt.AlignCenter, name_top)

        name_bot = self._names["u"]
        bot_w = max(60, fm.horizontalAdvance(name_bot) + 12)
        bot_h = fm.height() + 6
        bot_box = QRectF(bot_r.center().x() - bot_w / 2,
                         min(rect.height() - bot_h - 4, bot_r.bottom() + 6),
                         bot_w, bot_h)
        p.drawText(bot_box, Qt.AlignCenter, name_bot)

        name_left = self._names["b"]
        left_w = max(60, fm.horizontalAdvance(name_left) + 12)
        left_h = fm.height() + 6
        left_box = QRectF(max(4, left_r.left() - left_w - 8),
                          left_r.center().y() - left_h / 2,
                          left_w, left_h)
        p.drawText(left_box, Qt.AlignCenter, name_left)

        name_right = self._names["a"]
        right_w = max(60, fm.horizontalAdvance(name_right) + 12)
        right_h = fm.height() + 6
        right_box = QRectF(min(rect.width() - right_w - 4, right_r.right() + 8),
                           right_r.center().y() - right_h / 2,
                           right_w, right_h)
        p.drawText(right_box, Qt.AlignCenter, name_right)

class ProtocolAdapter:
    def set_callbacks(self, on_log: Callable[[str], None],
                      on_connected: Callable[[], None],
                      on_disconnected: Callable[[str], None],
                      on_hand: Callable[[List[str]], None],
                      on_table_linear: Callable[[List[str]], None],
                      on_your_turn: Callable[[bool], None],
                      on_game_end: Callable[[], None]) -> None:
        raise NotImplementedError

    def connect(self, host: str, port: int, seat: int) -> None:
        raise NotImplementedError

    def run(self, should_stop: Callable[[], bool]) -> None:
        raise NotImplementedError

    def play_card(self, code: str) -> None:
        raise NotImplementedError

    def close(self) -> None:
        raise NotImplementedError

class HokmAdapter(ProtocolAdapter):
    def __init__(self):
        self.cb_log: Callable[[str], None] = lambda m: None
        self.cb_connected: Callable[[], None] = lambda: None
        self.cb_disconnected: Callable[[str], None] = lambda r: None
        self.cb_hand: Callable[[List[str]], None] = lambda cards: None
        self.cb_table_linear: Callable[[List[str]], None] = lambda cards: None
        self.cb_your_turn: Callable[[bool], None] = lambda b: None
        self.cb_game_end: Callable[[], None] = lambda: None

        self._cb_trump: Callable[[Optional[str]], None] = lambda s: None
        self._cb_positions: Callable[[Dict[str, Optional[str]]], None] = lambda pos: None
        self._cb_scores: Callable[[int, int], None] = lambda us, opp: None
        self._cb_gscores: Callable[[int, int], None] = lambda us, opp: None
        self._cb_team: Callable[[int], None] = lambda tid: None
        self._cb_alert: Callable[[str], None] = lambda t: None
        self._cb_player_id: Callable[[int], None] = lambda pid: None
        self._cb_seat_names: Callable[[Dict[str, str]], None] = lambda nm: None

        self._get_user_input: Optional[Callable[[str, int], str]] = None

        self._sock: Optional[socket.socket] = None
        self._f = None
        self._connected = False
        self.player_id = 0
        self.my_team_id = 0
        self.tcp_rcv_sz = 4 * 1024

        self._host: str = ""
        self._port: int = 0

    def set_callbacks(self, on_log: Callable[[str], None],
                      on_connected: Callable[[], None],
                      on_disconnected: Callable[[str], None],
                      on_hand: Callable[[List[str]], None],
                      on_table_linear: Callable[[List[str]], None],
                      on_your_turn: Callable[[bool], None],
                      on_game_end: Callable[[], None]) -> None:
        self.cb_log = on_log
        self.cb_connected = on_connected
        self.cb_disconnected = on_disconnected
        self.cb_hand = on_hand
        self.cb_table_linear = on_table_linear
        self.cb_your_turn = on_your_turn
        self.cb_game_end = on_game_end

    def set_input_provider(self, fn: Callable[[str, int], str]) -> None:
        self._get_user_input = fn

    def set_suit_callbacks(self, on_trump: Callable[[Optional[str]], None]) -> None:
        self._cb_trump = on_trump

    def set_table_positions_callback(self, on_positions: Callable[[Dict[str, Optional[str]]], None]) -> None:
        self._cb_positions = on_positions

    def set_score_callbacks(self,
                            on_scores: Callable[[int, int], None],
                            on_game_scores: Callable[[int, int], None]) -> None:
        self._cb_scores = on_scores
        self._cb_gscores = on_game_scores

    def set_team_callback(self, on_team: Callable[[int], None]) -> None:
        self._cb_team = on_team

    def set_alert_callback(self, on_alert: Callable[[str], None]) -> None:
        self._cb_alert = on_alert

    def set_player_id_callback(self, on_pid: Callable[[int], None]) -> None:
        self._cb_player_id = on_pid

    def set_seat_names_callback(self, on_names: Callable[[Dict[str, str]], None]) -> None:
        self._cb_seat_names = on_names

    def _token_path(self, host: str, port: int) -> str:
        safe_host = (host or "localhost").replace("/", "_").replace(":", "_")
        return os.path.expanduser(f"~/.hokm_token_{safe_host}_{port}")

    def _load_token(self, host: str, port: int) -> Optional[str]:
        try:
            with open(self._token_path(host, port), "r") as f:
                t = f.read().strip()
                return t if t else None
        except FileNotFoundError:
            return None
        except Exception:
            return None

    def _save_token(self, host: str, port: int, tok: str) -> None:
        if not tok:
            return
        try:
            with open(self._token_path(host, port), "w") as f:
                f.write(tok)
        except Exception:
            pass

    def connect(self, host: str, port: int, seat: int) -> None:
        self._host, self._port = host, port
        sc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sc.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sc.settimeout(5.0)
        sc.connect((host, port))
        sc.settimeout(None)
        self._sock = sc
        self._f = sc.makefile("rwb", buffering=0)
        self._connected = True
        self.cb_connected()
        self.cb_log(f"Connected to {host}:{port}. Waiting for assignment...")

        tok = self._load_token(host, port)
        if tok:
            try:
                self.cb_log("Attempting to resume previous seat...")
                sc.sendall(f"RESUME {tok}\n".encode("ascii", errors="ignore"))
            except Exception:
                pass

        assigned = False
        tries = 0
        while not assigned and tries < 16:
            line = self._recv_line(sc)
            if line is None:
                self.cb_disconnected("Server closed during assignment")
                self.close()
                return
            if line.startswith("OK "):
                try:
                    self.player_id = int(line.split()[1])
                except Exception:
                    self.player_id = 0
                self.my_team_id = self.player_id % 2
                self._cb_team(self.my_team_id)
                self._cb_player_id(self.player_id)
                assigned = True
                self.cb_log("Seat assigned")
                break
            else:
                self.cb_log(line)
            tries += 1
        if not assigned:
            self.cb_disconnected("Assignment failed")
            self.close()
            return

    def run(self, should_stop: Callable[[], bool]) -> None:
        if not self._sock:
            self.cb_disconnected("not connected")
            return
        recv_buf = ""
        try:
            while self._connected and not should_stop():
                try:
                    b_srv_msg = self._sock.recv(self.tcp_rcv_sz)
                except Exception as e:
                    self.cb_log(f"Recv error: {e}")
                    break
                if not b_srv_msg:
                    self.cb_log("Disconnected from server.")
                    break
                try:
                    chunk = b_srv_msg.decode('ascii', errors='ignore')
                except Exception:
                    chunk = b_srv_msg.decode('utf-8', errors='ignore')
                recv_buf += chunk
                while True:
                    idx = recv_buf.find('[SEP]')
                    if idx == -1:
                        break
                    srv_msg = recv_buf[:idx]
                    recv_buf = recv_buf[idx + 5:]
                    if len(srv_msg) < 4:
                        continue
                    com = srv_msg[:4]
                    msg = srv_msg[4:]
                    self._handle_msg(com, msg)
        finally:
            if self._connected:
                self.cb_disconnected("remote closed")
            self.close()

    def _parse_scores(self, payload: str) -> Optional[Tuple[int, int]]:
        nums = re.findall(r'\d+', payload)
        if len(nums) >= 2:
            try:
                return int(nums[0]), int(nums[1])
            except Exception:
                return None
        if len(payload) >= 7 and payload[4].isdigit() and payload[6].isdigit():
            return int(payload[4]), int(payload[6])
        return None

    def _parse_tbl_pairs(self, payload: str):
        # returns list of (name, card_raw) in order; pads/truncates to 4
        pairs: List[Tuple[str, str]] = []
        for seg in payload.split(';'):
            seg = seg.strip()
            if not seg:
                continue
            name, sep, card = seg.partition(':')
            if sep != ':':
                continue
            name = (name or "").strip()
            card = (card or "").strip()
            pairs.append((name, card))
        while len(pairs) < 4:
            pairs.append(("", ""))
        return pairs[:4]

    def _handle_msg(self, com: str, msg: str) -> None:
        if com == '/RSC':
            pair = self._parse_scores(msg)
            if pair:
                t0, t1 = pair
                if self.my_team_id == 0:
                    self._cb_scores(t0, t1)
                else:
                    self._cb_scores(t1, t0)
        elif com == '/GSC':
            pair = self._parse_scores(msg)
            if pair:
                t0, t1 = pair
                if self.my_team_id == 0:
                    self._cb_gscores(t0, t1)
                else:
                    self._cb_gscores(t1, t0)
        elif com == '/TRM':
            suit_char = (msg.strip()[:1] or "-").upper()
            suit = suit_char if suit_char in SUITS else None
            self._cb_trump(suit)
        elif com == '/TOK':
            tok = (msg or "").strip()
            if tok:
                self._save_token(self._host, self._port, tok)
                self.cb_log("Reconnect token received.")
        elif com == '/INF':
            # Just forward to Info; do not parse names
            self.cb_log("/INF" + msg)
        elif com == '/ALR':
            self._cb_alert(msg)
        elif com == '/TBL':
            self._handle_table(msg)
        elif com == '/HND':
            self._handle_hand(msg)
        elif com == '/INP':
            self._handle_prompt(msg)
        elif com == '/WIT':
            try:
                time.sleep(float(msg))
            except Exception:
                pass
        elif com == '/END':
            self.cb_game_end()
            self.close()
        else:
            self.cb_log(f"[Unknown] {com} {msg}")

    def _handle_hand(self, hand_str: str) -> None:
        parts = hand_str.split(",")
        codes: List[str] = []
        for part in parts:
            for tok in part.split():
                norm = normalize_card_code(tok)
                if norm:
                    codes.append(norm)
        self.cb_hand(codes)

    def _handle_table(self, table_str: str) -> None:
        # Parse pairs by absolute seat order, map to relative seats by player_id
        pairs = self._parse_tbl_pairs(table_str)
        names = [nm for nm, _ in pairs]
        raw = [cd for _, cd in pairs]

        def norm(i: int) -> Optional[str]:
            c = (raw[i] or "").strip().upper()
            n = normalize_card_code(c)
            return n

        u = self.player_id % 4
        a = (u + 1) % 4
        c = (u + 2) % 4
        b = (u + 3) % 4

        positions = {
            "u": norm(u),
            "a": norm(a),
            "c": norm(c),
            "b": norm(b),
        }
        # Also send seat names derived from TBL (fallbacks if missing)
        seat_names = {
            "u": names[u] if names[u] else "You",
            "a": names[a] if names[a] else "Right",
            "b": names[b] if names[b] else "Left",
            "c": names[c] if names[c] else "Across",
        }
        self._cb_positions(positions)
        self._cb_seat_names(seat_names)

    def _handle_prompt(self, prompt: str) -> None:
        if not self._sock:
            return
        ans = ""
        if self._get_user_input:
            self.cb_your_turn(True)
            try:
                ans = self._get_user_input(prompt, 20) or ""
            finally:
                self.cb_your_turn(False)
        try:
            out = (ans + '\n').encode('ascii', errors='ignore')
            self._sock.sendall(out)
        except Exception as e:
            self.cb_log(f"Send error: {e}")

    def _recv_line(self, sc: socket.socket) -> Optional[str]:
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

    def play_card(self, code: str) -> None:
        pass

    def close(self) -> None:
        try:
            if self._f:
                try:
                    self._f.flush()
                except Exception:
                    pass
                self._f.close()
        except Exception:
            pass
        finally:
            self._f = None
        try:
            if self._sock:
                try:
                    self._sock.shutdown(socket.SHUT_RDWR)
                except Exception:
                    pass
                self._sock.close()
        except Exception:
            pass
        finally:
            self._sock = None
            self._connected = False

class ClientWorker(QObject):
    sig_connected = Signal()
    sig_disconnected = Signal(str)
    sig_log = Signal(str)
    sig_hand = Signal(list)
    sig_table_positions = Signal(object)
    sig_seat_names = Signal(object)
    sig_your_turn = Signal(bool)
    sig_game_end = Signal()
    sig_prompt = Signal(str)
    sig_trump = Signal(object)
    sig_scores = Signal(int, int)
    sig_gscores = Signal(int, int)
    sig_team_id = Signal(int)
    sig_alert = Signal(str)
    sig_player_id = Signal(int)

    def __init__(self, parent: Optional[QObject] = None):
        super().__init__(parent)
        self._adapter: Optional[ProtocolAdapter] = None
        self._thread_should_stop = False
        self._lock = threading.Lock()
        self._input_event = threading.Event()
        self._input_val: Optional[str] = None

    @Slot(object, str, int)
    def start_client(self, adapter: ProtocolAdapter, host: str, port: int):
        self._adapter = adapter

        def cb_log(msg: str): self.sig_log.emit(msg)
        def cb_connected(): self.sig_connected.emit()
        def cb_disconnected(reason: str): self.sig_disconnected.emit(reason)
        def cb_hand(cards: List[str]): self.sig_hand.emit(cards)
        def cb_table_linear(cards: List[str]): pass
        def cb_your_turn(flag: bool): self.sig_your_turn.emit(flag)
        def cb_game_end(): self.sig_game_end.emit()
        def cb_positions(pos: Dict[str, Optional[str]]): self.sig_table_positions.emit(pos)
        def cb_seat_names(nm: Dict[str, str]): self.sig_seat_names.emit(nm)
        def cb_trump(suit: Optional[str]): self.sig_trump.emit(suit)
        def cb_scores(us: int, opp: int): self.sig_scores.emit(us, opp)
        def cb_gscores(us: int, opp: int): self.sig_gscores.emit(us, opp)
        def cb_team(tid: int): self.sig_team_id.emit(tid)
        def cb_alert(text: str): self.sig_alert.emit(text)
        def cb_pid(pid: int): self.sig_player_id.emit(pid)

        try:
            if hasattr(adapter, "set_input_provider"):
                adapter.set_input_provider(self._wait_for_input)
            if hasattr(adapter, "set_suit_callbacks"):
                adapter.set_suit_callbacks(cb_trump)
            if hasattr(adapter, "set_table_positions_callback"):
                adapter.set_table_positions_callback(cb_positions)
            if hasattr(adapter, "set_seat_names_callback"):
                adapter.set_seat_names_callback(cb_seat_names)
            if hasattr(adapter, "set_score_callbacks"):
                adapter.set_score_callbacks(cb_scores, cb_gscores)
            if hasattr(adapter, "set_team_callback"):
                adapter.set_team_callback(cb_team)
            if hasattr(adapter, "set_alert_callback"):
                adapter.set_alert_callback(cb_alert)
            if hasattr(adapter, "set_player_id_callback"):
                adapter.set_player_id_callback(cb_pid)

            adapter.set_callbacks(
                on_log=cb_log,
                on_connected=cb_connected,
                on_disconnected=cb_disconnected,
                on_hand=cb_hand,
                on_table_linear=cb_table_linear,
                on_your_turn=cb_your_turn,
                on_game_end=cb_game_end,
            )
            with self._lock:
                self._thread_should_stop = False
            adapter.connect(host, port, 0)
            adapter.run(lambda: self._thread_should_stop)
        except Exception as e:
            self.sig_disconnected.emit(str(e))

    def _wait_for_input(self, prompt: str, max_len: int = 20) -> str:
        self._input_event.clear()
        self._input_val = None
        self.sig_prompt.emit(prompt)
        while not self._input_event.wait(timeout=0.1):
            with self._lock:
                if self._thread_should_stop:
                    self.sig_prompt.emit("")
                    return ""
        val = self._input_val or ""
        self.sig_prompt.emit("")
        return val[:max_len]

    @Slot(str)
    def provide_input(self, text: str):
        self._input_val = text or ""
        self._input_event.set()

    @Slot()
    def stop(self):
        with self._lock:
            self._thread_should_stop = True
        try:
            if self._adapter:
                self._adapter.close()
        except Exception:
            pass

    @Slot(str)
    def play_card(self, code: str):
        try:
            if self._adapter:
                self._adapter.play_card(code)
        except Exception as e:
            self.sig_log.emit(f"Failed to play {code}: {e}")

class SuitPicker(QWidget):
    suitChosen = Signal(str)

    def __init__(self, parent: Optional[QWidget] = None):
        super().__init__(parent)
        row = QHBoxLayout(self)
        row.setContentsMargins(0, 0, 0, 0)
        row.setSpacing(12)
        self._buttons: Dict[str, QPushButton] = {}
        for s in SUITS:
            btn = QPushButton(SUIT_SYMBOL[s])
            btn.setCursor(Qt.PointingHandCursor)
            btn.setFixedSize(56, 40)
            col = SUIT_COLOR[s]
            btn.setStyleSheet(
                "QPushButton { background-color: #FFFFFF; color: %s; "
                "border: 2px solid #90A4AE; border-radius: 6px; font-weight: bold; font-size: 18px; }"
                "QPushButton:hover { background-color: #FAFAFA; }"
                "QPushButton:pressed { background-color: #F0F0F0; }" % col.name()
            )
            btn.clicked.connect(lambda _, k=s: self.suitChosen.emit(k))
            row.addWidget(btn)
            self._buttons[s] = btn

class MainWindow(QMainWindow):
    sig_start = Signal(object, str, int)
    sig_stop = Signal()
    sig_play = Signal(str)
    sig_input = Signal(str)

    def __init__(self, default_host_port: Optional[str] = None):
        super().__init__()
        self.setWindowTitle("Hokm GUI Client")
        self.resize(1000, 800)

        central = QWidget(self)
        root = QHBoxLayout(central)
        root.setContentsMargins(8, 8, 8, 8)
        root.setSpacing(8)
        self.setCentralWidget(central)

        left = QWidget(self)
        left.setObjectName("LeftPane")
        left.setStyleSheet("""
            QWidget#LeftPane { background-color: #000000; border-radius: 8px; }
            QLabel { color: #EEEEEE; font-size: 12px; }
            QLineEdit { background-color: #111111; color: #E0E0E0; border: 1px solid #333; border-radius: 4px; padding: 4px 6px; }
            QPushButton#ConnectBtn { background-color: #263238; color: #ECEFF1; border: 1px solid #455A64; border-radius: 6px; padding: 6px 10px; font-weight: bold; }
            QPushButton#ConnectBtn:disabled { color: #90A4AE; border-color: #37474F; background-color: #1C262B; }
            QListWidget { background-color: #0F1318; color: #E0E0E0; border: 1px solid #1F242A; padding: 6px; }
        """)
        left_layout = QVBoxLayout(left)
        left_layout.setContentsMargins(12, 12, 12, 12)
        left_layout.setSpacing(10)

        self.host_edit = QLineEdit(self)
        self.host_edit.setPlaceholderText("Host or host:port")
        self.host_edit.setText(default_host_port if default_host_port else "localhost")

        try:
            default_name = getpass.getuser() or ""
        except Exception:
            default_name = ""
        self.name_edit = QLineEdit(self)
        self.name_edit.setPlaceholderText("Name")
        self.name_edit.setText(default_name)

        self.btn_connect = QPushButton("Connect", self)
        self.btn_connect.setObjectName("ConnectBtn")

        left_layout.addWidget(QLabel("Host:"))
        left_layout.addWidget(self.host_edit)
        left_layout.addWidget(QLabel("Name:"))
        left_layout.addWidget(self.name_edit)
        left_layout.addSpacing(6)
        left_layout.addWidget(self.btn_connect)

        left_layout.addSpacing(14)
        self.lbl_gscores = ScoreLabel("Game Scores")
        self.lbl_gscores.setFixedHeight(36)
        left_layout.addWidget(self.lbl_gscores)
        left_layout.addSpacing(4)
        self.lbl_scores = ScoreLabel("Hand Scores")
        self.lbl_scores.setFixedHeight(36)
        left_layout.addWidget(self.lbl_scores)

        left_layout.addSpacing(10)
        lbl_info = QLabel("Info")
        fi = lbl_info.font(); fi.setPointSize(11); fi.setBold(True)
        lbl_info.setFont(fi)
        left_layout.addWidget(lbl_info)
        self.info_list = QListWidget(self)
        self._setup_fixed_height_rows(self.info_list, rows=5, scrollable=False)
        left_layout.addWidget(self.info_list)

        left_layout.addSpacing(6)
        lbl_last = QLabel("Last table")
        fl = lbl_last.font(); fl.setPointSize(11); fl.setBold(True)
        lbl_last.setFont(fl)
        left_layout.addWidget(lbl_last)
        self.last_table_list = QListWidget(self)
        self._setup_fixed_height_rows(self.last_table_list, rows=5, scrollable=False)
        left_layout.addWidget(self.last_table_list)

        left_layout.addStretch(1)

        middle = QWidget(self)
        mid_layout = QVBoxLayout(middle)
        mid_layout.setContentsMargins(12, 12, 12, 12)
        mid_layout.setSpacing(8)

        self.trump_icon = TrumpIcon(self)
        wrap_trump = QWidget(self)
        wrap_trump_layout = QHBoxLayout(wrap_trump)
        wrap_trump_layout.setContentsMargins(0, 0, 0, 0)
        wrap_trump_layout.addStretch(1)
        wrap_trump_layout.addWidget(self.trump_icon)
        wrap_trump_layout.addStretch(1)
        mid_layout.addWidget(wrap_trump)

        self.winner_badge = WinnerBadge(self)
        mid_layout.addWidget(self.winner_badge)

        self.table = TableArea(self)
        mid_layout.addWidget(self.table, stretch=5)

        self.lbl_turn = QLabel("Waiting...")
        f = self.lbl_turn.font(); f.setPointSize(14); f.setBold(True)
        self.lbl_turn.setFont(f); self.lbl_turn.setAlignment(Qt.AlignCenter)
        mid_layout.addWidget(self.lbl_turn)

        prompt_row = QHBoxLayout()
        self.lbl_prompt = QLabel("")
        self.edit_prompt = QLineEdit(self); self.edit_prompt.setPlaceholderText("Type response and press Enter"); self.edit_prompt.setMaxLength(16)
        self.btn_submit = QPushButton("Submit")
        prompt_row.addWidget(self.lbl_prompt, stretch=0)
        prompt_row.addWidget(self.edit_prompt, stretch=1)
        prompt_row.addWidget(self.btn_submit, stretch=0)
        self.prompt_wrap = QWidget(self); self.prompt_wrap.setLayout(prompt_row); self.prompt_wrap.setVisible(False)
        mid_layout.addWidget(self.prompt_wrap)

        sp_row = QHBoxLayout()
        self.lbl_trump_prompt = QLabel("")
        self.suit_picker = SuitPicker(self)
        sp_row.addStretch(1)
        sp_col = QVBoxLayout()
        sp_col.addWidget(self.lbl_trump_prompt, 0, Qt.AlignHCenter)
        sp_col.addWidget(self.suit_picker, 0, Qt.AlignHCenter)
        sp_row.addLayout(sp_col)
        sp_row.addStretch(1)
        self.suit_picker_wrap = QWidget(self); self.suit_picker_wrap.setLayout(sp_row); self.suit_picker_wrap.setVisible(False)
        mid_layout.addWidget(self.suit_picker_wrap)

        self.hand = FlowHand(self)
        mid_layout.addWidget(self.hand, stretch=2)

        left.setFixedWidth(360)
        root.addWidget(left, 0)
        root.addWidget(middle, 1)

        self.status = QStatusBar(self); self.setStatusBar(self.status)

        self.thread = QThread(self)
        self.worker = ClientWorker()
        self.worker.moveToThread(self.thread)

        self.sig_start.connect(self.worker.start_client)
        self.sig_play.connect(self.worker.play_card)
        self.sig_input.connect(self.worker.provide_input, Qt.DirectConnection)
        self.sig_stop.connect(self.worker.stop, Qt.DirectConnection)

        self.worker.sig_log.connect(self._log)
        self.worker.sig_connected.connect(self._on_connected)
        self.worker.sig_disconnected.connect(self._on_disconnected)
        self.worker.sig_hand.connect(self._on_hand)
        self.worker.sig_table_positions.connect(self._on_table_positions)
        self.worker.sig_seat_names.connect(self._on_seat_names)
        self.worker.sig_your_turn.connect(self._on_your_turn)
        self.worker.sig_game_end.connect(self._on_game_end)
        self.worker.sig_prompt.connect(self._on_prompt)
        self.worker.sig_trump.connect(self._on_trump)
        self.worker.sig_scores.connect(self._on_scores)
        self.worker.sig_gscores.connect(self._on_gscores)
        self.worker.sig_team_id.connect(self._on_team_id)
        self.worker.sig_alert.connect(self._on_alert)
        self.worker.sig_player_id.connect(self._on_player_id)

        self.btn_connect.clicked.connect(self._on_connect_clicked)
        self.hand.playRequested.connect(self._on_play_requested)
        self.btn_submit.clicked.connect(self._on_submit_prompt)
        self.edit_prompt.returnPressed.connect(self._on_submit_prompt)
        self.suit_picker.suitChosen.connect(self._on_trump_chosen)

        self.thread.start()

        self._prompt_visible = False
        self._inp_pending = False
        self._pending_is_card = False
        self._trump_picker_active = False

        self._trump_suit: Optional[str] = None
        self._last_scores: Tuple[int, int] = (0, 0)
        self._last_gscores: Tuple[int, int] = (0, 0)
        self._my_team_id: Optional[int] = None
        self._game_winner_shown = False

        self._is_your_turn = False
        self._last_alert_text: str = ""

        # Seat names (now driven from /TBL)
        self._player_id: Optional[int] = None
        self._player_names: Dict[str, str] = {"u": "You", "a": "Right", "b": "Left", "c": "Across"}

        self._prev_table: Dict[str, Optional[str]] = {"u": None, "c": None, "a": None, "b": None}
        self._current_trick: List[Tuple[str, str]] = []
        self._last_trick: List[Tuple[str, str]] = []
        self._last_trick_winner_line: str = ""
        self._trick_full: bool = False
        self._refresh_last_table_view()

        self._round_clear_timer = QTimer(self)
        self._round_clear_timer.setSingleShot(True)
        self._round_clear_timer.timeout.connect(lambda: self.winner_badge.clear_text())

    def _setup_fixed_height_rows(self, lw: QListWidget, rows: int, scrollable: bool):
        lw.setUniformItemSizes(True)
        lw.setSelectionMode(QListWidget.NoSelection)
        lw.setFocusPolicy(Qt.NoFocus)
        fm = lw.fontMetrics()
        row_h = fm.height() + 6
        lw.setFixedHeight(rows * row_h + 8)
        lw.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded if scrollable else Qt.ScrollBarAlwaysOff)
        lw.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

    def _parse_host_port(self, text: str) -> Tuple[str, int]:
        s = (text or "").strip()
        if not s:
            return "localhost", DEFAULT_PORT
        if ':' in s and s.count(':') == 1:
            host, p = s.split(':', 1)
            try:
                port = int(p)
            except ValueError:
                port = DEFAULT_PORT
            return host or "localhost", port
        return s, DEFAULT_PORT

    def _is_card_prompt(self, prompt: str) -> bool:
        p = (prompt or "").lower()
        return any(k in p for k in ("card", "play", "select", "lead"))

    def _is_trump_prompt(self, prompt: str) -> bool:
        p = (prompt or "").lower()
        return any(k in p for k in ("trump", "hokm", "suit", "call"))

    def _is_name_prompt(self, prompt: str) -> bool:
        p = (prompt or "").lower()
        return any(k in p for k in ("name", "username"))

    def _reset_ui(self):
        self.table.clear()
        self.hand.setCards([])
        self.hand.clearSelection()
        self.hand.setTurnActive(False)
        self._on_trump(None)
        self._on_scores(0, 0)
        self._on_gscores(0, 0)
        self._last_alert_text = ""
        self._is_your_turn = False
        self._update_status_label()
        self._show_prompt(False, "")
        self.suit_picker_wrap.setVisible(False)
        self.lbl_trump_prompt.setText("")
        self._inp_pending = False
        self._pending_is_card = False
        self._trump_picker_active = False
        self._game_winner_shown = False
        self.winner_badge.clear_text()
        self._round_clear_timer.stop()
        self._player_names = {"u": "You", "a": "Right", "b": "Left", "c": "Across"}
        self.table.setPlayerNames(self._player_names)
        self._prev_table = {"u": None, "c": None, "a": None, "b": None}
        self._current_trick = []
        self._last_trick = []
        self._last_trick_winner_line = ""
        self._trick_full = False
        self._refresh_last_table_view()
        self.info_list.clear()

    def _announce_round_winner(self, team_id: int, round_num: Optional[int]):
        if self._my_team_id is None:
            return
        you = (team_id == self._my_team_id)
        who = "you" if you else "opp"
        self.winner_badge.show_round(who)
        self._round_clear_timer.start(3000)

    def _announce_game_winner(self, team_id: Optional[int]):
        if self._game_winner_shown:
            return
        who = None
        if team_id is not None and self._my_team_id is not None:
            who = "you" if team_id == self._my_team_id else "opp"
        else:
            g_us, g_opp = self._last_gscores
            s_us, s_opp = self._last_scores
            if g_us != g_opp:
                who = "you" if g_us > g_opp else "opp"
            elif s_us != s_opp:
                who = "you" if s_us > s_opp else "opp"
        if who:
            self._round_clear_timer.stop()
            self.winner_badge.show_game(who)
            self._game_winner_shown = True

    def _append_info(self, text: str, color: Optional[QColor] = None):
        if not text:
            return
        item = QListWidgetItem(text)
        if color:
            item.setForeground(color)
        self.info_list.addItem(item)
        while self.info_list.count() > 5:
            self.info_list.takeItem(0)

    def _seat_name(self, k: str) -> str:
        return self._player_names.get(k, k)

    def _refresh_last_table_view(self):
        self.last_table_list.clear()
        for k, code in self._last_trick[:4]:
            self.last_table_list.addItem(f"{self._seat_name(k)}: {card_to_human(code)}")
        for _ in range(4 - min(4, len(self._last_trick))):
            self.last_table_list.addItem("")
        self.last_table_list.addItem(self._last_trick_winner_line if self._last_trick_winner_line else "")

    def _update_status_label(self):
        self.lbl_turn.setText(self._last_alert_text)

    def _compare_cards(self, code1: str, code2: str, led_suit: Optional[str], trump_suit: Optional[str]) -> int:
        c1 = Card.from_code(code1)
        c2 = Card.from_code(code2)
        ts = trump_suit
        ls = led_suit
        if ts and c1.suit == ts and c2.suit != ts:
            return 1
        if ts and c2.suit == ts and c1.suit != ts:
            return -1
        if c1.suit == c2.suit:
            r1 = RANKS.index(c1.rank)
            r2 = RANKS.index(c2.rank)
            return 1 if r1 > r2 else (-1 if r1 < r2 else 0)
        if ls:
            if c1.suit == ls and c2.suit != ls:
                return 1
            if c2.suit == ls and c1.suit != ls:
                return -1
        return 0

    def _compute_last_trick_winner_line(self):
        self._last_trick_winner_line = ""
        if len(self._last_trick) < 4:
            return
        led_code = self._last_trick[0][1]
        led_suit = Card.from_code(led_code).suit if normalize_card_code(led_code) else None
        trump_suit = self._trump_suit

        best_idx = 0
        for i in range(1, 4):
            if self._compare_cards(self._last_trick[i][1], self._last_trick[best_idx][1], led_suit, trump_suit) > 0:
                best_idx = i
        winner_seat_key = self._last_trick[best_idx][0]

        # Determine team by seats (U & C) vs (A & B)
        if winner_seat_key in ("u", "c"):
            team_seats = ("u", "c")
        else:
            team_seats = ("a", "b")
        names = [self._player_names.get(s, s) for s in team_seats]
        if len(names) == 2:
            self._last_trick_winner_line = f"Winners: {names[0]} & {names[1]}"

    def closeEvent(self, event) -> None:
        try:
            self.sig_stop.emit()
        except Exception:
            pass
        self.thread.quit()
        self.thread.wait(1500)
        super().closeEvent(event)

    def _on_connect_clicked(self):
        self._reset_ui()
        host_text = self.host_edit.text()
        host, port = self._parse_host_port(host_text)

        adapter: ProtocolAdapter = HokmAdapter()
        if hasattr(adapter, "set_suit_callbacks"):
            adapter.set_suit_callbacks(lambda s: self.worker.sig_trump.emit(s))
        if hasattr(adapter, "set_table_positions_callback"):
            adapter.set_table_positions_callback(lambda pos: self.worker.sig_table_positions.emit(pos))
        if hasattr(adapter, "set_seat_names_callback"):
            adapter.set_seat_names_callback(lambda nm: self.worker.sig_seat_names.emit(nm))
        if hasattr(adapter, "set_score_callbacks"):
            adapter.set_score_callbacks(lambda us, opp: self.worker.sig_scores.emit(us, opp),
                                        lambda us, opp: self.worker.sig_gscores.emit(us, opp))
        if hasattr(adapter, "set_team_callback"):
            adapter.set_team_callback(lambda tid: self.worker.sig_team_id.emit(tid))
        if hasattr(adapter, "set_alert_callback"):
            adapter.set_alert_callback(lambda t: self.worker.sig_alert.emit(t))
        if hasattr(adapter, "set_player_id_callback"):
            adapter.set_player_id_callback(lambda pid: self.worker.sig_player_id.emit(pid))

        self.btn_connect.setEnabled(False)
        self.host_edit.setEnabled(False)
        self.name_edit.setEnabled(False)

        self._append_info(f"Connecting to {host}:{port} ...")
        self.sig_start.emit(adapter, host, port)

    def _on_player_id(self, pid: int):
        self._player_id = pid

    def _on_team_id(self, tid: int):
        self._my_team_id = tid

    def _on_play_requested(self, code: str):
        if self._inp_pending and self._pending_is_card and not self._trump_picker_active:
            self.sig_input.emit(code)
            self._inp_pending = False
            self._pending_is_card = False
            self._show_prompt(False, "")
        else:
            self._append_info("(hint) Wait for play prompt from server.")

    def _on_submit_prompt(self):
        if not self._prompt_visible:
            return
        txt = self.edit_prompt.text()
        self.sig_input.emit(txt)

    def _on_trump_chosen(self, suit: str):
        self.sig_input.emit(suit)
        self._trump_picker_active = False
        self.suit_picker_wrap.setVisible(False)
        self.lbl_trump_prompt.setText("")

    def _on_connected(self):
        self.status.showMessage("Connected")
        self._append_info("Connected")

    def _on_disconnected(self, reason: str):
        self.status.showMessage(f"Disconnected: {reason}")
        self._append_info(f"Disconnected: {reason}")
        self.btn_connect.setEnabled(True)
        self.host_edit.setEnabled(True)
        self.name_edit.setEnabled(True)
        self._last_alert_text = ""
        self._is_your_turn = False
        self._update_status_label()
        self._show_prompt(False, "")
        self.suit_picker_wrap.setVisible(False)
        self._trump_picker_active = False
        self.hand.setInteractive(False)
        self.hand.clearSelection()
        self.hand.setTurnActive(False)
        self.table.clear()
        self._on_trump(None)
        self._inp_pending = False
        self._pending_is_card = False
        self.winner_badge.clear_text()
        self._round_clear_timer.stop()
        self._current_trick.clear()
        self._last_trick.clear()
        self._last_trick_winner_line = ""
        self._prev_table = {"u": None, "c": None, "a": None, "b": None}
        self._trick_full = False
        self._refresh_last_table_view()

    def _on_hand(self, cards: List[str]):
        self.hand.setCards(cards)

    def _on_seat_names(self, names: Dict[str, str]):
        # Update labels using names derived from /TBL
        merged = dict(self._player_names)
        merged.update(names or {})
        self._player_names = {
            "u": merged.get("u", "You"),
            "a": merged.get("a", "Right"),
            "b": merged.get("b", "Left"),
            "c": merged.get("c", "Across"),
        }
        self.table.setPlayerNames(self._player_names)
        self._refresh_last_table_view()

    def _on_table_positions(self, positions: Dict[str, Optional[str]]):
        self.table.setPositions(positions)

        curr: Dict[str, Optional[str]] = {
            k: normalize_card_code(positions.get(k) or "") for k in ("u", "c", "a", "b")
        }

        for k in ("u", "a", "c", "b"):
            if self._prev_table.get(k) is None and curr.get(k) is not None:
                self._current_trick.append((k, curr[k]))  # type: ignore

        non_none_now = sum(1 for v in curr.values() if v is not None)
        if non_none_now == 4:
            self._trick_full = True

        non_none_prev = sum(1 for v in self._prev_table.values() if v is not None)
        if self._trick_full and non_none_now == 0 and non_none_prev > 0:
            self._last_trick = list(self._current_trick)
            self._current_trick = []
            self._trick_full = False
            self._compute_last_trick_winner_line()
            self._refresh_last_table_view()

        if non_none_prev == 0 and non_none_now == 0:
            self._current_trick = []

        self._prev_table = curr

    def _on_your_turn(self, flag: bool):
        self._is_your_turn = flag
        self.hand.setTurnActive(flag and not self._trump_picker_active)
        self.hand.setInteractive(flag and not self._trump_picker_active)
        if not self._last_alert_text:
            self._update_status_label()

    def _on_game_end(self):
        self._append_info("Game ended")
        self._last_alert_text = ""
        self._update_status_label()
        self.hand.setTurnActive(False)
        self.hand.setInteractive(False)
        self._announce_game_winner(team_id=None)
        self.btn_connect.setEnabled(True)
        self.host_edit.setEnabled(True)
        self.name_edit.setEnabled(True)

    def _on_alert(self, text: str):
        t = (text or "").strip()
        self._last_alert_text = t
        self._update_status_label()
        lower = t.lower()
        m = re.search(r'round\s*\d+[^a-z0-9]+winner\s*team[:\s]+(\d+)', lower)
        if m:
            try:
                team_id = int(m.group(1))
                self._announce_round_winner(team_id, round_num=None)
            except Exception:
                pass
        mg = re.search(r'team\s+(\d+)\s+is\s+the\s+winner\s+of\s+the\s+game', lower)
        if mg:
            try:
                team_id = int(mg.group(1))
                self._announce_game_winner(team_id)
            except Exception:
                pass
        if t == "":
            self._update_status_label()

    def _on_prompt(self, prompt: str):
        if prompt:
            self._inp_pending = True
            if self._is_name_prompt(prompt):
                name = (self.name_edit.text() or "").strip() or getpass.getuser() or "You"
                self.sig_input.emit(name)
                self._inp_pending = False
                self._pending_is_card = False
                self._show_prompt(False, "")
                return
            if self._is_trump_prompt(prompt):
                self._trump_picker_active = True
                self.hand.setInteractive(False)
                self.hand.clearSelection()
                self.hand.setTurnActive(False)
                self.lbl_trump_prompt.setText(prompt)
                self.suit_picker_wrap.setVisible(True)
                self._show_prompt(False, "")
                return
            if self._is_card_prompt(prompt):
                self._pending_is_card = True
                self._show_prompt(False, "")
                return
            self._pending_is_card = False
            self._show_prompt(True, prompt)
        else:
            self._inp_pending = False
            self._pending_is_card = False
            self._trump_picker_active = False
            self._show_prompt(False, "")
            self.suit_picker_wrap.setVisible(False)
            self.lbl_trump_prompt.setText("")
            self.hand.setTurnActive(False)

    def _show_prompt(self, visible: bool, text: str):
        self._prompt_visible = visible
        self.prompt_wrap.setVisible(visible)
        self.lbl_prompt.setText(text)
        if visible:
            self.edit_prompt.clear()
            self.edit_prompt.setFocus()

    def _on_trump(self, suit: Optional[str]):
        self._trump_suit = suit
        self.trump_icon.setSuit(suit)

    def _on_scores(self, us: int, opp: int):
        self._last_scores = (us, opp)
        self.lbl_scores.setScores(us, opp)

    def _on_gscores(self, us: int, opp: int):
        self._last_gscores = (us, opp)
        self.lbl_gscores.setScores(us, opp)

    def _log(self, msg: str):
        raw = (msg or "").strip()
        if raw.startswith("/INF"):
            inf_text = raw[4:]
            self._append_info(inf_text)
        else:
            if raw:
                self._append_info(raw)

def main():
    parser = argparse.ArgumentParser(description="Hokm GUI Client")
    parser.add_argument("host", nargs="?", help="Default host or host:port to prefill in the UI")
    parser.add_argument("-H", "--host", dest="host_opt", help="Default host or host:port to prefill in the UI")
    args = parser.parse_args()
    default_host_port = args.host_opt or args.host

    QGuiApplication.setHighDpiScaleFactorRoundingPolicy(Qt.HighDpiScaleFactorRoundingPolicy.PassThrough)
    app = QApplication(sys.argv)
    w = MainWindow(default_host_port=default_host_port)
    w.show()
    return app.exec()

if __name__ == "__main__":
    sys.exit(main())
