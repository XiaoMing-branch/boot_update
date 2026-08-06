#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ymodem_upload_gui.py — YMODEM 串口固件升级上位机(GUI 版,双击即用)

用法:
    python tool/ymodem_upload_gui.py
打包(生成单文件 exe):
    pyinstaller --onefile --noconsole --name ymodem_upload_gui ymodem_upload_gui.py
"""

import os
import queue
import sys
import threading
import tkinter as tk
from tkinter import ttk, filedialog, scrolledtext

import serial
import serial.tools.list_ports

import ymodem_upload as y


class GuiApp:
    def __init__(self, root):
        self.root = root
        root.title("STM32 YMODEM 串口固件升级")
        root.geometry("640x480")
        root.minsize(560, 400)

        self.q = queue.Queue()

        # ---------- 串口 / 波特率 ----------
        frm = ttk.Frame(root, padding=10)
        frm.pack(fill=tk.X)
        ttk.Label(frm, text="串口:").grid(row=0, column=0, sticky=tk.W)
        self.port_var = tk.StringVar()
        self.port_cb = ttk.Combobox(frm, textvariable=self.port_var, width=12)
        self.port_cb.grid(row=0, column=1, sticky=tk.W, padx=4)
        self._refresh_ports()
        ttk.Button(frm, text="刷新", command=self._refresh_ports, width=6).grid(row=0, column=2)

        ttk.Label(frm, text="波特率:").grid(row=0, column=3, padx=(14, 0), sticky=tk.W)
        self.baud_var = tk.StringVar(value="115200")
        ttk.Combobox(frm, textvariable=self.baud_var, width=9,
                     values=["9600", "19200", "38400", "57600",
                             "115200", "230400", "460800", "921600"]).grid(row=0, column=4, padx=4)

        # ---------- 固件文件 ----------
        frm2 = ttk.Frame(root, padding=(10, 0))
        frm2.pack(fill=tk.X)
        ttk.Label(frm2, text="固件:").pack(side=tk.LEFT)
        self.file_var = tk.StringVar()
        ttk.Entry(frm2, textvariable=self.file_var).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=4)
        ttk.Button(frm2, text="浏览...", command=self._pick_file).pack(side=tk.LEFT)

        # ---------- 发送 / 状态 ----------
        frm3 = ttk.Frame(root, padding=10)
        frm3.pack(fill=tk.X)
        self.send_btn = ttk.Button(frm3, text="发送固件", command=self._on_send)
        self.send_btn.pack(side=tk.LEFT)
        self.status_var = tk.StringVar(value="就绪")
        ttk.Label(frm3, textvariable=self.status_var).pack(side=tk.LEFT, padx=12)

        # ---------- 进度条 ----------
        self.pb = ttk.Progressbar(root, maximum=100, value=0)
        self.pb.pack(fill=tk.X, padx=10)

        # ---------- 日志 ----------
        self.txt = scrolledtext.ScrolledText(root, height=16)
        self.txt.pack(fill=tk.BOTH, expand=True, padx=10, pady=(10, 10))
        self._log("1. 选择串口与固件文件;2. 复位目标板;3. 点击\"发送固件\"。")

        root.protocol("WM_DELETE_WINDOW", self._on_close)
        self._poll()

    # ---------- 界面辅助 ----------
    def _refresh_ports(self):
        self.port_cb["values"] = [p.device for p in serial.tools.list_ports.comports()]

    def _pick_file(self):
        path = filedialog.askopenfilename(title="选择固件文件",
                                          filetypes=[("固件文件", "*.bin"), ("所有文件", "*.*")])
        if path:
            self.file_var.set(path)

    def _log(self, msg):
        self.q.put(str(msg))

    def _poll(self):
        try:
            while True:
                self.txt.insert(tk.END, self.q.get_nowait() + "\n")
                self.txt.see(tk.END)
        except queue.Empty:
            pass
        self.root.after(80, self._poll)

    def _on_close(self):
        self.root.destroy()

    # ---------- 发送 ----------
    def _on_send(self):
        port = self.port_var.get().strip()
        path = self.file_var.get().strip()
        try:
            baud = int(self.baud_var.get())
        except ValueError:
            self._log("错误: 波特率无效。")
            return
        if not port:
            self._log("错误: 请选择串口。")
            return
        if not os.path.isfile(path):
            self._log("错误: 固件文件不存在: {}".format(path))
            return

        self.send_btn.config(state=tk.DISABLED)
        self.status_var.set("传输中...")
        self.pb["value"] = 0
        threading.Thread(target=self._transfer, args=(port, baud, path), daemon=True).start()

    def _transfer(self, port, baud, path):
        ser = None
        try:
            with open(path, "rb") as f:
                data = f.read()
            if not data:
                self._log("错误: 固件文件为空。")
                return
            filename = os.path.basename(path)
            self._log("打开 {} @ {} ...".format(port, baud))
            ser = serial.Serial(port, baud, timeout=0.2)
            ok = y.upload(ser, filename, data, log=self._log, progress=self._on_progress)
            if ok:
                self._log("传输完成! Boot 校验通过后将自动跳转 APP。")
            else:
                self._log("传输失败。")
        except Exception as e:
            self._log("错误: {}".format(e))
        finally:
            if ser:
                ser.close()
            self.root.after(0, self._on_done)

    def _on_progress(self, sent, size):
        self.root.after(0, lambda: self.pb.configure(value=sent * 100.0 / size))

    def _on_done(self):
        self.send_btn.config(state=tk.NORMAL)
        self.status_var.set("就绪")


def main():
    root = tk.Tk()
    GuiApp(root)
    root.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
