#include "VisaWsServer.hpp"
#include <regex>

namespace hippo
{

std::string parse_param(std::string& resoure, const std::string& key){
    std::smatch result;
    if (std::regex_search(resoure.cbegin(), resoure.cend(), result, std::regex(key + "=(.*?)&"))) {
        return result[1];
    } else if (std::regex_search(resoure.cbegin(), resoure.cend(), result, std::regex(key + "=(.*)"))) {
        return result[1];
    } else {
        return std::string();
    }
}

int VisaWsServer::init(){
    return vsp_->init();
}

bool VisaWsServer::start(int port){

    // Initialize Asio Transport
    m_server.init_asio();
    m_server.start_perpetual();
    m_server.set_reuse_addr(true);
    
    // Register handler callbacks
    m_server.set_open_handler(websocketpp::lib::bind(&VisaWsServer::on_open,this,websocketpp::lib::placeholders::_1));
    m_server.set_close_handler(websocketpp::lib::bind(&VisaWsServer::on_close,this,websocketpp::lib::placeholders::_1));
    m_server.set_message_handler(websocketpp::lib::bind(&VisaWsServer::on_message,this,websocketpp::lib::placeholders::_1,websocketpp::lib::placeholders::_2));
    m_server.clear_access_channels(websocketpp::log::alevel::all);
    m_server.set_access_channels(websocketpp::log::alevel::connect);
    m_server.set_access_channels(websocketpp::log::alevel::disconnect);
    //m_server.set_access_channels(websocketpp::log::alevel::app);
    //m_server.set_access_channels(websocketpp::log::alevel::frame_payload);
    
    std::cout << "server ready to listen at port:"<< port<<std::endl;
    // listen on specified port
    m_server.listen(websocketpp::lib::asio::ip::tcp::v4(), port);
    
    // Start the server accept loop
    std::cout << "server ready to accept:"<< std::endl;
    m_server.start_accept();
    
    std::cout << "server ready to run:"<< std::endl;
    // Start the ASIO io_service run loop
    try {
        m_server.run();
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
    }
    return true;
}

bool VisaWsServer::start(){
    return start(DEF_PORT);
}

bool VisaWsServer::stop(){
    m_server.stop();
    return true;
}

void VisaWsServer::on_open(websocketpp::connection_hdl conn)
{
    WsServer::connection_ptr con = m_server.get_con_from_hdl(conn);
    std::string res = con->get_resource();

    std::cout<<res<<std::endl;
    if(res.find("/avisa/service/availableDevs") != std::string::npos)
    {
        std::string pattern = parse_param(res, "pattern");
        auto list = vsp_->availableDevs(pattern);
        std::string ret("");

        std::cout<<"pattern is "+pattern<<std::endl;

        for(auto i:list){
            ret += i.res_; 
            ret += ",";
        }
        if(ret.size() <= 0){
            ret += ",";
        }

        std::cout<<"ret is:"+ret<<std::endl;
        m_server.send(conn, ret.data(), websocketpp::frame::opcode::value::text);
    }
    else if(res.find("/avisa/service/connect") != std::string::npos)
    {
        std::string res_str = parse_param(res, "res");
        auto f = vsp_->getFirstFinder(res_str);
        VisaURL url(res_str, f);

        for (auto i:visa_conns_){
            if(i->isMe(url.sha1_)){
                i->refMe();
                m_server.send(conn, url.sha1_.data(), websocketpp::frame::opcode::value::text);
                return;
            } 
        }

        std::cout<<"res is "+ res_str<<std::endl;
        if(f != nullptr){
            int error = 0;
            VisaConnctionPtr vc = vsp_->connect(url, &error);
            if(vc != nullptr){
                std::cout<<"res sha1 is "+ url.sha1_<<std::endl;
                m_server.send(conn, url.sha1_.data(), websocketpp::frame::opcode::value::text);
                visa_conns_.insert(vc);
            }
        }else{
            std::cout<<"but res has no finder"<<std::endl;
        }
    }
    else if(res.find("/avisa/service/disconnect") != std::string::npos)
    {
        std::string res_str = parse_param(res, "res");
        VisaURL url(res_str, nullptr);
        for (auto i:visa_conns_){
            if(i->isMe(url.sha1_)){
                if(i->close()){
                    visa_conns_.erase(i);
                }
            } 
        }
    }else
    {
        std::smatch result;
        if (std::regex_search(res.cbegin(), res.cend(), result, std::regex("/avisa/connection([a-zA-Z0-9]{40})/recv"))) {
            std::string con_sha1 = result[1];
            std::cout<<"got recv route: "+ con_sha1<<std::endl;
            for (auto i:visa_conns_){
                if(i->isMe(con_sha1)){
                    int ret = i->recv(buf_+4, sizeof(buf_)-4);
                    memcpy(buf_, &ret, 4);
                    printf("recv data is %s<--->%d\n", buf_+4, ret);
                    if(ret > 0)
                    {
                        m_server.send(conn, buf_, 4+ret, websocketpp::frame::opcode::value::binary);
                    } else {
                        m_server.send(conn, &ret, sizeof(ret), websocketpp::frame::opcode::value::binary);
                    }
                    break;
                } 
            }
        }
        else if (std::regex_search(res.cbegin(), res.cend(), result, std::regex("/avisa/connection([a-zA-Z0-9]{40})/get_option"))) {
            std::string con_sha1 = result[1];
            std::cout<<"got get option route: "+ con_sha1<<std::endl;
            for (auto i:visa_conns_){
                if(i->isMe(con_sha1)){
                    std::string ostr = parse_param(res, "option");
                    int option = stoul(ostr);
                    char buf[32];
                    int32_t ret = i->getOption(option, buf+4, 32);
                    memcpy(buf_, &ret, 4);
                    if(ret < 0){
                        m_server.send(conn, &ret, sizeof(ret), websocketpp::frame::opcode::value::binary);
                    } else {
                        m_server.send(conn, buf_, 4+ret, websocketpp::frame::opcode::value::binary);
                    }
                    break;
                }
            }
        }
    }


    //connections_.insert(conn);
}

void VisaWsServer::on_close(websocketpp::connection_hdl conn)
{
    //connections_.erase(conn);
}

void VisaWsServer::on_message(websocketpp::connection_hdl conn, WsServer::message_ptr msg)
{
    WsServer::connection_ptr con = m_server.get_con_from_hdl(conn);
    std::string res = con->get_resource();
    std::string payload = msg->get_payload(); 
    int32_t size = payload.size();
    const char *data = payload.data();

    {
        std::smatch result;

        if (std::regex_search(res.cbegin(), res.cend(), result, std::regex("/avisa/connection([a-zA-Z0-9]{40})/send"))) {
            std::string con_sha1 = result[1];
            std::cout<<"got send route: "+ con_sha1<<std::endl;
            for (auto i:visa_conns_){
                if(i->isMe(con_sha1)){
                    int32_t ret = i->send(data, size);
                    printf("send data is %s<--->%d\n", data, ret);
                    m_server.send(conn, &ret, sizeof(ret), websocketpp::frame::opcode::value::binary);
                    break;
                }
            }
        }
        else if (std::regex_search(res.cbegin(), res.cend(), result, std::regex("/avisa/connection([a-zA-Z0-9]{40})/set_option"))) {
            std::string con_sha1 = result[1];
            std::cout<<"got set option route: "+ con_sha1<<std::endl;
            for (auto i:visa_conns_){
                if(i->isMe(con_sha1)){
                    std::string ostr = parse_param(res, "option");
                    int option = stoul(ostr);
                    int32_t ret = i->setOption(option, data, size);
                    m_server.send(conn, &ret, sizeof(ret), websocketpp::frame::opcode::value::binary);
                    break;
                }
            }
        }
    }
}

}

