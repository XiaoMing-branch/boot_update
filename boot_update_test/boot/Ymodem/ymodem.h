#ifndef __YMODEM_H__
#define __YMODEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* YMODEM 协议控制字符 */
#define YMODEM_SOH          0x01    /* 128 字节数据块起始 */
#define YMODEM_STX          0x02    /* 1024 字节数据块起始 */
#define YMODEM_EOT          0x04    /* 传输结束 */
#define YMODEM_ACK          0x06    /* 正确应答 */
#define YMODEM_NAK          0x15    /* 错误应答,请求重发 */
#define YMODEM_CAN          0x18    /* 取消传输 */
#define YMODEM_CRC          0x43    /* 'C' 请求 CRC16 校验模式 */

/* 超时与重试参数 */
#define YMODEM_TIMEOUT      1000U   /* 等待一字节的超时(ms) */
#define YMODEM_MAX_RETRIES  10      /* 请求帧的最大重试次数 */

/* 返回码 */
#define YMODEM_OK               0
#define YMODEM_ERR_TIMEOUT      (-1)
#define YMODEM_ERR_CANCEL       (-2)
#define YMODEM_ERR_CRC          (-3)
#define YMODEM_ERR_SEQ          (-4)
#define YMODEM_ERR_FILESIZE     (-5)
#define YMODEM_ERR_WRITE        (-6)
#define YMODEM_ERR_NOFILE       (-7)
#define YMODEM_ERR_ABORT        (-8)

/* 平台适配回调: 成功返回 0,失败返回非 0 */
typedef int (*ymodem_uart_rx_t)(uint8_t *buf, uint16_t len, uint32_t timeout_ms);
typedef int (*ymodem_uart_tx_t)(const uint8_t *buf, uint16_t len);
typedef int (*ymodem_flash_erase_t)(uint32_t addr, uint32_t size);
typedef int (*ymodem_flash_write_t)(uint32_t addr, const uint8_t *data, uint32_t len);
typedef int (*ymodem_flash_verify_t)(uint32_t addr, const uint8_t *data, uint32_t len);

typedef struct {
    uint32_t flash_addr;                    /* 固件写入起始地址(APP_START_ADDR) */
    uint32_t max_size;                      /* 允许接收的最大固件字节数(APP 区域大小) */
    uint32_t file_size;                     /* 接收完成后固件实际大小(元素数,可读) */
    ymodem_uart_rx_t      uart_rx;
    ymodem_uart_tx_t      uart_tx;
    ymodem_flash_erase_t  flash_erase;
    ymodem_flash_write_t  flash_write;
    ymodem_flash_verify_t flash_verify;
} ymodem_handle_t;

/**
  * @brief  通过 YMODEM 协议接收一个固件并写入 Flash
  * @param  h         协议句柄(回调已绑定)
  * @param  first_byte 调用前已读取到的帧首字符;无则传 0
  * @retval YMODEM_OK / 负值错误码;成功后 h->file_size 为固件字节数
  */
int ymodem_receive(ymodem_handle_t *h, uint8_t first_byte);

#ifdef __cplusplus
}
#endif

#endif
