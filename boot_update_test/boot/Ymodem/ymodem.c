#include "ymodem.h"
#include <string.h>

/* 保证帧缓冲 4 字节对齐,便于以 FlashBandwidthType_t 指针写入 Flash */
#if defined(__CC_ARM)
__align(4)
#elif defined(__GNUC__)
__attribute__((aligned(4)))
#endif
static uint8_t s_frame[1024];

/**
  * @brief  CRC16-XMODEM (poly 0x1021, init 0x0000)
  */
static uint16_t ymodem_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0;
    uint16_t i;
    uint8_t  j;

    for (i = 0; i < len; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (j = 0; j < 8; j++)
        {
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/**
  * @brief  请求下一帧: 可选发送 'C' 并等待帧首字符
  * @param  send_c  非 0: 先发送 'C'(仅在开始与 EOT 后使用)
  * @retval 帧首字符; 超时返回 0
  */
static uint8_t ymodem_wait_head(ymodem_handle_t *h, uint8_t send_c)
{
    uint8_t  ch = 0;
    int      retry;

    for (retry = 0; retry < YMODEM_MAX_RETRIES; retry++)
    {
        if (send_c)
        {
            uint8_t c = YMODEM_CRC;
            h->uart_tx(&c, 1);
        }
        if (h->uart_rx(&ch, 1, YMODEM_TIMEOUT) == 0)
            return ch;
    }
    return 0;
}

/**
  * @brief  读取一帧数据(SOH/STX + seq + ~seq + data + CRC16)
  * @param  first  已读取到的帧首字符
  * @retval YMODEM_OK       成功
  *         YMODEM_EOT      收到 EOT
  *         YMODEM_ERR_CANCEL 收到 CAN
  *         其他负值        错误
  */
static int ymodem_get_frame(ymodem_handle_t *h, uint8_t first,
                            uint8_t *seq_out, uint8_t *data, uint16_t *data_len)
{
    uint8_t  seq, seq_c, crc_hi, crc_lo;
    uint16_t payload;

    if (first == YMODEM_EOT)
        return YMODEM_EOT;
    if (first == YMODEM_CAN)
        return YMODEM_ERR_CANCEL;
    if (first != YMODEM_SOH && first != YMODEM_STX)
        return YMODEM_ERR_ABORT;

    payload = (first == YMODEM_SOH) ? 128U : 1024U;

    if (h->uart_rx(&seq,   1, YMODEM_TIMEOUT) != 0) return YMODEM_ERR_TIMEOUT;
    if (h->uart_rx(&seq_c, 1, YMODEM_TIMEOUT) != 0) return YMODEM_ERR_TIMEOUT;
    if ((uint8_t)(seq + seq_c) != 0xFF)            return YMODEM_ERR_SEQ;
    if (h->uart_rx(data, payload, YMODEM_TIMEOUT) != 0) return YMODEM_ERR_TIMEOUT;
    if (h->uart_rx(&crc_hi, 1, YMODEM_TIMEOUT) != 0) return YMODEM_ERR_TIMEOUT;
    if (h->uart_rx(&crc_lo, 1, YMODEM_TIMEOUT) != 0) return YMODEM_ERR_TIMEOUT;

    if ((uint16_t)((crc_hi << 8) | crc_lo) != ymodem_crc16(data, payload))
        return YMODEM_ERR_CRC;

    *seq_out  = seq;
    *data_len = payload;
    return YMODEM_OK;
}

/**
  * @brief  解析块0(文件名[\0 或 空格] 文件大小),返回固件字节数;无有效大小返回 0
  */
static uint32_t ymodem_parse_header(const uint8_t *data, uint16_t len)
{
    const uint8_t *p = data;
    const uint8_t *end = data + len;
    uint32_t size = 0;

    /* 跳过文件名(文件名以 '\0' 或空格结尾) */
    while (p < end && *p != '\0' && *p != ' ')
        p++;
    /* 跳过分隔符 */
    while (p < end && (*p == '\0' || *p == ' '))
        p++;
    /* 解析文件大小 */
    while (p < end && *p >= '0' && *p <= '9')
    {
        size = size * 10 + (uint32_t)(*p - '0');
        p++;
    }
    return size;
}

int ymodem_receive(ymodem_handle_t *h, uint8_t first_byte)
{
    uint8_t  first = first_byte;
    uint8_t  seq;
    uint16_t pkt_len;
    uint32_t file_size = 0;
    uint32_t received = 0;
    uint8_t  last_seq = 0;
    uint8_t  block0_done = 0;
    uint8_t  data_done = 0;     /* 已收到第一个 EOT */
    uint8_t  ack = YMODEM_ACK;
    uint8_t  nak = YMODEM_NAK;
    int      ret;

    h->file_size = 0;
    if (h->max_size == 0)
        h->max_size = 0xFFFFFFFFUL;

    for (;;)
    {
        /* 获取帧首字符: 开始/收尾阶段发 'C',数据阶段只等待 */
        if (first == 0)
        {
            first = ymodem_wait_head(h, (uint8_t)(!block0_done || data_done));
            if (first == 0)
                return YMODEM_ERR_TIMEOUT;
        }

        ret = ymodem_get_frame(h, first, &seq, s_frame, &pkt_len);
        first = 0;              /* 首字符已被消费 */

        if (ret == YMODEM_EOT)
        {
            /* 数据块结束: ACK 第一个 EOT;再收到 EOT 即确认完成 */
            h->uart_tx(&ack, 1);
            if (!block0_done)
                return YMODEM_ERR_NOFILE;
            if (data_done)
                return (received >= file_size) ? YMODEM_OK : YMODEM_ERR_WRITE;
            data_done = 1;
            continue;
        }
        if (ret == YMODEM_ERR_CANCEL)
            return YMODEM_ERR_CANCEL;
        if (ret == YMODEM_ERR_TIMEOUT)
            return YMODEM_ERR_TIMEOUT;
        if (ret != YMODEM_OK)
        {
            h->uart_tx(&nak, 1);
            continue;
        }

        /* ---------- 块0: 文件名 + 文件大小(仅传输开始时有效) ---------- */
        if (seq == 0 && !block0_done)
        {
            file_size = ymodem_parse_header(s_frame, pkt_len);
            if (file_size == 0)
            {
                /* 未提供有效文件名/大小,直接结束 */
                h->uart_tx(&ack, 1);
                return YMODEM_OK;
            }
            if (file_size > h->max_size)
            {
                h->uart_tx(&nak, 1);
                return YMODEM_ERR_FILESIZE;
            }

            if (h->flash_erase)
            {
                if (h->flash_erase(h->flash_addr, file_size) != 0)
                    return YMODEM_ERR_WRITE;
            }
            block0_done = 1;
            last_seq   = 0;
            received   = 0;
            h->uart_tx(&ack, 1);
            continue;
        }

        /* ---------- 数据块 ---------- */
        if (!block0_done)
        {
            h->uart_tx(&nak, 1);
            continue;
        }

        /* 已收到第一个 EOT 后的收尾帧(空块0 等): 一律视为传输结束 */
        if (data_done)
        {
            h->uart_tx(&ack, 1);
            return YMODEM_OK;
        }

        if (seq == (uint8_t)(last_seq + 1))
        {
            last_seq = seq;
            /* 去掉最后一个块补齐的填充字节 */
            if (received + pkt_len > file_size)
                pkt_len = (uint16_t)(file_size - received);

            if (pkt_len)
            {
                if (h->flash_write(h->flash_addr + received, s_frame, pkt_len) != 0)
                    return YMODEM_ERR_WRITE;
                if (h->flash_verify)
                {
                    if (h->flash_verify(h->flash_addr + received, s_frame, pkt_len) != 0)
                    {
                        h->uart_tx(&nak, 1);
                        return YMODEM_ERR_WRITE;
                    }
                }
                received += pkt_len;
            }
            h->uart_tx(&ack, 1);
            continue;
        }

        if (seq == last_seq)
        {
            /* 重复块: 上次 ACK 丢失,补发 ACK */
            h->uart_tx(&ack, 1);
            continue;
        }

        /* 序列号错误 */
        h->uart_tx(&nak, 1);
    }
}
