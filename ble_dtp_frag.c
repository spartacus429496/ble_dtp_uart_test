#include "ble_dtp_frag.h"
#include <stdio.h>
#include <string.h>

static
unsigned char __need_fragment(unsigned short len);

static
void __send_direct(m_buf_t *p_mbuf,
        unsigned char *p_data, unsigned short len);

static
void __do_fragment(m_buf_t *p_mbuf,
        unsigned char *p_data, unsigned short len);

static
void __encode_one_unit(ble_dtp_unit_t *p_unit, \
        unsigned char *p_data, unsigned short len, \
        unsigned char more_frag, unsigned char frag_enable, \
        unsigned short offset, unsigned short total_len);

//static
void __do_send(unsigned char *p_data, unsigned short len);
static
void __lowlevel_driver_tx(unsigned char *p_data, unsigned short len);

static
void __mbuf_dump(m_buf_t *p_mbuf);


static
unsigned short __do_reasm(m_buf_t *p_mbuf, unsigned char *p_out);
static
unsigned char __need_reasm(unsigned char *p_data);
static
ble_dtp_state_t  __do_defrag(unsigned char *p_data, unsigned short len);
ble_dtp_instance_t ble_dtp_txer = {0};
ble_dtp_instance_t ble_dtp_rxer = {0};

void ble_dtp_init(ble_dtp_instance_t *p_er)
{
    p_er->state = IDLE;
    //p_er->mbufs
}

void ble_dtp_destroy(ble_dtp_instance_t *p_er)
{
    p_er->state = IDLE;
    //p_er->mbufs
}

//API
/*
 * in: p_data
 * internal: m_buf
 * out:
 */
void ble_dtp_send(unsigned char *p_data, unsigned short len)
{
    m_buf_t *p_mbuf = &(ble_dtp_txer.mbufs);
    if(__need_fragment(len)) {
        //m_buf_t *p_mbuf = &(ble_dtp_txer.mbufs);
        __do_fragment(p_mbuf, p_data, len);

    } else {
        //send_direct()
        __send_direct(p_mbuf, p_data, len);
    }
}

static
unsigned char __need_fragment(unsigned short len)
{
    //(len>PAYLOAD_LEN)?return 1:return 0;
    if(len>PAYLOAD_LEN) {
        return 1;
    } else {
        return 0;
    }
}
static
void __do_fragment(m_buf_t *p_mbuf,
        unsigned char *p_data, unsigned short len)
{
    unsigned char entries = 0;
    unsigned short offset = 0;
    unsigned short total_len = MTU;//len;
    do {
        __encode_one_unit(&(p_mbuf->unit[entries]), \
        p_data + entries*PAYLOAD_LEN, PAYLOAD_LEN, \
        1,1, offset, total_len);
        len -= PAYLOAD_LEN;
        offset+= PAYLOAD_LEN;
        entries++;
        p_mbuf->entry_num++;
    } while (len>PAYLOAD_LEN);

        //offset -= PAYLOAD_LEN;
        //offset += len;
        total_len = len+HDR_LEN;
        __encode_one_unit(&(p_mbuf->unit[entries]), \
        p_data + entries*PAYLOAD_LEN, len, \
        0,1, offset, total_len);
        //no more fragment
        p_mbuf->entry_num++;

}
static
void __send_direct(m_buf_t *p_mbuf,
        unsigned char *p_data, unsigned short len)
{
        __encode_one_unit(&(p_mbuf->unit[0]), \
        p_data, len, \
        0,0,0, len+HDR_LEN);
        p_mbuf->entry_num=1;

}

static
void __encode_one_unit(ble_dtp_unit_t *p_unit, \
        unsigned char *p_data, unsigned short len, \
        unsigned char more_frag, unsigned char frag_enable, \
        unsigned short offset, unsigned short total_len)
{
    if (frag_enable) {
        p_unit->hdr.flag |= 0x2;
    } else {
        p_unit->hdr.flag &= ~0x2;
    }
    if (more_frag) {
        p_unit->hdr.flag |= 0x1;
    } else {
        p_unit->hdr.flag &= ~0x1;
    }
    p_unit->hdr.hdr_len = HDR_LEN;
    p_unit->hdr.total_len = total_len;
    p_unit->hdr.id = 0xff;
    p_unit->hdr.frag_offset = offset;
    //p_unit->crc_chk_sum = 0;

    memcpy(p_unit->payload, p_data, len);
    //just for test
    __do_send((unsigned char *)p_unit, len+p_unit->hdr.hdr_len);
}
void mbuf_dump(void)
{
    m_buf_t *p_mbuf = &(ble_dtp_txer.mbufs);
    __mbuf_dump(p_mbuf);

}
static
void __mbuf_dump(m_buf_t *p_mbuf)
{
    int i = 0;
    ble_dtp_unit_t *p_unit = (ble_dtp_unit_t *)p_mbuf->unit[0];
    for (i=0;i<p_mbuf->entry_num;i++) {
        p_unit = (ble_dtp_unit_t *)p_mbuf->unit[i];
        __do_send((unsigned char *)p_unit, p_unit->hdr.total_len);
    }
}

unsigned short packets_unit_generator_and_recv(m_buf_t *p_mbuf, unsigned char *p_out)
{
    int i = 0;
    unsigned short len_sum = 0;
    ble_dtp_unit_t *p_unit = (ble_dtp_unit_t *)p_mbuf->unit[0];
    for (i=0;i<p_mbuf->entry_num;i++) {
        p_unit = (ble_dtp_unit_t *)p_mbuf->unit[i];

        len_sum += ble_dtp_recv( (unsigned char *)p_unit, p_unit->hdr.total_len,
                p_out);
    }
    return len_sum;
}

//static
void __do_send(unsigned char *p_data, unsigned short len)
{
    //encoder one unit
    //driver to send
    __lowlevel_driver_tx(p_data, len);
}

static
void __lowlevel_driver_tx(unsigned char *p_data, unsigned short len)
{
    unsigned short i=0;
    for (i=0;i<len;i++) {
#ifdef DEBUG_LOG_EN
        printf("%2x,", *(p_data+i));
        //printf("%c", *(p_data+i));
#endif
    }
    printf("\n");
}

/*
 * brief: this function will process one unit each calling.
 * in: multi segments units
 * internal: m_buf
 * out:
 */
unsigned short ble_dtp_recv(unsigned char *p_data, unsigned short len,\
        unsigned char *p_out)
{
    m_buf_t *p_mbuf = &(ble_dtp_rxer.mbufs);
    unsigned short ret_len = 0;
    ble_dtp_state_t *p_state;
    p_state = &ble_dtp_rxer.state;

    if(__need_reasm(p_data)) {
        __do_defrag(p_data, len);
        if (*p_state == COMPLETE) {
            ret_len = __do_reasm(p_mbuf,p_out);
        }

    } else {
        //recv_direct()
    }
    return ret_len;
}

static
unsigned char __need_reasm(unsigned char *p_data)
{
    ble_dtp_unit_t *p_unit = CAST_TO_BLE_DTP_UNIT(p_data);
    if(p_unit->hdr.flag & 0x02) {
        return 1;
    } else {
        return 0;
    }
}

static
ble_dtp_state_t  __do_defrag(unsigned char *p_data, unsigned short len)
{
    m_buf_t *p_mbuf = &(ble_dtp_rxer.mbufs);
    ble_dtp_state_t *p_state;//state machine
    p_state = &ble_dtp_rxer.state;

    //if rx_mbuf is avilaible
    //copy data to mbuf
    unsigned short entries = p_mbuf->entry_num;
    unsigned char *dst = p_mbuf->unit[entries];

    ble_dtp_unit_t *p_unit = CAST_TO_BLE_DTP_UNIT(p_data);

    switch (*p_state) {
        case IDLE:
            p_mbuf->entry_num=0;
            if ((p_unit->hdr.flag)& 0x02 && (p_unit->hdr.flag&0x01)) {
                entries = p_mbuf->entry_num;
                dst = p_mbuf->unit[entries];
                memcpy(dst, p_data, len);
                p_mbuf->entry_num++;
                *p_state = RECEIVING;

            }
            break;
        case START:
            break;
        case RECEIVING:
            if ((p_unit->hdr.flag)& 0x02 ) {//frag enable
                entries = p_mbuf->entry_num;
                dst = p_mbuf->unit[entries];
                memcpy(dst, p_data, len);
                p_mbuf->entry_num++;
                if ((p_unit->hdr.flag&0x01)) {//more
                    *p_state = RECEIVING;
                } else {//no more, collection over
                    *p_state = COMPLETE;
                }
            }
            break;
        case COMPLETE:
                //error
            break;
        default:
            break;
    }
    return *p_state;
}

static
unsigned short __do_reasm(m_buf_t *p_mbuf, unsigned char *p_out)
{
    unsigned short i = 0;
    unsigned short len_sum = 0;
    unsigned short len = 0;
    unsigned short offset = 0;
    for (i=0;i< p_mbuf->entry_num;i++) {

        ble_dtp_unit_t *p_unit = CAST_TO_BLE_DTP_UNIT(p_mbuf->unit[i]);
        offset = p_unit->hdr.frag_offset;
        len = p_unit->hdr.total_len - HDR_LEN;
        len_sum+=len;
        memcpy(p_out+offset, p_unit->payload, len);

    }
    //check len_sum and offset
    if (len_sum != (offset+len)) {
        //recv checking error
#ifdef DEBUG_LOG_EN
        printf("recving length checking error!len_sum:%d, \
                offset:%d,\
                len:%d,\
                \n",\
                len_sum, offset, len);
#endif
        len_sum = 0;
        goto err;
    }

    __do_send((unsigned char *)p_out, len_sum);
err:
    //have read out, set state to idle
    //ble_dtp_state_t *p_state = &ble_dtp_rxer.state;
    //*p_state = IDLE;
    ble_dtp_rxer.state = IDLE;
    return len_sum;

}


