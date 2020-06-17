
#ifndef __SESSION_MANAGER_HPP__
#define __SESSION_MANAGER_HPP__

#include "basic.hpp"
#include "Resource.hpp"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

using namespace websocketpp;
#if 1
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>
struct deflate_config : public websocketpp::config::asio_client {
    typedef deflate_config type;
    typedef asio_client base;
    
    typedef base::concurrency_type concurrency_type;
    
    typedef base::request_type request_type;
    typedef base::response_type response_type;

    typedef base::message_type message_type;
    typedef base::con_msg_manager_type con_msg_manager_type;
    typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;
    
    typedef base::alog_type alog_type;
    typedef base::elog_type elog_type;
    
    typedef base::rng_type rng_type;
    
    struct transport_config : public base::transport_config {
        typedef type::concurrency_type concurrency_type;
        typedef type::alog_type alog_type;
        typedef type::elog_type elog_type;
        typedef type::request_type request_type;
        typedef type::response_type response_type;
        typedef websocketpp::transport::asio::basic_socket::endpoint 
            socket_type;
    };

    typedef websocketpp::transport::asio::endpoint<transport_config> 
        transport_type;
        
    /// permessage_compress extension
    struct permessage_deflate_config {};

    typedef websocketpp::extensions::permessage_deflate::enabled
        <permessage_deflate_config> permessage_deflate_type;
};
typedef websocketpp::client<deflate_config> WsClient;
#else
typedef websocketpp::client<websocketpp::config::asio> WsClient;
#endif
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

namespace hippo
{

class Session;
typedef ::std::shared_ptr<Session> SessionPtr;
class Session{
public:
    Session(::std::string& url, Resource res, uint32_t manid);

    int recv(unsigned char *data, ssize_t len);
    int send(const unsigned char *data, ssize_t len);
    int setOption(uint32_t option, const void *data, ssize_t len);
    int getOption(uint32_t option, void *data, ssize_t len);

    const Resource& getResource() const{
        return res_; 
    };

protected:
    void on_message(websocketpp::connection_hdl conn, message_ptr msg);
    void on_open(websocketpp::connection_hdl conn);
    void on_fail(websocketpp::connection_hdl conn);
    void on_close(websocketpp::connection_hdl conn);
    virtual int registerSession(ViSession id, SessionPtr session){return -1;};
    virtual void unregisterSession(ViSession id) {};

    unsigned char *recv_buf;
    ssize_t recv_ideal_len;
    ssize_t recv_actual_len;

    const unsigned char *send_buf;
    ssize_t send_ideal_len;
    ssize_t send_actual_len;

    uint32_t set_option;
    const void *set_option_data;
    ssize_t set_option_data_len;

    uint32_t get_option;
    void *get_option_data;
    ssize_t get_option_data_len;

    ::std::string ws_url;
    Resource res_;
    WsClient client;
    uint32_t manid_;
    uint32_t id_;
    ResList rl;
};

class SessionManager:Session{
public:
    SessionManager(uint32_t id, ::std::string& root_url);

    ResList availableDevs(const ::std::string &pattern);
    ViSession connect(Resource &res, int *error);
    SessionPtr getSession(ViSession id);
    int disconnect(ViSession id);
private:
    int registerSession(ViSession id, SessionPtr session) override;
    void unregisterSession(ViSession id) override;
    ::std::map<ViSession, SessionPtr> repo;
};
typedef ::std::shared_ptr<SessionManager> SessionManagerPtr;

}

#endif

