#include <nodepp/nodepp.h>
#include <nodepp/wifi.h>
#include <nodepp/http.h>

using namespace nodepp;

void start_server() {

    auto skt = http::server([=]( http_t cli ){
        
        cli.write_header( 200, header_t({
            { "Content-Type", "text/html" }
        }) );

        cli.write( "hello world!" );

    });

    skt.listen( "0.0.0.0", 8000, [=]( socket_t /*unused*/ ){
        console::log( "-> http://localhost:8000" );
    });

}

void onMain() {

    console::enable( 9600 );

    wifi::connect_wifi_AP( "WIFI_NAME", "WIFI_PASS" )

    .then([=]( wifi_t /*unused*/ ){
        start_server();
    })

    .fail([=]( except_t err ){
        console::log( err );
    });

}
