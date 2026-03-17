/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOfficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_ARDUINO_SOCKET
#define NODEPP_ARDUINO_SOCKET
#define INVALID_SOCKET nullptr

/*────────────────────────────────────────────────────────────────────────────*/

struct SOCKET_CTX  { void* socket_fd; uchar addr=0; };
enum   SOCKET_TYPE { SOCK_STREAM, SOCK_DGRAM };
using  SOCKET      = void*;

/*────────────────────────────────────────────────────────────────────────────*/

#include "lwip/netdb.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp {

struct ip_t    { string_t address; uint port; };
struct agent_t {
    ulong buffer_size   = CHUNK_SIZE;
    ulong conn_timeout  = 1000;
    ulong recv_timeout  = 0;
    ulong send_timeout  = 0;
    bool  reuse_address = 1;
    bool  no_delay_mode = 0;
    bool  reuse_port    = 1;
    bool  keep_alive    = 0;
    bool  broadcast     = 0;
};

class socket_t {
protected:

    using TIMEVAL = struct timeval;
    using TCPPCB  = struct tcp_pcb;
    using UDPPCB  = struct udp_pcb;
    using PBUF    = struct pbuf   ; 

    /*─······································································─*/

    static void udp_recv_callback( void *arg, TCPPCB* newfd, PBUF* p, ip_addr_t *addr, u16_t port ){

    }

    /*─······································································─*/

    static void tcp_recv_callback() {}

    static void tcp_accept() {}

    static void tcp_connect() {}

    static void tcp_keep_alive() {}

protected:

    void kill() const noexcept {
        obj->state |= STATE::FS_STATE_KILL;
    switch( obj->type ){
        case SOCK_STREAM:
             tcp_shutdown( (TCPPCB*) get_addr_fd(), 1, 1 );
             tcp_close   ( (TCPPCB*) get_addr_fd() );
        break;
        case SOCK_DGRAM :
             udp_disconnect( (UDPPCB*) get_addr_fd() );
        break;
    } obj->fd = INVALID_SOCKET; }

    /*─······································································─*/
 
    bool is_state( uchar value ) const noexcept {
        if( obj->state & value ){ return true; }
    return false; }

    void set_state( uchar value ) const noexcept {
    if( obj->state & STATE::FS_STATE_KILL ){ return; }
        obj->state = value;
    }

    SOCKADDR* get_addr() const noexcept { 
        return is_server() ? (SOCKADDR*)&obj->client_addr 
        /*--------------*/ : (SOCKADDR*)&obj->server_addr; 
    }

    void* get_addr_fd() const noexcept {
        if( obj->fd == nullptr ){ return nullptr; }
        return type::cast<SOCKET_CTX>(fd)->addr;
    }

    /*─······································································─*/

    enum STATE {
         FS_STATE_UNKNOWN = 0b00000000,
         FS_STATE_OPEN    = 0b00000001,
         FS_STATE_CLOSE   = 0b00000010,
         FS_STATE_READING = 0b00010000,
         FS_STATE_WRITING = 0b00100000,
         FS_STATE_KILL    = 0b00000100,
         FS_STATE_REUSE   = 0b00001000,
         FS_STATE_DISABLE = 0b00001110,
         FS_STATE_SERVER  = 0b10000000
    };

    enum BLOCKED {
         ECONNECTDONE ,
         EACCEPTDONE  ,
         EWRITEDONE   ,
         EREADDONE    ,
         EDONE        ,
         EWRITTING    ,
         EREADING     ,
         EACCEPTING   ,
         ECONNECTING
    };

protected:

    struct NODE {

        ulong recv_timeout=0; 
        ulong send_timeout=0;
        ulong conn_timeout=0; 
        ulong range[2]={0,0};

        int feof = 1;

        SOCKET *fd = INVALID_SOCKET;
        uchar state=STATE::FS_STATE_OPEN;

        ptr_t<char> buffer; string_t borrow;
        generator::file::until _until;
        generator::file::line  _line ;
        generator::file::read  _read ;
        generator::file::write _write;

   ~NODE(){ if( fd == nullptr ){ return; }
        auto  tmp  = type::cast<SOCKET_CTX>(fd);
        uchar type = tmp->type;
        void* addr = tmp->addr;
    switch( type ){
        case SOCK_STREAM: tcp_remove( (TCPPCB*) addr ); break;
        case SOCK_STREAM: udp_remove( (UDPPCB*) addr ); break;
    }   delete tmp; }

    };  ptr_t<NODE> obj;

public:

    event_t<>          onUnpipe;
    event_t<>          onResume;
    event_t<except_t>  onError;
    event_t<>          onDrain;
    event_t<>          onClose;
    event_t<>          onOpen;
    event_t<>          onPipe;
    event_t<string_t>  onData;

    /*─······································································─*/

    int SOCK    = SOCK_STREAM;
    int AF      = AF_INET;
    int IPPROTO = 0;

    /*─······································································─*/

    ulong get_recv_timeout() const noexcept {
        return obj->recv_timeout==0 ? process::millis() : obj->recv_timeout;
    }

    ulong get_send_timeout() const noexcept {
        return obj->send_timeout==0 ? process::millis() : obj->send_timeout;
    }

    ulong get_conn_timeout() const noexcept {
        return obj->conn_timeout==0 ? process::millis() : obj->conn_timeout;
    }

    /*─······································································─*/

    ulong set_conn_timeout( ulong time ) const noexcept {
        if( time == 0 ){ obj->conn_timeout = 0; return 0; }
        obj->conn_timeout = process::millis() + time; 
        return time;
    }

    ulong set_recv_timeout( ulong time ) const noexcept {
        if( time == 0 ){ obj->recv_timeout = 0; return 0; } TIMEVAL en; 
        en.tv_sec  =  time / 1000; 
        en.tv_usec = (time % 1000) * 1000;
        int c = setsockopt( obj->fd, SOL_SOCKET, SO_RCVTIMEO, &en, sizeof(en) ); 
        obj->recv_timeout = process::millis() + time; 
        return c == 0 ? time : 0;
    }

    ulong set_send_timeout( ulong time ) const noexcept {
        if( time == 0 ){ obj->send_timeout = 0; return 0; }
        TIMEVAL en; memset( &en, 0, sizeof(en) ); en.tv_sec = time / 1000; en.tv_usec = 0;
    int c= setsockopt( obj->fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&en, sizeof(en) ); 
        obj->send_timeout = process::millis() + time; return c==0 ? time : 0;
    }

    /*─······································································─*/

    int set_no_delay_mode( uint en ) const noexcept { if( IPPROTO != IPPROTO_TCP ){ return -1; }
    int c= setsockopt( obj->fd, IPPROTO, TCP_NODELAY, (char*)&en, sizeof(en) ); 
        return c;
    }

    int set_recv_buff( uint en ) const noexcept {
    int c= setsockopt( obj->fd, SOL_SOCKET, SO_RCVBUF, (char*)&en, sizeof(en) ); 
        return c;
    }

    int set_send_buff( uint en ) const noexcept {
    int c= setsockopt( obj->fd, SOL_SOCKET, SO_SNDBUF, (char*)&en, sizeof(en) );
        return c;
    }

    int set_accept_connection( uint en ) const noexcept {
    int c= setsockopt( obj->fd, SOL_SOCKET, SO_ACCEPTCONN, (char*)&en, sizeof(en) ); 
        return c;
    }

    int set_dont_route( uint en ) const noexcept {
    int c= setsockopt( obj->fd, SOL_SOCKET, SO_DONTROUTE, (char*)&en, sizeof(en) );
        return c;
    }

    int set_keep_alive( uint en ) const noexcept {
    int c= setsockopt( obj->fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&en, sizeof(en) ); 
        return c;
    }

    int set_broadcast( uint en ) const noexcept {
    int c= setsockopt( obj->fd, SOL_SOCKET, SO_BROADCAST, (char*)&en, sizeof(en) ); 
        return c;
    }

    int set_reuse_address( uint en ) const noexcept {
    int c= setsockopt( obj->fd, SOL_SOCKET, SO_REUSEADDR, (char*)&en, sizeof(en) ); 
        return c;
    }

#ifdef SO_REUSEPORT
    int set_reuse_port( uint en ) const noexcept {
    int c= setsockopt( obj->fd, SOL_SOCKET, SO_REUSEPORT, (char*)&en, sizeof(en) );
        return c;
    }
#endif

    int get_no_delay_mode() const noexcept { int en; socklen_t size = sizeof(en);
    int c= getsockopt( obj->fd, IPPROTO, TCP_NODELAY, (char*)&en, &size ); 
        return c==0 ? en : c;
    }

    int get_error() const noexcept { int en; socklen_t size = sizeof(en);
    int c= getsockopt(obj->fd, SOL_SOCKET, SO_ERROR, (char*)&en, &size); 
        return c==0 ? en : c;
    }

    int get_recv_buff() const noexcept { int en; socklen_t size = sizeof(en);
    int c= getsockopt(obj->fd, SOL_SOCKET, SO_RCVBUF, (char*)&en, &size); 
        return c==0 ? en : c;
    }

    int get_send_buff() const noexcept { int en; socklen_t size = sizeof(en);
    int c= getsockopt(obj->fd, SOL_SOCKET, SO_SNDBUF, (char*)&en, &size); 
        return c==0 ? en : c;
    }

    int get_accept_connection() const noexcept { int en; socklen_t size = sizeof(en);
    int c= getsockopt(obj->fd, SOL_SOCKET, SO_ACCEPTCONN, (char*)&en, &size); 
        return c==0 ? en : c;
    }

    int get_dont_route() const noexcept { int en; socklen_t size = sizeof(en);
    int c= getsockopt(obj->fd, SOL_SOCKET, SO_DONTROUTE, (char*)&en, &size); 
        return c==0 ? en : c;
    }

    int get_reuse_address() const noexcept { int en; socklen_t size = sizeof(en);
    int c= getsockopt(obj->fd, SOL_SOCKET, SO_REUSEADDR, (char*)&en, &size);
        return c==0 ? en : c;
    }

#ifdef SO_REUSEPORT
    int get_reuse_port() const noexcept { int en; socklen_t size = sizeof(en);
    int c= getsockopt(obj->fd, SOL_SOCKET, SO_REUSEPORT, (char*)&en, &size);
        return c==0 ? en : c;
    }
#endif

    int get_keep_alive() const noexcept { int en; socklen_t size = sizeof(en);
    int c= getsockopt(obj->fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&en, &size);
        return c==0 ? en : c;
    }

    int get_broadcast() const noexcept { int en; socklen_t size = sizeof(en);
    int c= getsockopt(obj->fd, SOL_SOCKET, SO_BROADCAST, (char*)&en, &size);
        return c==0 ? en : c;
    }

    /*─······································································─*/

    expected_t<ip_t,except_t> get_sockname() const noexcept {
        SOCKADDR_ST addr; socklen_t len = sizeof(addr);

        if( is_closed() )
          { return except_t( "invalid socket" ); }

        if( getsockname( obj->fd, (SOCKADDR*)&addr, &len ) < 0 )
          { return except_t( "address not found" ); }

        char host[INET6_ADDRSTRLEN] = {0}; uint port;

        if( addr.ss_family == AF_INET ) {
            SOCKADDR_IN* s = (SOCKADDR_IN*)&addr;
            port = ntohs( ((SOCKADDR_IN*) &addr)->sin_port );
            inet_ntop( AF_INET, &s->sin_addr, host, sizeof(host) );
        } else {
            SOCKADDR_IN6* s = (SOCKADDR_IN6*)&addr;
            port = ntohs( ((SOCKADDR_IN6*) &addr)->sin6_port );
            inet_ntop( AF_INET6, &s->sin6_addr, host, sizeof(host) );
        }

        return ip_t({ host, port });
    }

    expected_t<ip_t,except_t> get_peername() const noexcept { 
        SOCKADDR_ST addr = get_addr(); socklen_t len = sizeof(addr);

        if( is_closed() )
          { return except_t( "invalid socket" ); }

        if( getpeername( obj->fd, (SOCKADDR*) &addr, &len ) < 0 )
          { return except_t( "address not found" ); }

        char host[INET6_ADDRSTRLEN] = {0}; uint port;

        if( addr.ss_family == AF_INET ) {
            SOCKADDR_IN* s = (SOCKADDR_IN*)&addr;
            port = ntohs( ((SOCKADDR_IN*) &addr)->sin_port );
            inet_ntop( AF_INET, &s->sin_addr, host, sizeof(host) );
        } else {
            SOCKADDR_IN6* s = (SOCKADDR_IN6*)&addr;
            port = ntohs( ((SOCKADDR_IN6*) &addr)->sin6_port );
            inet_ntop( AF_INET6, &s->sin6_addr, host, sizeof(host) );
        }

        return ip_t({ host, port });
    }

    /*─······································································─*/

    ulong set_buffer_size( ulong _size ) const noexcept {
    //  set_send_buff( _size ); set_recv_buff( _size );
        obj->buffer = ptr_t<char>(_size); return _size;
    }

    /*─······································································─*/

    ulong set_timeout( ulong time ) const noexcept {
        set_recv_timeout( time );
        set_send_timeout( time ); return time;
    }

    /*─······································································─*/

    void  resume() const noexcept { if(is_state(STATE::FS_STATE_OPEN )){ return; } set_state(STATE::FS_STATE_OPEN ); onResume.emit(); }
    void    stop() const noexcept { if(is_state(STATE::FS_STATE_REUSE)){ return; } set_state(STATE::FS_STATE_REUSE); onDrain .emit(); }
    void   reset() const noexcept { if(is_state(STATE::FS_STATE_KILL )){ return; } resume(); pos(0); }
    void   flush() const noexcept { obj->buffer.fill(0); }

    /*─······································································─*/

    bool     is_closed() const noexcept { return is_state(STATE::FS_STATE_DISABLE) || is_feof() || obj->fd==INVALID_SOCKET; }
    bool     is_server() const noexcept { return obj->state & STATE::FS_STATE_SERVER; }
    bool       is_feof() const noexcept { return obj->feof <= 0 && obj->feof != -2; }
    bool    is_waiting() const noexcept { return obj->feof == -2; }
    bool  is_available() const noexcept { return !is_closed(); }

    /*─······································································─*/

    void close() const noexcept {
        if( is_state ( STATE::FS_STATE_DISABLE ) ){ return; }
            set_state( STATE::FS_STATE_CLOSE   );
    onDrain.emit(); free(); }

    /*─······································································─*/

    SOCKET    get_fd() const noexcept { return obj == nullptr ? INVALID_SOCKET : obj->fd;    }
    ulong* get_range() const noexcept { return obj == nullptr ?        nullptr : obj->range; }

    /*─······································································─*/

    void   set_borrow( const string_t& brr ) const noexcept { obj->borrow = brr; }
    ulong  get_borrow_size() const noexcept { return obj->borrow.size(); }
    char*  get_borrow_data() const noexcept { return obj->borrow.data(); }
    void        del_borrow() const noexcept { obj->borrow.clear(); }
    string_t&   get_borrow() const noexcept { return obj->borrow; }

    /*─······································································─*/

    ulong   get_buffer_size() const noexcept { return obj->buffer.size(); }
    char*   get_buffer_data() const noexcept { return obj->buffer.data(); }
    ptr_t<char>& get_buffer() const noexcept { return obj->buffer; }

    /*─······································································─*/

    ulong pos( ulong /*unused*/ ) const noexcept { return 0; }

    ulong size() const noexcept { return 0; }

    ulong  pos() const noexcept { return 0; }

    /*─······································································─*/

    void set_sockopt( agent_t opt ) const noexcept {
        set_no_delay_mode( opt.no_delay_mode );
        set_reuse_address( opt.reuse_address );
        set_conn_timeout ( opt.conn_timeout  );
        set_recv_timeout ( opt.recv_timeout  );
        set_send_timeout ( opt.send_timeout  );
        set_buffer_size  ( opt.buffer_size   );
    #ifdef SO_REUSEPORT
        set_reuse_port   ( opt.reuse_port    );
    #endif
        set_keep_alive   ( opt.keep_alive    );
        set_broadcast    ( opt.broadcast     );
    }

    agent_t get_sockopt() const noexcept {
    agent_t opt;
        opt.reuse_address = get_reuse_address();
        opt.recv_timeout  = get_recv_timeout();
        opt.send_timeout  = get_send_timeout();
        opt.conn_timeout  = get_conn_timeout();
        opt.buffer_size   = get_buffer_size();
    #ifdef SO_REUSEPORT
        opt.reuse_port    = get_reuse_port();
    #endif
        opt.keep_alive    = get_keep_alive();
        opt.broadcast     = get_broadcast();
    return opt;
    }

    /*─······································································─*/
    
    virtual ~socket_t() noexcept { if( obj.count()>1 && !is_closed() ){ return; } free(); }

    socket_t( SOCKET* fd, ulong _size=CHUNK_SIZE ) : obj( new NODE() ) {
        if( fd == INVALID_SOCKET )
          { ARDUINO_ERROR("Such Socket has an Invalid fd"); }
        obj->fd = fd; set_buffer_size(_size);
    }

    socket_t() noexcept : obj( new NODE() ) {}

    /*─······································································─*/

    void free() const noexcept {

        if( is_state( STATE::FS_STATE_REUSE ) && !is_feof() && obj.count()>1 ){ return; }
        if( is_state( STATE::FS_STATE_KILL  ) ) /*-------*/ { return; } 
        if(!is_state( STATE::FS_STATE_CLOSE | STATE::FS_STATE_REUSE ) )
          { kill(); onDrain.emit(); } else { kill(); }

        onUnpipe.clear(); onResume.clear();
        onError .clear(); onData  .clear();
        onOpen  .clear(); onPipe  .clear(); onClose.emit();

    }

    /*─······································································─*/

    virtual int socket( const string_t& host, int port ) const noexcept {
        if( host.empty() ){ onError.emit("invalid IP address"); return -1; }
            obj->addrlen = sizeof( obj->server_addr );

        void* tmp = nullptr; switch( SOCK ){
            case SOCK_STREAM: tmp = tcp_new(); break;
            case SOCK_DGRAM : tmp = udp_new(); break;
        }

        if( tmp == INVALID_SOCKET )
          { onError.emit("can't initializate socket fd"); return -1; }

        obj->fd = new SOCKET_CTX({ tmp, SOCK });
        set_buffer_size( CHUNK_SIZE );
        set_reuse_address(1);

    #ifdef SO_REUSEPORT
        set_reuse_port(1);
    #endif

        SOCKADDR_IN server, client;
        memset(&server, 0, sizeof(SOCKADDR_IN));
        memset(&client, 0, sizeof(SOCKADDR_IN));
        server.sin_family = AF; if( port>0 ) server.sin_port = htons(port);

        if  ( host == "0.0.0.0"         || host == "global"    ){ server.sin_addr.s_addr = INADDR_ANY; }
        elif( host == "1.1.1.1"         || host == "loopback"  ){ server.sin_addr.s_addr = INADDR_LOOPBACK; }
        elif( host == "255.255.255.255" || host == "broadcast" ){ server.sin_addr.s_addr = INADDR_BROADCAST; }
        elif( host == "127.0.0.1"       || host == "localhost" ){ inet_pton(AF, "127.0.0.1", &server.sin_addr); }
        else                                                    { inet_pton(AF, host.c_str(),&server.sin_addr); }

        obj->server_addr = *((SOCKADDR_ST*) &server); 
        obj->client_addr = *((SOCKADDR_ST*) &client); 
        obj->len = sizeof( server ); /*--*/ return 1;

    }

    /*─······································································─*/

    int _connect() const noexcept { 
        if( process::millis() > get_conn_timeout() || is_server() ){ return -1; }
        if( obj->errno != BLOCKED::EDONE ) /*-------------------*/ { return -2; }
            obj->errno  = BLOCKED::ECONNECTING;
    switch( obj->type ){
        case SOCK_STREAM: 
             tcp_connect( (TCPPCB*) obj->fd, tcp_connect_callback );
        return  1; break;
        case SOCK_DGRAM : 
             udp_connect( (UDPPCB*) obj->fd, udp_connect_callback );
        return  1; break;
    }   return -1; }

    int _accept() const noexcept { 
        if( !is_server() ) /*----------*/ { return -1; } 
        if( obj->errno != BLOCKED::EDONE ){ return -2; }
            obj->errno  = BLOCKED::EACCEPTING;
    switch( obj->type ){
        case SOCK_STREAM: 
             tcp_accept( (TCPPCB*) obj->fd, tcp_accept_callback );
        return  1; break;
        case SOCK_DGRAM : 
             udp_accept( (UDPPCB*) obj->fd, udp_accept_callback );
        return  1; break;
    }   return -1; }

    /*─······································································─*/

    int listen() const noexcept { if( !is_server() ){ return -1; }
        if( obj->fd == INVALID_SOCKET ){ return -1; } 
    switch( obj->type ){
        case SOCK_STREAM:
             obj->fd = tcp_listen( (TCPPCB*) obj->fd );
        return  1; break;
        case SOCK_DGRAM :
             obj->fd = udp_listen( (UDPPCB*) obj->fd );
        return  1; break;
    }   return -1; }

    int connect() const noexcept { int c=0;
        while((c=_connect()) == -2 ){ process::next(); } return c;
    }

    int accept() const noexcept { int c=0;
        while((c=_accept()) == -2 ){ process::next(); } return c;
    }

    int bind() const noexcept { obj->state |= STATE::FS_STATE_SERVER;
        if( obj->fd == INVALID_SOCKET ){ return -1; } 
    switch( obj->type ){
        case SOCK_STREAM:
             tcp_bind( (TCPPCB*) obj->fd );
        return  1; break;
        case SOCK_DGRAM :
             udp_bind( (UDPPCB*) obj->fd );
        return  1; break; 
    }   return -1; }

    /*─······································································─*/

    string_t read( ulong size=CHUNK_SIZE ) const noexcept {
        while( obj->_read( this, size )==1 ){ process::next(); }
        return obj->_read.data;
    }

    char read_char() const noexcept { return read(1)[0]; }

    ulong write( const string_t& msg ) const noexcept {
        while( obj->_write( this, msg )==1 ){ process::next(); }
        return obj->_write.data;
    }

    /*─······································································─*/

    string_t read_until( string_t ch ) const noexcept {
        while( obj->_until( this, ch )==1 ){ process::next(); }
        return obj->_until.data;
    }

    string_t read_until( char ch ) const noexcept {
        while( obj->_until( this, ch )==1 ){ process::next(); }
        return obj->_until.data;
    }

    string_t read_line() const noexcept {
        while( obj->_line( this )==1 ){ process::next(); }
        return obj->_line.data;
    }

    /*─······································································─*/

    virtual int _read ( char* bf, const ulong& sx ) const noexcept { return __read ( bf, sx ); }
    virtual int _write( char* bf, const ulong& sx ) const noexcept { return __write( bf, sx ); }

    /*─······································································─*/

    virtual int __read( char* bf, const ulong& sx ) const noexcept {
        if ( process::millis() > get_recv_timeout() || is_closed() )
           { return -1; } if ( sx==0 ) { return 0; }

        int res = ( SOCK != SOCK_DGRAM ) 
                ? ::recv    ( obj->fd, bf, sx, 0 )
                : ::recvfrom( obj->fd, bf, sx, 0, get_addr(), &obj->len );
           
             obj->feof = is_blocked( res )? -2 : res;
    return ( obj->feof <= 0 && obj->feof != -2 ) ? -1 : obj->feof; }

    virtual int __write( char* bf, const ulong& sx ) const noexcept {
        if ( process::millis() > get_send_timeout() || is_closed() )
           { return -1; } if ( sx==0 ) { return 0; } 

        int res = ( SOCK != SOCK_DGRAM ) 
                ? ::send  ( obj->fd, bf, sx, 0 )
                : ::sendto( obj->fd, bf, sx, 0, get_addr(), obj->len );

             obj->feof = is_blocked( res )? -2 : res;
    return ( obj->feof <= 0 && obj->feof != -2 ) ? -1 : obj->feof; }

    /*─······································································─*/

    int _write_( char* bf, const ulong& sx, ulong* sy ) const noexcept {
        if( sx==0 || is_closed() ){ return -1; } while( *sy<sx ) {
            int c = __write( bf + *sy, sx - *sy );
            if( c <= 0 && c != -2 ) /*----*/ { return -2; }
            if( c >  0 ){ *sy+= c; continue; } break/**/;
        }   return sx;
    }

    int _read_( char* bf, const ulong& sx, ulong* sy ) const noexcept {
        if( sx==0 || is_closed() ){ return -1; } while( *sy<sx ) {
            int c = __read( bf + *sy, sx - *sy );
            if( c <= 0 && c != -2 ) /*----*/ { return -2; }
            if( c >  0 ){ *sy+= c; continue; } break/**/;
        }   return sx;
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

#endif

/*────────────────────────────────────────────────────────────────────────────*/