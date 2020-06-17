
#ifndef __USB_VISA_SERVICE_HPP__
#define __USB_VISA_SERVICE_HPP__

#include "UsbTmcPort.hpp"
#include "MdnsFinder.hpp"
#include "ComFinder.hpp"
#include <websocketpp/sha1/sha1.hpp>

namespace hippo
{

class VisaURL{
public:
    VisaURL(std::string &res, FinderPtr fp):res_(res), fp_(fp){
        unsigned char hashbuf[20] = {0}; 
        char buf[4] = {0}; 
        websocketpp::sha1::calc(res_.data(), res_.size(), hashbuf);
        sha1_.clear();
        for(int i=0; i<20; i++){
            sprintf(buf, "%x", hashbuf[i]);
            sha1_ += buf; 
        }
    }
    std::string res_;
    std::string sha1_;
    FinderPtr fp_;
};

class VisaConnction{
public:        
    VisaConnction(PortPtr p, VisaURL &url):p_(p), url_(url), ref_(1){};
    int recv(char *data, ssize_t len){return p_->read(data, len);};
    int send(const char *data, ssize_t len) {return p_->write(data, len);};
    int setOption(int option, const void *data, ssize_t len) { return p_->setOption(option, data, len);};
    int getOption(int option, void *data, ssize_t len) {return p_->getOption(option, data, len);};
    bool close(){
        ref_--;
        if(ref_ == 0){ p_->close(); return true;}
        else return false;
    };
    bool isMe(const std::string sha1){
        if(url_.sha1_ == sha1) return true; 
        return false;
    }
    void refMe(){
        ref_++; 
    }
private:
    int ref_;
    PortPtr p_;
    VisaURL url_;
};
typedef std::shared_ptr<VisaConnction> VisaConnctionPtr;

class VisaService{
public:        
    int init();
    std::list<VisaURL> availableDevs(const std::string &pattern) const;

    FinderPtr getFirstFinder(std::string &res);
    VisaConnctionPtr connect(VisaURL &url, int *error);
    int disconnect(VisaConnctionPtr con);
private:
    FinderRepoPtr frp_;
    PortRepoPtr prp_;
};
typedef std::unique_ptr<VisaService> VisaServicePtr;

}
#endif

