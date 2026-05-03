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

namespace nodepp {  struct dns_t { string_t address, hostname; int family; }; }
namespace nodepp { namespace dns {

    inline bool is_ipv6( const string_t& URL ){ 
    thread_local static regex_t reg ( "([0-9a-fA-F]+\\:)+", true );
        reg.clear_memory(); return reg.test( URL ) ? 1 : 0; 
    }
    
    inline bool is_ipv4( const string_t& URL ){ 
    thread_local static regex_t reg ( "([0-9]+\\.)+[0-9]+" );
        reg.clear_memory(); return reg.test( URL ) ? 1 : 0; 
    }
    
    /*─······································································─*/

    inline bool is_ip( const string_t& URL ){ 
        if( URL.empty( ) ){ return 0; }
        if( is_ipv4(URL) ){ return 1; }
        if( is_ipv6(URL) ){ return 1; } return 0;
    }
    
    /*─······································································─*/

    inline ptr_t<dns_t> lookup( string_t host, int family = AF_UNSPEC ) { 
        _socket_::start_device();

        if( family == AF_INET ) {

            if( host == "broadcast" || host == "255.255.255.255" ){
                dns_t tmp ({ "255.255.255.255", host, AF_INET }); 
                return ptr_t<dns_t>( 1UL, tmp );
            }
            elif( host == "localhost" || host == "127.0.0.1" ){ 
                dns_t tmp ({ "127.0.0.1", host, AF_INET }); 
                return ptr_t<dns_t>( 1UL, tmp );
            }
            elif( host == "global"    || host == "0.0.0.0" ){ 
                dns_t tmp ({ "0.0.0.0", host, AF_INET }); 
                return ptr_t<dns_t>( 1UL, tmp );
            }
            elif( host == "loopback"  || host == "1.1.1.1" ){
                dns_t tmp ({ "1.1.1.1", host, AF_INET }); 
                return ptr_t<dns_t>( 1UL, tmp );
            }

        }  elif( family == AF_INET6 )  {

            if( host == "broadcast" || host == "::2" ){
                dns_t tmp ({ "::2", host, AF_INET6 }); 
                return ptr_t<dns_t>( 1UL, tmp );
            }
            elif( host == "localhost" || host == "::1" ){ 
                dns_t tmp ({ "::1", host, AF_INET6 }); 
                return ptr_t<dns_t>( 1UL, tmp );
            }
            elif( host == "global"    || host == "::0" ){ 
                dns_t tmp ({ "::0", host, AF_INET6 }); 
                return ptr_t<dns_t>( 1UL, tmp );
            }
            elif( host == "loopback"  || host == "::3" ){ 
                dns_t tmp ({ "::3", host, AF_INET6 }); 
                return ptr_t<dns_t>( 1UL, tmp );
            }

        }   
        
        addrinfo hints, *res, *ptr; memset( &hints, 0, sizeof(hints) );
        if( url::is_valid(host) ){ host = url::hostname(host); }
        
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family   = family     ;
        hints.ai_flags    = AI_PASSIVE ;

        if( getaddrinfo( host.get(), nullptr, &hints, &res ) != 0 )
          { return nullptr; }

        string_t ipAddress ; char ipstr[INET6_ADDRSTRLEN]; 
        queue_t<dns_t> list;

        for( ptr = res; ptr != nullptr; ptr = ptr->ai_next ) {
             void *addr = nullptr;

            if( ptr->ai_family == AF_INET ) {
                addr = &((struct sockaddr_in*) ptr->ai_addr)->sin_addr;
            } elif ( ptr->ai_family == AF_INET6 ) {
                addr = &((struct sockaddr_in6*)ptr->ai_addr)->sin6_addr;
            }

            if( addr ) {
                inet_ntop( ptr->ai_family, addr, ipstr, sizeof(ipstr) );
                list.push( dns_t({ ipstr, host, ptr->ai_family }) );
                if ( family != AF_UNSPEC ){ break; }
            }

        }

        freeaddrinfo(res); return list.data();
    }
    
    /*─······································································─*/

    inline expected_t<ip_t,except_t> get_host_data(){
        auto socket = socket_t();
            
        socket.SOCK    = SOCK_DGRAM ;
        socket.IPPROTO = IPPROTO_UDP;
        socket.socket ( "loopback", 0 );
        socket.connect();

    return socket.get_sockname(); }

}}

/*────────────────────────────────────────────────────────────────────────────*/

#endif

/*────────────────────────────────────────────────────────────────────────────*/