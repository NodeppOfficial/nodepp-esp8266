/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOfficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_ARDUINO_WIFI_ESP8266
#define NODEPP_ARDUINO_WIFI_ESP8266

/*────────────────────────────────────────────────────────────────────────────*/

#include <ESP8266WiFi.h>

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace wifi { enum AUTHMODE {

    WIFI_AUTHMODE_OPEN         = ENC_TYPE_NONE,
    WIFI_AUTHMODE_WEP          = ENC_TYPE_WEP,
    WIFI_AUTHMODE_WPA_PSK      = ENC_TYPE_TKIP,
    WIFI_AUTHMODE_WPA2_PSK     = ENC_TYPE_CCMP,
    WIFI_AUTHMODE_AUTO         = ENC_TYPE_AUTO

};}}

namespace nodepp { namespace wifi { enum ANTENNA {
    WIFI_ANTENNA_0       = 0,
    WIFI_ANTENNA_INVALID = 1
};}}

namespace nodepp { struct wifi_device_t {
    string_t /*---*/ mac     ;
    string_t /*---*/ name    ;
    uint8 /*------*/ channel ;
    uint8 /*------*/ strength;
    uint8 /*------*/ auth_mode;
}; }

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class wifi_t {
private:

    enum STATE {
         WIFI_STATE_UNKNOWN   = 0b00000000,
         WIFI_STATE_USED      = 0b01000000,
         WIFI_STATE_SCANNING  = 0b10000000,
         WIFI_STATE_AVAILABLE = 0b00100000
    };

    struct NODE { int state; }; ptr_t<NODE> obj;

public:

    /*─······································································─*/

   ~wifi_t() noexcept {
        if( obj.count() > 1 ){ return; }
        if( obj->state == 0 ){ return; } free();
    }

    wifi_t() noexcept : obj( new NODE() ) {
        obj->state = STATE::WIFI_STATE_AVAILABLE;
        WiFi.persistent(false);
        WiFi.mode(WIFI_OFF);
    }

    /*─······································································─*/

    void    free() const noexcept { obj->state=0x00; turn_off(); }

    int  turn_on() const noexcept { return 1; } // ESP8266 handles power-up via mode()

    int turn_off() const noexcept { return WiFi.mode(WIFI_OFF) ? 1 : -1; }

    /*─······································································─*/

    promise_t< wifi_t, except_t >
    create_wifi_AP( string_t ssid, string_t pass, uint8 channel ) const noexcept {
        auto self = type::bind( this );
    return promise_t<wifi_t,except_t> ([=]( res_t<wifi_t> res, rej_t<except_t> rej ){

        if( obj->state & STATE::WIFI_STATE_USED )
          { rej( except_t( "wifi already in used" ) ); return; }

        WiFi.mode( WIFI_AP );

        if( WiFi.softAP( ssid.get(), pass.get(), channel, false, MAX_SOCKET ) ){
            res( *self ); self->obj->state |= STATE::WIFI_STATE_USED;
        } else {
            rej( except_t( "can't start AP mode" ) );
        }
    
    }); }

    /*─······································································─*/

    promise_t< wifi_t, except_t >
    connect_wifi_AP( string_t ssid, string_t pass ) const noexcept {
        auto self = type::bind( this );
    return promise_t<wifi_t,except_t> ([=]( res_t<wifi_t> res, rej_t<except_t> rej ){

        if( obj->state & STATE::WIFI_STATE_USED )
          { rej( except_t( "wifi already in used" ) ); return; }

        WiFi.mode ( WIFI_STA );
        WiFi.begin( ssid.get(), pass.get() );

        // Note: ESP8266 begin() is non-blocking. 
        // Logic here assumes successful initiation of connection.

        res( *self ); self->obj->state |= STATE::WIFI_STATE_USED;
    
    }); }

    /*─······································································─*/

    promise_t< ptr_t<wifi_device_t>, except_t > scan() const noexcept {
        auto self = type::bind( this );
    return promise_t< ptr_t<wifi_device_t>, except_t > ([=]( 
        res_t< ptr_t<wifi_device_t> > res, rej_t<except_t> rej 
    ){

        if( obj->state & STATE::WIFI_STATE_SCANNING )
          { rej( "wifi already scanning" ); return; }

        WiFi.mode( WIFI_STA );
        self->obj->state |= STATE::WIFI_STATE_SCANNING;

        WiFi.scanNetworksAsync([=]( int n ){
            
            if( n<0 ) {
                self->obj->state &= ~STATE::WIFI_STATE_SCANNING;
                rej( except_t("Scan failed") );
            return; }

            elif( n==0 ){
                self->obj->state &= ~STATE::WIFI_STATE_SCANNING;
                rej( except_t("No Networks Found") );
            return; }

            ptr_t<wifi_device_t> device( n );
            for( int i = 0; i < n; i++ ){
                device[i].name      = string_t( WiFi.SSID(i).c_str() );
                device[i].strength  = WiFi.RSSI(i);
                device[i].mac       = string_t( WiFi.BSSIDstr(i).c_str() );
                device[i].auth_mode = WiFi.encryptionType(i);
                device[i].channel   = WiFi.channel(i);
            }

            self->obj->state &= ~STATE::WIFI_STATE_SCANNING;
            WiFi.scanDelete(); res( device );

        }, false );

    }); }

};}

#endif