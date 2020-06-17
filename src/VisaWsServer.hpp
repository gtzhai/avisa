#ifndef __VISA_WS_SERVER_HPP__
#define __VISA_WS_SERVER_HPP__

#include "VisaService.hpp"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

#if 1
struct testee_config : public websocketpp::config::asio {
    // pull default settings from our core config
    typedef websocketpp::config::asio core;

    typedef core::concurrency_type concurrency_type;
    typedef core::request_type request_type;
    typedef core::response_type response_type;
    typedef core::message_type message_type;
    typedef core::con_msg_manager_type con_msg_manager_type;
    typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;

    typedef core::alog_type alog_type;
    typedef core::elog_type elog_type;
    typedef core::rng_type rng_type;
    typedef core::endpoint_base endpoint_base;

    static bool const enable_multithreading = true;

    struct transport_config : public core::transport_config {
        typedef core::concurrency_type concurrency_type;
        typedef core::elog_type elog_type;
        typedef core::alog_type alog_type;
        typedef core::request_type request_type;
        typedef core::response_type response_type;

        static bool const enable_multithreading = true;
    };

    typedef websocketpp::transport::asio::endpoint<transport_config>
        transport_type;

    static const websocketpp::log::level elog_level =
        websocketpp::log::elevel::none;
    static const websocketpp::log::level alog_level =
        websocketpp::log::alevel::none;
        
    /// permessage_compress extension
    struct permessage_deflate_config {};

    typedef websocketpp::extensions::permessage_deflate::enabled
        <permessage_deflate_config> permessage_deflate_type;
};
typedef websocketpp::server<testee_config> WsServer;
#else
typedef websocketpp::server<websocketpp::config::asio> WsServer;
#endif

#include <set>

namespace hippo
{
/*
 *ws://local:5026/avisa/service/availableDevs
 *ws://local:5026/avisa/service/connect
 *ws://local:5026/avisa/service/disconnect
 *ws://local:5026/avisa/connection<sha1>/recv
 *ws://local:5026/avisa/connection<sha1>/send
 *ws://local:5026/avisa/connection<sha1>/set_option
 *ws://local:5026/avisa/connection<sha1>/get_option
 *ws://local:5026/scpi/
 *
 *
 */
class VisaWsServer{
public:
    const int DEF_PORT=5026;

    explicit VisaWsServer(int port):port_(port), vsp_(std::make_unique<VisaService>()){};
    int init();
    bool start();
    bool start(int port);
    bool stop();
private:
    void on_open(websocketpp::connection_hdl conn);
    void on_close(websocketpp::connection_hdl conn);
    void on_message(websocketpp::connection_hdl conn, WsServer::message_ptr msg);

    typedef std::set<websocketpp::connection_hdl,std::owner_less<websocketpp::connection_hdl>> ConnectionList ;
    ConnectionList connections_;
    WsServer m_server;
    
    typedef std::set<VisaConnctionPtr> VisaConnctionPtrList ;
    VisaServicePtr vsp_;
    VisaConnctionPtrList visa_conns_;
    char buf_[1024+4];

    int port_;
};
typedef std::shared_ptr<VisaWsServer> VisaWsServerPtr;

};

#endif

