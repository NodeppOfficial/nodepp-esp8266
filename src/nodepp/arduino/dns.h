/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOfficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_POSIX_DNS
#define NODEPP_POSIX_DNS

/*────────────────────────────────────────────────────────────────────────────*/

#include "lwip/dns.h"

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace dns {
    
    inline bool is_ipv4( const string_t& URL ){ 
    thread_local static regex_t reg ( "([0-9]+\\.)+[0-9]+" );
        reg.clear_memory(); return reg.test( URL ) ? 1 : 0; 
    }

    inline bool is_ipv6( const string_t& URL ){ 
    thread_local static regex_t reg ( "([0-9a-fA-F]+\\:)+[0-9a-fA-F]+" );
        reg.clear_memory(); return reg.test( URL ) ? 1 : 0; 
    }

    /*─······································································─*/

namespace {

    static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
        auto res_ptr = (res_t<string_t>*) callback_arg;

        if (ipaddr != NULL) {
            char ip_str[16];
            ipaddr_ntoa_r(ipaddr, ip_str, sizeof(ip_str));
            (*res_ptr)( string_t(ip_str) ); // Resolvemos la promesa
        } else {
            // Error de DNS (ej. host no encontrado)
            // Aquí deberías manejar el reject de la promesa
        }
        delete res_ptr; // Limpiamos el puntero temporal
    }

}
    
    /*─······································································─*/

    promise_t<string_t, except_t> dns_lookup( string_t host ) {
        return promise_t<string_t, except_t>([=]( res_t<string_t> res, rej_t<except_t> rej ){

            ip_addr_t resolved_ip;
            // Creamos un puntero persistente para el callback
            auto arg = new res_t<string_t>(res);

            // Intentamos resolver
            err_t err = dns_gethostbyname(host.c_str(), &resolved_ip, dns_callback, arg);

            if (err == ERR_OK) {
                // Caso A: Estaba en caché, resolvemos de inmediato
                char ip_str[16];
                ipaddr_ntoa_r(&resolved_ip, ip_str, sizeof(ip_str));
                res( string_t(ip_str) );
                delete arg;
            } else if (err != ERR_INPROGRESS) {
                // Caso B: Error inmediato (ej. DNS no configurado)
                rej( except_t("DNS Lookup Failed") );
                delete arg;
            }
            // Caso C: ERR_INPROGRESS. No hacemos nada, esperamos al callback.
        });
    }
    
    /*─······································································─*/

    inline expected_t<ip_t,except_t> get_host_data(){
        auto socket = socket_t();
            
        socket.SOCK    = SOCK_DGRAM;
        socket.IPPROTO = IPPROTO_UDP;
        socket.socket ( "loopback", 0 );
        socket.connect();

    return socket.get_sockname(); }
    
    /*─······································································─*/

    inline bool is_ip( const string_t& URL ){ 
        if( URL.empty() )/*-*/{ return 0; }
        if( is_ipv4(URL) > 0 ){ return 1; }
        if( is_ipv6(URL) > 0 ){ return 1; } return 0;
    }

}}

/*────────────────────────────────────────────────────────────────────────────*/

#endif

/*────────────────────────────────────────────────────────────────────────────*/