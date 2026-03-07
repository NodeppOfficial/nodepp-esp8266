#ifndef NODEPP_WIFI
#define NODEPP_WIFI

/*────────────────────────────────────────────────────────────────────────────*/

#if _KERNEL_ == NODEPP_KERNEL_ARDUINO
    #include "dns.h"
    #include "event.h"
    #include "arduino/wifi.h"
#else
    #error "This OS Does not support wifi.h"
#endif

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace wifi {

    inline wifi_t& get_wifi_device() { 
    static wifi_t device; return device;
    }

    inline int turn_on () { return get_wifi_device().turn_on(); }

    inline int turn_off() { return get_wifi_device().turn_off(); }

    inline promise_t< ptr_t<wifi_device_t>, except_t >
    scan() { return get_wifi_device().scan(); }

    inline promise_t< wifi_t, except_t >
    connect_wifi_AP( string_t ssid, string_t pass ){
        return get_wifi_device().connect_wifi_AP( ssid, pass );
    }

    inline promise_t< wifi_t, except_t > 
    create_wifi_AP( string_t ssid, string_t pass, uint8 channel, wifi_auth_mode_t authmode=WIFI_AUTH_WPA2_PSK ){
        return get_wifi_device().create_wifi_AP( ssid, pass, channel, authmode );
    }

} }

/*────────────────────────────────────────────────────────────────────────────*/

#endif