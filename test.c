
#include "ip_frag.h"
#include "hbto_uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char test_str[] =
" !\"#$%&'()*+,-./0123456789:;<=>?\
@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\
`abcdefghijklmnopqrstuvwxyz{|}~";
#if 0
"I have known adventures, seen places you people will never see, I've been Offworld and back...frontiers! I've stood on the back deck of a blinker bound for the Plutition Camps with sweat in my eyes watching the stars fight on the shoulder of Orion. I've felt wind in my hair, riding test boats off the black galaxies and seen an attack fleet burn like a match and disappear. I've seen it...felt it!";
"Definitions for the struct sk_buff memory handlers\
 	Authors:\
 		Alan Cox, <gw4pts@gw4pts.ampr.org>\
 		Florian La Roche, <rzsfl@rz.uni-sb.de>\
 ";
#endif
unsigned short uart_test(unsigned char *p_in, unsigned short len, \
        unsigned char *p_out);
unsigned short ble_test(unsigned char *p_in, unsigned short len, \
        unsigned char *p_out);
#define MAX_UNIT_SIZE 64

int main(int argc, char ** argv)
{
    unsigned char *p_in = test_str;
    unsigned short len_in = sizeof(test_str);
    (len_in > MAX_UNIT_SIZE)?len_in=MAX_UNIT_SIZE:(void)len_in;
    unsigned char  uart_recved_buf[1024] = {0};
    unsigned char *p_out = uart_recved_buf;
    unsigned short uart_recved_len = 0;
    uart_recved_len = uart_test(p_in, len_in, p_out);

    p_in = p_out;
    len_in = uart_recved_len;
    unsigned char p_ble_recv_buf[1024] = {0};
    p_out = p_ble_recv_buf;
    unsigned short ble_recved_len = 0;
    ble_recved_len = ble_test(p_in, len_in, p_out);

    //uart resend
    p_in = p_out;
    len_in = ble_recved_len;
    p_out = uart_recved_buf;
    uart_recved_len = uart_test(p_in, len_in, p_out);


}

unsigned short uart_test(unsigned char *p_in, unsigned short len, \
        unsigned char *p_out)
{
    printf(">>>>Uart sending!\n");
    hbto_uart_send(p_in, len);
    unsigned char array_uart_out[1024] = {0};
    hbto_uart_meta_t *p_meta = &hbto_uart_txer.meta;
    hbto_uart_state_t *p_state = &hbto_uart_rxer.state;
    s_buf_vector_t *p_sbuf_tx = &sbuf_tx;//
    unsigned char *p_data_uart_rx = p_sbuf_tx->stream;
    unsigned short uart_len = p_meta->hdr.payload_len + 9+2;
    unsigned short uart_recved_len = 0;
    //unsigned char *p_out = uart_recved_buf;

    /*
unsigned short hbto_uart_recv(unsigned char *p_data, unsigned short len, \
        unsigned char *p_out)

        */

    printf(">>>>Uart receiving!\n");
    int j = 0;
    do {
        uart_recved_len = hbto_uart_recv(p_data_uart_rx+j, 0, p_out);
        j++;

    }while(HBTO_COMPLETE != (*p_state));
    printf("uart_decoding:\n uart_len:%d\n",\
            uart_len);
    for (int i=0;i<uart_len;i++) {
        //uart_recved_len = hbto_uart_recv(p_data_uart_rx+j+i, 0, p_out);
    }

    printf("uart_decoded:\n %s\n", p_out);
    return uart_recved_len;


}

unsigned short ble_test(unsigned char *p_in, unsigned short len, \
        unsigned char *p_out)
{

    printf(">>>>BLE sending!\n");
    unsigned char unit_size = sizeof(ble_dtp_unit_t);
    //unsigned short len = sizeof(test_str);
    printf("unit size:%d:\n", unit_size);
    printf("raw data:(total_len:0x%x, %d)\n,%s\n",
            len, len, p_in);
    printf("after encoder:\n");
    ble_dtp_send(p_in, len);
    mbuf_dump();


    unsigned short i = 0;
    //unsigned char p_recv_buf[1024] = {0};
    //input m_buf
    m_buf_t *p_mbuf = &(ble_dtp_txer.mbufs);
    printf(">>>>BLE receiving!\n");
    return packets_unit_generator_and_recv(p_mbuf, p_out);
}
