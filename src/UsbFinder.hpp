#ifndef __USB_HPP__
#define __USB_HPP__

#include "Finder.hpp"
#include <string.h>
#include <libusb-1.0/libusb.h>

namespace hippo
{

class UsbURL{
public:
    UsbURL(std::string &url): valid_(false)
    {
        reset(url);
    }

    UsbURL(uint16_t p, uint16_t v, uint16_t i, const std::string &s): valid_(false){
        if((p != 0) && (v != 0) && (i != 0) && (s.size() > 0)){
            pid_ = p;    
            vid_ = v;
            iface_ = i;
            serial_ = s;
        }
    }
    ~UsbURL() = default;

    void reset(std::string &url){
    
        const char *str = url.c_str(); 
        int p;
        int v;
        int i;
        int r;
        char buf[256];


        printf("url is %s\n", str); 
        valid_ = false; 
        r = sscanf(str, "USB0::0x%x::0x%x::%s::%d::INSTR", &v, &p, buf, &i);
        if(r == 3){
            char *tmp = nullptr; 

            valid_ = true; 
            pid_ = p;
            vid_ = v;
            tmp = strstr(buf, "::");
            if(tmp != NULL)
            {
                serial_ = std::string(buf, tmp - buf); 
                r = sscanf(tmp, "::%d::INSTR", &i);
                if(r ==1){
                    iface_ = i;
                }
            }
        }
        else{
            printf("url parse error\n"); 
        }
    }

    uint16_t pid() const{
        return pid_; 
    }
    uint16_t vid() const{
        return vid_; 
    }
    uint16_t iface() const{
        return iface_; 
    }

    std::string serial() const{
        return serial_; 
    }

    std::string url_str() const {

        if(valid_){

            char buf[512];
            sprintf(buf, "USB0::0x%x::0x%x::%s::%d::INSTR", pid_, vid_, serial_.c_str(), iface_);
            return std::string(buf);
        }
        return std::string("");
    }


private:
    uint16_t pid_;
    uint16_t vid_;
    uint16_t iface_;
    std::string serial_;
    bool valid_;
};

/**
 *
 * usb finder
 *
 * find available devs by pattern
 */
class UsbFinder:public Finder{
public:
    const std::string TAIL_FLAG = "TAIL";
    const std::string HEAD_FLAG = "USB0";
    const std::string DELIM_FLAG = "::";
    const std::string DEF_PATTERN="USB0::.*::INSTR";//::TAIL::0xfe::0x03::0x1
    UsbFinder();
    virtual ~UsbFinder();

    std::list<std::string> availableDevs() const;
    std::list<std::string> availableDevs(const std::string &pattern) const;

private:
    bool get_string_descriptor_of_serial(libusb_device *dev, uint8_t serial_string_index, char *desc) const;
    void for_each_usb_devs(std::list<std::string>& list, libusb_device **devs, const std::string &pattern) const;
    libusb_context *ctx;
    friend class UsbPort;
};
typedef std::shared_ptr<UsbFinder> UsbFinderPtr;

/**
 *
 * usb port
 *
 */
class UsbPort:public Port{
public:
    UsbPort(std::string &res, std::shared_ptr<Finder> finder):Port(res, finder), finder_(std::dynamic_pointer_cast<UsbFinder>(finder)), url_(res){};
protected:
    bool check();
    UsbURL url_;
    std::shared_ptr<UsbFinder> finder_;
    libusb_device *dev_;
    struct libusb_device_descriptor desc_;
    libusb_device_handle *handle_;
};

}

#endif

