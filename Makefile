test_out: ble_dtp_frag.o hbto_uart.o test.c
	        cc -o test_out test.c ble_dtp_frag.o hbto_uart.o

ble_dtp_frag.o : ble_dtp_frag.c ble_dtp_frag.h
	        cc -c ble_dtp_frag.c
hbto_uart.o : hbto_uart.c hbto_uart.h
			cc -c hbto_uart.c
clean :
	        rm test_out ble_dtp_frag.o hbto_uart.o
