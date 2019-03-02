
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "hbto_uart.h"
#include <stdio.h>
#include <string.h>


hbto_uart_obj_t hbto_uart_txer = {0};
hbto_uart_obj_t hbto_uart_rxer = {0};
s_buf_vector_t sbuf_tx = {0};



static
void __set_sbuf(unsigned char *p_dst, hbto_uart_meta_t *p_meta);

static
void __driver_uart_tx(unsigned char *p_data, unsigned short len);
//API
/*
 * in: p_data
 * internal: s_buf
 * out:
 */
void hbto_uart_send(unsigned char *p_data, unsigned short len)
{
    //encode
    hbto_uart_meta_t *p_meta = &hbto_uart_txer.meta;
    s_buf_vector_t *p_sbuf_tx = &sbuf_tx;
    p_meta->hdr.head1 = 0xa5;
    p_meta->hdr.head2 = 0x5a;
    p_meta->hdr.head3 = '#';
    p_meta->hdr.payload_len = len;
    p_meta->hdr.id = 0xffff;

    p_meta->p_payload = p_data;
    p_meta->crc_chk_sum = 0;
    p_meta->tail = 0xAA;

    __set_sbuf(p_sbuf_tx->stream, p_meta);

    //send
    __driver_uart_tx(p_sbuf_tx->stream, p_meta->hdr.payload_len + 9+2);

}
static
void __driver_uart_tx(unsigned char *p_data, unsigned short len)
{
    uint32_t err_code = 0;
    for (uint32_t i = 0; i < len; i++)
    {
        do
        {
            err_code = app_uart_put(p_data[i]);
            if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
            {
                NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
                APP_ERROR_CHECK(err_code);
            }
        } while (err_code == NRF_ERROR_BUSY);
    }
}
static
void __set_sbuf(unsigned char *p_dst, hbto_uart_meta_t *p_meta)
{
    unsigned char *p_src= (unsigned char *)p_meta;
    memcpy(p_dst, p_src, HBTO_HDR_LEN);
    memcpy(p_dst+HBTO_HDR_LEN, p_meta->p_payload,\
            p_meta->hdr.payload_len);
    p_dst += (HBTO_HDR_LEN + p_meta->hdr.payload_len);
    *p_dst++ = p_meta->crc_chk_sum;
    *p_dst++ = p_meta->tail;
}

/* a receiever fsm
 * in: data: 1 Byte
 * internal: s_buf
 * out:
 */
//void hbto_uart_recv(unsigned char *p_data, unsigned short len);
unsigned short hbto_uart_recv(unsigned char *p_data, unsigned short len, \
        unsigned char *p_out)
{
    unsigned char tmp_char = *p_data;
    hbto_uart_meta_t *p_meta = &hbto_uart_rxer.meta;
    hbto_uart_state_t *p_state = &hbto_uart_rxer.state;
    static unsigned short cnt = 0;
    static unsigned short len_sum = 0;
    static unsigned short len_payload = 0;
    switch(*p_state) {
        case  HBTO_IDLE:
            if((tmp_char == HAED1_MAGIC) && (cnt == 0)) {
                //*(p_out+len_sum) = tmp_char;
                *p_state = HBTO_HAED_S;
                cnt = 1;
                len_sum++;
            }
            break;
        case  HBTO_START:
            break;
        case  HBTO_HAED_S:
            if((tmp_char == HAED2_MAGIC) && (1 == cnt)) {
                //*(p_out+len_sum) = tmp_char;
                len_sum++;
                *p_state = HBTO_HAED_S;
                cnt++;
            } else if((tmp_char == HAED3_MAGIC) && (2 == cnt)) {
                *p_state = HBTO_LEN_S;
                cnt = 0;
            } else {
                goto err;
            }
            break;
        case  HBTO_LEN_S:
            if((0 == cnt)) {
                p_meta->hdr.payload_len = tmp_char;
#ifdef DEBUG_LOG_EN
                printf("payload len 1:%x\n", tmp_char);
#endif
                len_sum++;
                cnt++;
            } else if((1 == cnt)) {
#ifdef DEBUG_LOG_EN
                printf("payload len 2:%x\n", tmp_char);
#endif
                //p_meta->hdr.payload_len *=(0xff+1);
                p_meta->hdr.payload_len += tmp_char*(0xff+1);;
                cnt = 0;
                len_sum++;
                *p_state = HBTO_ID_S;

            } else {
                goto err;
            }

            break;
        case  HBTO_ID_S:
            if((0 == cnt)) {
                p_meta->hdr.id= tmp_char;
                len_sum++;
                cnt++;
            } else if((1 == cnt)) {
                p_meta->hdr.id*=(0xff+1);
                p_meta->hdr.id+= tmp_char;
                cnt = 0;
                len_sum++;
                *p_state = HBTO_RESERVE_S;

            } else {
                goto err;
            }


            break;
        case  HBTO_RESERVE_S:
            if((0 == cnt)) {
                p_meta->hdr.reserve= tmp_char;
                len_sum++;
                cnt++;
            } else if((1 == cnt)) {
                //p_meta->hdr.id*=(0xff+1);
                //p_meta->hdr.id+= tmp_char;
                cnt = 0;
                len_sum++;
                *p_state = HBTO_PAYLOAD_RECEIVING;

            } else {
                goto err;
            }
            break;
        case  HBTO_SENDING:
            break;
        case  HBTO_PAYLOAD_RECEIVING:
            if (cnt < p_meta->hdr.payload_len  ) {
                *(p_out+cnt) = tmp_char;
                len_sum++;
                cnt++;
#ifdef DEBUG_X
                printf("parsing receiving:%c, payload len:%d,\
                        cnt:%d\n",
                        tmp_char,  p_meta->hdr.payload_len, \
                        cnt);
#endif
            } else {
                //*p_state = HBTO_CHK_SUM_S;//this byte is chk_sum
                //save chk_sum
                *p_state = HBTO_TAIL_S;
                cnt = 0;
            }

            break;
        case  HBTO_CHK_SUM_S:
                *p_state = HBTO_TAIL_S;
            break;
        case  HBTO_TAIL_S:
            if (0xAA == tmp_char) {
                *p_state = HBTO_COMPLETE;
            } else {
#ifdef DEBUG_LOG_EN
                printf("tail:0x%2x\n", tmp_char);
#endif
                goto err;
            }

            break;
        case  HBTO_COMPLETE:
            break;
        default:
            break;
    }
    return p_meta->hdr.payload_len;
err:
#ifdef DEBUG_LOG_EN
    printf("recv parsing error: state:%d\n", *p_state);
#endif
    *p_state = HBTO_IDLE;
    return 0;
}
