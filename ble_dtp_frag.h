/*
 *	Definitions for the 'ble data transmission' protocol.
 *	aka ble_dtp (self defined)
 *
 *	Authors:
 *
 */

#ifndef _BLE_DTP_BUF_H
#define _BLE_DTP_BUF_H
#ifndef DEBUG_LOG_EN
//#define DEBUG_LOG_EN 1
#endif

struct dtp_buff_head {
    unsigned char reserved;
};

//#define MTU 255//20
#define MTU 20
#define MAX_ENTRIES (1024/MTU+1)
//based on vector
typedef struct m_buff {
    unsigned char unit[MAX_ENTRIES][MTU];
    unsigned short entry_num;

}m_buf_t;
#define HDR_LEN 9
#define PAYLOAD_LEN (MTU-HDR_LEN) //sizeof(ble_dtp_hdr)
typedef struct __attribute__((__packed__)) ble_dtp_unit{

    struct __attribute__((__packed__)) ble_dtp_hdr {
        unsigned char flag;
        unsigned char hdr_len;
        unsigned short total_len;
        unsigned short id;
        unsigned short frag_offset;
        unsigned char crc_chk_sum;
    }hdr;
    unsigned char payload[PAYLOAD_LEN];
} ble_dtp_unit_t;

typedef enum ble_dtp_state {
    IDLE=0,
    START,
    SENDING,
    RECEIVING,
    COMPLETE,
} ble_dtp_state_t;

//class
typedef struct ble_dtp_instance {
    ble_dtp_state_t state;
    m_buf_t mbufs;
    //operations
    /*
     *init
     *destroy
     *send
     *recv
     */

}ble_dtp_instance_t;

#define CAST_TO_BLE_DTP_UNIT(p_data) \
    (ble_dtp_unit_t *)p_data
    //do {(ble_dtp_unit_t *)p_data } while(0)
extern ble_dtp_instance_t ble_dtp_txer;
extern ble_dtp_instance_t ble_dtp_rxer;
void ble_dtp_init(ble_dtp_instance_t *p_er);
void ble_dtp_destroy(ble_dtp_instance_t *p_er);
//void mbuf_dump(m_buf_t *p_mbuf);
void mbuf_dump(void);
void __do_send(unsigned char *p_data, unsigned short len);
unsigned short packets_unit_generator_and_recv(m_buf_t *p_mbuf, unsigned char *p_out);


//API
/*
 * in: p_data
 * internal: m_buf
 * out:
 */
void ble_dtp_send(unsigned char *p_data, unsigned short len);

/*
 * in: data
 * internal: m_buf
 * out:
 */
//void ble_dtp_recv(unsigned char *p_data, unsigned short len);
unsigned short ble_dtp_recv(unsigned char *p_data, unsigned short len, \
        unsigned char *p_out);

#endif	/* _BLE_DTP_BUF_H */
