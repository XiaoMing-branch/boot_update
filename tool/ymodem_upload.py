#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ymodem_upload.py — 简易 YMODEM 上位机,通过串口向 BootLoader 发送固件。

与 boot 工程 (boot_update_test/boot) 的 YMODEM 接收端配套使用:
    python tool/ymodem_upload.py -p COM5 -f app_update.bin
    python tool/ymodem_upload.py --port COM5 --baud 115200 --file app_update.bin

用法说明:
    1. 目标板上电后 Boot 会在 5 秒内周期发送 'C' 等待升级;
       启动本工具后请在超时前复位目标板(或保持 Boot 处于等待升级状态)。
    2. 传输成功打印 "Transfer complete"。
"""

import argparse
import sys
import time

import serial

# YMODEM 控制字符
SOH = 0x01
STX = 0x02
EOT = 0x04
ACK = 0x06
NAK = 0x15
CAN = 0x18
CRC_CHAR = 0x43          # 'C'

MAX_RETRIES = 10
PAD_CHAR = 0x1A          # 末块补齐字节(Ctrl-Z)


def crc16(data: bytes) -> int:
    """CRC16-XMODEM (poly 0x1021, init 0),与 boot/ymodem.c 一致。"""
    crc = 0
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def wait_byte(ser: serial.Serial, timeout: float):
    """读取一个字节,超时返回 None。"""
    data = ser.read(1)
    return data[0] if data else None


def wait_for_c(ser: serial.Serial, timeout: float) -> bool:
    """等待接收方发送 'C'(忽略其他字节)。"""
    end = time.time() + timeout
    while time.time() < end:
        b = wait_byte(ser, end - time.time())
        if b is None:
            break
        if b == CRC_CHAR:
            return True
    return False


def wait_ack(ser: serial.Serial, timeout: float):
    """等待应答,忽略期间的 'C' 字节。返回 'ack'/'nak'/'can'/None(超时)。"""
    end = time.time() + timeout
    while time.time() < end:
        b = wait_byte(ser, end - time.time())
        if b is None:
            break
        if b == ACK:
            return 'ack'
        if b == NAK:
            return 'nak'
        if b == CAN:
            return 'can'
        # CRC_CHAR ('C') 忽略
    return None


def send_block(ser: serial.Serial, seq: int, payload: bytes, stx: bool) -> None:
    """发送一个数据块(STX=1024 或 SOH=128),含 seq、~seq、CRC16。"""
    head = STX if stx else SOH
    pkt = bytes([head, seq & 0xFF, (~seq) & 0xFF]) + payload
    pkt += crc16(payload).to_bytes(2, 'big')
    ser.write(pkt)


def upload(ser: serial.Serial, filename: str, data: bytes, log=print, progress=None) -> bool:
    """发送固件。log: 步骤/错误消息回调;progress: progress(sent, size) 进度回调(可选)。"""
    size = len(data)

    # ---------- 1. 等待接收方 'C' ----------
    log("[1/4] Waiting for receiver 'C'...")
    if not wait_for_c(ser, 15.0):
        log("ERROR: no 'C' received (超时). 请复位目标板,保持 Boot 等待升级。")
        return False

    # ---------- 2. 块0: 文件名 + 大小 ----------
    log("[2/4] Sending header ({}...)".format(filename))
    header = filename.encode('utf-8') + b'\x00' + str(size).encode('ascii') + b'\x00'
    header = header.ljust(128, b'\x00')          # 补齐到 128 字节
    seq = 0
    ok = False
    for _ in range(MAX_RETRIES):
        send_block(ser, seq, header, stx=False)
        r = wait_ack(ser, 30.0)                  # 需覆盖 Boot 擦除 APP 区的时间
        if r == 'ack':
            ok = True
            break
        if r == 'can':
            log("ERROR: receiver cancelled.")
            return False
    if not ok:
        log("ERROR: header not acknowledged.")
        return False

    # ---------- 3. 数据块 ----------
    log("[3/4] Sending data ({} bytes)...".format(size))
    sent = 0
    seq = 1
    while sent < size:
        chunk = data[sent:sent + 1024]
        chunk = chunk.ljust(1024, bytes([PAD_CHAR]))   # 末块补齐
        ok = False
        for _ in range(MAX_RETRIES):
            send_block(ser, seq, chunk, stx=True)
            r = wait_ack(ser, 3.0)
            if r == 'ack':
                ok = True
                break
            if r == 'can':
                log("ERROR: receiver cancelled.")
                return False
        if not ok:
            log("ERROR: data block not acknowledged (seq={}).".format(seq))
            return False
        sent += len(data[sent:sent + 1024])
        seq = (seq + 1) & 0xFF
        if progress:
            progress(sent, size)
        else:
            print("\r  progress: {:5.1f}%".format(sent * 100.0 / size), end='', flush=True)

    if not progress:
        print("\r  progress: 100.0%")

    # ---------- 4. EOT 收尾 ----------
    log("[4/4] Sending EOT...")
    ser.write(bytes([EOT]))
    if wait_ack(ser, 3.0) != 'ack':
        log("ERROR: EOT not acknowledged.")
        return False
    ser.write(bytes([EOT]))
    if wait_ack(ser, 3.0) != 'ack':
        log("ERROR: final EOT not acknowledged.")
        return False

    return True


def main():
    parser = argparse.ArgumentParser(description='简易 YMODEM 串口固件升级上位机')
    parser.add_argument('-p', '--port', required=True, help='串口号,如 COM5')
    parser.add_argument('-b', '--baud', type=int, default=115200, help='波特率(默认 115200)')
    parser.add_argument('-f', '--file', required=True, help='固件文件路径(如 app_update.bin)')
    args = parser.parse_args()

    try:
        with open(args.file, 'rb') as f:
            data = f.read()
    except OSError as e:
        print("ERROR: cannot open file: {}".format(e))
        return 1

    if not data:
        print("ERROR: empty file.")
        return 1

    filename = args.file.split('\\')[-1].split('/')[-1]

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.2)
    except serial.SerialException as e:
        print("ERROR: cannot open {}: {}".format(args.port, e))
        return 1

    try:
        print("Port: {} @ {}".format(args.port, args.baud))
        print("File: {} ({} bytes)".format(args.file, len(data)))
        if upload(ser, filename, data):
            print("Transfer complete.")
        else:
            print("Transfer FAILED.")
            return 1
    except KeyboardInterrupt:
        print("\nAborted by user.")
        return 1
    finally:
        ser.close()

    return 0


if __name__ == '__main__':
    sys.exit(main())
