/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOfficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_POSIX_KERNEL
#define NODEPP_POSIX_KERNEL
#define NODEPP_POLL_NPOLL
#endif

/*────────────────────────────────────────────────────────────────────────────*/

#ifdef NODEPP_POLL_NPOLL

namespace nodepp { class kernel_t {
private:

    enum FLAG { 
         KV_STATE_UNKNOWN = 0b00000000, 
         KV_STATE_WRITE   = 0b00000001,
         KV_STATE_READ    = 0b00000010,
         KV_STATE_EDGE    = 0b10000000,
         KV_STATE_USED    = 0b00000100,
         KV_STATE_AWAIT   = 0b00001100,
         KV_STATE_CLOSED  = 0b00001000
    };

    struct kevent_t { public:
        function_t<int> callback;
        ulong timeout; int fd, flag; 
    };

    /*─······································································─*/

    int get_delay_ms() const noexcept {
        ulong tasks= obj->ev_queue.size();
    return ( tasks==0 ) ? -1 : TIMEOUT; }

protected:

    struct NODE { loop_t ev_queue; }; ptr_t<NODE> obj;

public:

    kernel_t() noexcept : obj( new NODE() ) {}

public:

    void off( ptr_t<task_t> address ) const noexcept { clear( address ); }

    void clear( ptr_t<task_t> address ) const noexcept {
         if( address.null() ) /*--------------*/ { return; }
         if( address->flag & TASK_STATE::CLOSED ){ return; }
             address->flag = TASK_STATE::CLOSED;
    }

    /*─······································································─*/

    ulong size() const noexcept { return obj->ev_queue.size() + obj.count()-1; }

    void clear() const noexcept { /*--*/ obj->ev_queue.clear(); }
    
    bool* should_close() const noexcept { return &SHOULD_CLOSE(); }

    bool empty() const noexcept { return size()==0; }

    int   emit() const noexcept { return -1; }

    /*─······································································─*/

    template< class T, class U, class... W >
    ptr_t<task_t> poll_add ( T str, int /*unused*/, U cb, ulong timeout=0, const W&... args ) noexcept {

        auto time = type::bind( timeout>0 ? timeout + process::now() : timeout );
        auto clb  = type::bind( cb ); 
        
        return obj->ev_queue.add( coroutine::add( COROUTINE(){
        coBegin 

            if( *time > 0 && *time < process::now() ){ coEnd; }
            coSet(0); return (*clb)( args... ) >= 0 ? 1 : -1; 

        coFinish
        }));

    }

    /*─······································································─*/

    inline int next() const { 
        obj->ev_queue.next(); process::yield(); 
    return 1; }

    /*─······································································─*/

    template< class T, class... V > 
    int await( T cb, const V&... args ) const { 
    int c=0;
        if ((c =cb(args...))>=0 ){ next(); return 1; } 
    return -1; }

    template< class T, class... V >
    ptr_t<task_t> loop_add ( T cb, const V&... args ) noexcept {
        return obj->ev_queue.add( cb, args... );
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

#endif

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_ARDUINO_LWIP
#define NODEPP_ARDUINO_LWIP

/*────────────────────────────────────────────────────────────────────────────*/

#include "lwip/netdb.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/raw.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/*────────────────────────────────────────────────────────────────────────────*/

enum   SOCKET_TYPE { SOCK_STREAM, SOCK_DGRAM, SOCK_RAW };
struct SOCKADDR    { ip_addr_t* addr; int port; }
using  SOCKET      = void*;

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class lwip_t {
protected:

    static void udp_recv_fn( void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port ){
        if( arg == nullptr ){ pbuf_free( p ); return; }
        auto tmp = (DONE*) arg; pbuf* x = p;
        auto stm = string_t( p->tot_len );
        auto off = 0UL;

        while( x != nullptr ) { pbuf* y = x->next;
            memcpy( stm.get() + off, x->payload, x->len );
        off += x->len; x = y; }

        tmp->que.push( stm ); pbuf_free( p ); 
    }

    static void raw_recv_fn( void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr ){
        if( arg == nullptr ){ pbuf_free( p ); return; }
        auto tmp = (DONE*) arg; pbuf* x = p;
        auto stm = string_t( p->tot_len );
        auto off = 0UL;

        while( x != nullptr ) { pbuf* y = x->next;
            memcpy( stm.get() + off, x->payload, x->len );
        off += x->len; x = y; }

        tmp->que.push( stm ); pbuf_free( p ); 
    }

protected:

    enum MODE {
         LWIP_MODE_UDP = 0b0000001,
         LWIP_MODE_TCP = 0b0000010,
         LWIP_MODE_RAW = 0b0000100
    };

    using TCPPCB = struct tcp_pcb;
    using UDPPCB = struct udp_pcb;
    using RAWPCB = struct raw_pcb;

    struct DONE { 
        queue_t<string_t>que;
        SOCKADDR   sock_addr;
        void* addr = nullptr;
        void* sign = nullptr;
        int   mode = 0x00   ;
    };

    struct NODE {
        ptr_t <void*> sock_que;
        queue_t<DONE> sock_ctx; 
    };  ptr_t<NODE> obj;

public:

    lwip_t() : obj( new NODE ) {
        obj->socket_que.resize( MAX_SOCKET );
        obj->socket_que.fill  ( nullptr    );
    }

   ~lwip() noexcept { if( obj.count()>0 ){ return; } free(); }

    /*─······································································─*/

    void free() const noexcept { for( int x=0; x<MAX_SOCKET; x++ ){ 
         /*close_socket(x);*/ remove_socket(x);
    }}

    /*─······································································─*/

    int create_socket( uchar type ) const noexcept {
        if( obj->sock_ctx.size() >= MAX_SOCKET )
          { return -1; } int tmp  = -1;

        for( int x=0; x < MAX_SOCKET; x++ ){
        if ( sock_que[x] == nullptr ){ tmp=x; break; }}
        if ( tmp == -1 ){ return -1; }

        sock_ctx.push( DONE( nullptr, &obj, tmp ));
        sock_que[tmp] = (void*) sock_ctx.last();
        auto addr     = /*---*/ sock_ctx.last();

        switch( type ){
            case SOCK_STREAM:
                addr->mode |= MODE::LWIP_MODE_TCP;
                addr->addr  = (void*) tcp_new();
            break;
            case SOCK_DGRAM:
                addr->mode |= MODE::LWIP_MODE_UDP;
                addr->addr  = (void*) udp_new();
                udp_recv( (UDPPCB) addr, udp_recv_clb, (void*) addr );
            break;
            case SOCK_RAW:
                addr->mode |= MODE::LWIP_MODE_RAW;
                addr->addr  = (void*) raw_new( 0x00 );
            break;
        }

    }

    /*─······································································─*/

    int close_socket ( int fd ) const noexcept {
        if( fd < 0 || fd > MAX_SOCKET )   { return -1; }
        if( obj->sock_que[fd] == nullptr ){ return -1; }

        auto tmp = obj->sock_ctx.as( obj->sock_que[fd] );
        if( tmp->sign != &obj ){ return -1; }

        if( tmp->mode & LWIP_MODE_RAW ){
            raw_disconnect( (RAWPCB*) tmp->addr );
        }

        elif( tmp_mode & LWIP_MODE_UDP ){
            raw_disconnect( (UDPPCB*) tmp->addr );
        }

        elif( tmp->mode & LWIP_MODE_TCP ){
            tcp_arg ( (TCPPCB*) tmp->addr, nullptr );
            tcp_sent( (TCPPCB*) tmp->addr, nullptr );
            tcp_recv( (TCPPCB*) tmp->addr, nullptr );
            tcp_shutdown( (TCPPCB*) tmp->addr, 1,1 );
        }

    }

    /*─······································································─*/

    int send_socket( int fd, const char* data, ulong len ) const noexcept {

        if( fd < 0 || fd >= MAX_SOCKET ){ return -1; }
        if( obj->sock_que[fd]==nullptr ){ return -1; }

        auto tmp = (DONE*) obj->sock_que[fd];
        if( tmp->sign != &obj ) /*----*/ { return -1; }
        if( data == nullptr || len == 0 ){ return  0; }

    if( tmp->mode & LWIP_MODE_TCP ){

        TCPPCB* pcb = (TCPPCB*) tmp->addr;
            
        u16_t space = tcp_sndbuf( pcb );
        if( space < len ){ len=space; }
        if( len  ==   0 ){ return -2; }

        err_t err = tcp_write( pcb, data, (u16_t)len, TCP_WRITE_FLAG_COPY );
        if( err == ERR_MEM ){ return -2; }
        if( err != ERR_OK  ){ return -1; } // tcp_output( pcb ); 
        
        /**/ return (int) len;
    } else { return -1; } }

    /*─······································································─*/

    int sendto_socket( int fd, const char* data, ulong len, SOCKADDR* dst ) const noexcept {

        if( fd < 0 || fd >= MAX_SOCKET ){ return -1; }
        if( obj->sock_que[fd]==nullptr ){ return -1; }

        auto tmp = (DONE*) obj->sock_que[fd];
        if( tmp->sign != &obj ) /*----*/ { return -1; }
        if( data == nullptr || len == 0 ){ return  0; }

    if(( tmp->mode & LWIP_MODE_RAW ) ||  
       ( tmp->mode & LWIP_MODE_UDP )
    ) {

        struct pbuf* p = pbuf_alloc( PBUF_TRANSPORT, len, PBUF_RAM );
        if( p == nullptr ){ return -2; }

        memcpy( p->payload, data, len ); err_t err;

        if( tmp->mode & MODE::LWIP_MODE_UDP ) {
        auto pcb = (UDPPCB*) tmp->addr;
             err = ( dst != nullptr ) ? udp_sendto( pcb, p, dst->addr, dst->port ) 
                                      : udp_send  ( pcb, p );
        } else { 
        auto pcb = (RAWPCB*) tmp->addr;
             err = ( dst != nullptr ) ? raw_sendto( pcb, p, dst->addr ) 
                                      : raw_send  ( pcb, p );
        }

        pbuf_free( p ); 
        
        if( err == ERR_MEM ){ return -2; }
        if( err != ERR_OK  ){ return -1; }

        /**/ return (int) len;
    } else { return -1; } }

    /*─······································································─*/

    int remove_socket( int fd ) const noexcept {
        if( fd < 0 || fd >= MAX_SOCKET ){ return -1; }
        if( obj->sock_que[fd]==nullptr ){ return -1; }

        auto tmp = obj->sock_ctx.as( obj->sock_que[fd] );
        if( tmp->sign != &obj ){ return -1; }

        if( tmp->mode & LWIP_MODE_RAW ){
            raw_remove( (RAWPCB*) tmp->addr );
        }

        elif( tmp->mode & LWIP_MODE_UDP ){
            udp_remove( (UDPPCB*) tmp->addr );
        }

        elif( tmp->mode & LWIP_MODE_TCP ){
            tcp_abort ( (TCPPCB*) tmp->addr );
        }

        tmp->sign = nullptr; 
        tmp->addr = nullptr;
        tmp->mode = 0x00   ;
        obj->sock_ctx.erase( tmp );
        obj->sock_que[fd] = nullptr;

    return 1; }

}}

#endif

/*────────────────────────────────────────────────────────────────────────────*/