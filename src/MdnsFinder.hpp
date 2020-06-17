
#ifndef __MDNS_FINDER_HPP__
#define __MDNS_FINDER_HPP__

#include "Finder.hpp"
#include <asio.hpp>

using asio::ip::tcp;

namespace hippo
{

struct NetcardInfo{
public:
    std::string name;
    std::string ip;
    std::string mask;
    std::string mac;
    std::string gateway;
};

class MdnsFinder:public Finder{
public:
    const std::string DEF_PATTERN = ("TCPIP0::.*::SOCKET");
    MdnsFinder(){
        setPattern(DEF_PATTERN);
    };
    std::list<std::string> availableDevs() const override;
    std::list<std::string> availableDevs(const std::string &pattern) const override;
    std::list<NetcardInfo> availableNetcards() const;
};
typedef std::shared_ptr<MdnsFinder> MdnsFinderPtr;

class MdnsPort:public Port{
public:
    MdnsPort(std::string &res, std::shared_ptr<Finder> finder):Port(res, finder), finder_(std::dynamic_pointer_cast<MdnsFinder>(finder)), url_(res){
    }
    
    bool open() override;
    void close() override;
    int write(const char *data, ssize_t len) override;
    int read(char *data, ssize_t len) override;
    int setOption(int option, const void *data, ssize_t len) override;
    int getOption(int option, void *data, ssize_t len) override;
protected:
    typedef std::shared_ptr<tcp::socket> TCPSocketPtr;

    std::string url_;
    std::string ip;
    std::string port;

    std::shared_ptr<MdnsFinder> finder_;

    asio::io_context io_context;
    TCPSocketPtr sp;
};

}

#endif

