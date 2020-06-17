#include "SessionManager.hpp"
#include "IdManager.hpp"
#include <string>

namespace hippo
{

    SessionManager::SessionManager(uint32_t id, ::std::string& root_url):Session(root_url, Resource(""), id){
    };

    ResList SessionManager::availableDevs(const ::std::string &pattern){
        websocketpp::lib::error_code ec;
        client.reset();
        WsClient::connection_ptr con = client.get_connection(ws_url+"/avisa/service/availableDevs?pattern="+pattern, ec);
        rl.list.clear();
        client.connect(con);
        client.run(); 

        return rl;
    }

    ViSession SessionManager::connect(Resource &res, int *error){
        websocketpp::lib::error_code ec;
        res_ = res;
        client.reset();
        WsClient::connection_ptr con = client.get_connection(ws_url+"/avisa/service/connect?res="+res.resStr(), ec);
        client.connect(con);
        std::cout << "after con" <<std::endl;
        client.run(); 
        return id_;
    }

    SessionPtr SessionManager::getSession(ViSession id){
        auto search = repo.find(id);
        if (search != repo.end()) {
            return search->second;
        } else {
            return nullptr;
        }
    }

    int SessionManager::disconnect(ViSession id){
        auto search = repo.find(id);
        if (search != repo.end()) {
            auto sp = search->second;
            websocketpp::lib::error_code ec;
            client.reset();
            WsClient::connection_ptr con = client.get_connection(ws_url+"/avisa/service/disconnect?res="+sp->getResource().resStr(), ec);
            client.connect(con);
            std::cout << "after con" <<std::endl;
            client.run(); 

            unregisterSession(id);
        }
        return 0;
    }

    int SessionManager::registerSession(ViSession id, SessionPtr session){
        repo.insert(::std::pair<ViSession, SessionPtr>(id, session));
        return 0;
    }

    void SessionManager::unregisterSession(ViSession id){
        repo.erase(id); 
    }
/*-----------------------------------------------------------------------------------------------*/
    Session::Session(::std::string& url, Resource res, uint32_t manid):ws_url(url), res_(res), manid_(manid){

        client.clear_access_channels(websocketpp::log::alevel::all);
        client.clear_error_channels(websocketpp::log::elevel::all);
        client.init_asio();
        client.set_message_handler(websocketpp::lib::bind(&Session::on_message,this,websocketpp::lib::placeholders::_1,websocketpp::lib::placeholders::_2));
        client.set_open_handler(websocketpp::lib::bind(&Session::on_open,this,websocketpp::lib::placeholders::_1));
        client.set_fail_handler(websocketpp::lib::bind(&Session::on_fail,this,websocketpp::lib::placeholders::_1));
        client.set_close_handler(websocketpp::lib::bind(&Session::on_close,this,websocketpp::lib::placeholders::_1));
        client.stop_perpetual();
    
    };

    int Session::send(const unsigned char *data, ssize_t len){
        websocketpp::lib::error_code ec;
        client.reset();
        WsClient::connection_ptr con = client.get_connection(ws_url+"/send", ec);
        client.connect(con);

        send_buf = data;
        send_ideal_len = len;
        send_actual_len = 0;
        client.run(); 
        return send_actual_len;
    }


    int Session::recv(unsigned char *data, ssize_t len){
        websocketpp::lib::error_code ec;
        client.reset();
        WsClient::connection_ptr con = client.get_connection(ws_url+"/recv", ec);
        client.connect(con);

        recv_buf = data;
        recv_ideal_len = len;
        recv_actual_len = 0;
        client.run(); 
        return recv_actual_len;
    }

    int Session::setOption(uint32_t option, const void *data, ssize_t len){
        websocketpp::lib::error_code ec;
        client.reset();
        WsClient::connection_ptr con = client.get_connection(ws_url+"/set_option?option="+::std::to_string(option), ec);
        client.connect(con);
        set_option = option;
        set_option_data = data;
        set_option_data_len = len;
        client.run(); 
        return 0;
    }

    int Session::getOption(uint32_t option, void *data, ssize_t len){

        websocketpp::lib::error_code ec;
        client.reset();
        WsClient::connection_ptr con = client.get_connection(ws_url+"/get_option?option="+::std::to_string(option), ec);
        client.connect(con);
        get_option = option;
        get_option_data = data;
        get_option_data_len = len;
        client.run(); 
        return 0;
    }

    void Session::on_message(websocketpp::connection_hdl conn, message_ptr msg){
        WsClient::connection_ptr con = client.get_con_from_hdl(conn);
        ::std::string res = con->get_resource();
        ::std::string payload = msg->get_payload(); 
        int32_t size = payload.size();
        const char *data = payload.data();

        ::std::cout <<"res is " + res<<::std::endl;
        ::std::cout <<"payload is " + payload<<::std::endl;

        if(res.find("/avisa/service/availableDevs") != ::std::string::npos){
            ::std::regex word_regex("(.*),");
            auto words_begin = ::std::sregex_iterator(payload.begin(), payload.end(), word_regex);
            auto words_end   = ::std::sregex_iterator();
 
            for (::std::sregex_iterator iter = words_begin; iter != words_end; ++iter) {
                ::std::smatch match = *iter;
                //::std::string match_str = match.str();
                ::std::string match_str = match[1];
                if (match_str.size() > 2) {
                    ::std::cout <<"got dev:" + match_str <<::std::endl;
                    rl.list.push_back(Resource(match_str));
                }
            }
            client.stop();
        }
        else if(res.find("/avisa/service/connect") != ::std::string::npos){
            ::std::string url(ws_url+"/avisa/connection"+payload);
            SessionPtr sp = ::std::make_shared<Session>(url, res_, manid_);

            if(sp != nullptr){
                uint32_t id = IdGen::genSessionId(manid_);
                ::std::cout <<"setup a new session: "<<id<<::std::endl;
                registerSession(id, sp);
                id_ = id;
            }
            client.stop();
        } else {
            ::std::smatch result;

            if (::std::regex_search(res.cbegin(), res.cend(), result, ::std::regex("/avisa/connection([a-zA-Z0-9]{40})/send"))) {
                ::std::string con_sha1 = result[1];
                if(size == sizeof(int32_t)){
                    int32_t ret = *((int32_t *)data); 
                    send_actual_len = ret;
                }else{
                    ::std::cout <<"got send ret error: "<<::std::endl;
                    send_actual_len = 0;
                }
                client.stop();
            }
            else if (::std::regex_search(res.cbegin(), res.cend(), result, ::std::regex("/avisa/connection([a-zA-Z0-9]{40})/recv"))) {
                ::std::string con_sha1 = result[1];

                if(size >= sizeof(int32_t)){
                    int32_t ret = *((int32_t *)data); 
                    recv_actual_len = ret;
                    memcpy(recv_buf, data+sizeof(int32_t), size-sizeof(int32_t));
                }
                client.stop();
            }
            else if (::std::regex_search(res.cbegin(), res.cend(), result, ::std::regex("/avisa/connection([a-zA-Z0-9]{40})/set_option"))) {
                ::std::string con_sha1 = result[1];
            }
            else if (::std::regex_search(res.cbegin(), res.cend(), result, ::std::regex("/avisa/connection([a-zA-Z0-9]{40})/get_option"))) {
                ::std::string con_sha1 = result[1];
            }
        }

        ::std::cout <<"out"<<::std::endl;

    }

    void Session::on_open(websocketpp::connection_hdl conn){
        WsClient::connection_ptr con = client.get_con_from_hdl(conn);
        ::std::string res = con->get_resource();

        if(res.find("/avisa/service/availableDevs") != ::std::string::npos){
        }
        else if(res.find("/avisa/service/disconnect") != ::std::string::npos){
            client.stop();
        }
        else if(res.find("/avisa/service/connect") != ::std::string::npos){
        } else {
            ::std::smatch result;

            if (::std::regex_search(res.cbegin(), res.cend(), result, ::std::regex("/avisa/connection([a-zA-Z0-9]{40})/send"))) {
                ::std::string con_sha1 = result[1];
                client.send(conn, send_buf, send_ideal_len, websocketpp::frame::opcode::value::binary);
            }
            else if (::std::regex_search(res.cbegin(), res.cend(), result, ::std::regex("/avisa/connection([a-zA-Z0-9]{40})/recv"))) {
                ::std::string con_sha1 = result[1];
            }
            else if (::std::regex_search(res.cbegin(), res.cend(), result, ::std::regex("/avisa/connection([a-zA-Z0-9]{40})/set_option"))) {
                ::std::string con_sha1 = result[1];
            }
            else if (::std::regex_search(res.cbegin(), res.cend(), result, ::std::regex("/avisa/connection([a-zA-Z0-9]{40})/get_option"))) {
                ::std::string con_sha1 = result[1];
            }
        }

    }

    void Session::on_close(websocketpp::connection_hdl conn){
    
    }

    void Session::on_fail(websocketpp::connection_hdl conn){
    
    }

}

