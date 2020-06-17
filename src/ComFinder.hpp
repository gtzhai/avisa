
#ifndef __COM_FINDER_HPP__
#define __COM_FINDER_HPP__

#include "Finder.hpp"
#include <asio.hpp>

namespace hippo
{
class ComFinder:public Finder{
public:
    const std::string DEF_PATTERN = ("ASRL[0-9]{1,}::INSTR(.*)");
    ComFinder(){
        setPattern(DEF_PATTERN);
    };
    std::list<std::string> availableDevs() const override;
    std::list<std::string> availableDevs(const std::string &pattern) const override;
    static std::string res2Dev(const std::string &res);
};
typedef std::shared_ptr<ComFinder> ComFinderPtr;

class ComPort:public Port{
public:
    ComPort(std::string &res, std::shared_ptr<Finder> finder):Port(res, finder), finder_(std::dynamic_pointer_cast<ComFinder>(finder)), url_(res), port(io_context){};
    
    bool open() override;
    void close() override;
    int write(const char *data, ssize_t len) override;
    int read(char *data, ssize_t len) override;
    int setOption(int option, const void *data, ssize_t len) override;
    int getOption(int option, void *data, ssize_t len) override;
protected:
    std::string url_;
    std::shared_ptr<ComFinder> finder_;
    asio::io_context io_context;
    asio::serial_port port;
};

}

#endif

