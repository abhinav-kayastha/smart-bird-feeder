//
// Created by Keijo Länsikunnas on 23.9.2024.
//

#ifndef RP2040_FREERTOS_IRQ_LWIPOPTS_TLS_H
#define RP2040_FREERTOS_IRQ_LWIPOPTS_TLS_H

/* TCP WND must be at least 16 kb to match TLS record size
   or you will get a warning "altcp_tls: TCP_WND is smaller than the RX decrypion buffer, connection RX might stall!" */
#undef TCP_WND
#define TCP_WND  16384

#define LWIP_ALTCP               1
#define LWIP_ALTCP_TLS           1
#define LWIP_ALTCP_TLS_MBEDTLS   1

#define LWIP_DEBUG 1
#define ALTCP_MBEDTLS_DEBUG  LWIP_DBG_ON

#endif //RP2040_FREERTOS_IRQ_LWIPOPTS_TLS_H
