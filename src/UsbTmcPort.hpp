#ifndef __USB_TMC_PORT_HPP__
#define __USB_TMC_PORT_HPP__

#include "UsbFinder.hpp"

namespace hippo
{
/**
 *
 * usb tmc port
 *
 */
class UsbTmcPort:public UsbPort{
public:
    const int DEF_TIMEOUT=1000;
    static const int DEF_DATA_MAX_SIZE =8192;
    static const int DEF_BUFFER_SIZE=DEF_DATA_MAX_SIZE+12+3;
    static constexpr const char * DEF_PATTERN="USB0::*::*::*::*::INSTR::TAIL::0xfe::0x03::0x1";

    UsbTmcPort(std::string &res, std::shared_ptr<Finder> finder):UsbPort(res, finder), timeout_(DEF_TIMEOUT){};
    bool open() override;
    bool open(int timeout);
    void close() override;
    int write(const char *data, ssize_t len) override;
    int read(char *data, ssize_t len) override;
    int setOption(int option, const void *data, ssize_t len) override;
    int getOption(int option, void *data, ssize_t len) override;

private:
    int bulk_transfer_send(unsigned char *data, int len);
    int write_REQUEST_DEV_DEP_MSG_IN(int transfer_len);
    int get_capabilities();
    int bulk_in_ep_;
    int bulk_out_ep_;
    int intr_ep_;
    uint8_t config_;
    uint8_t interf_;
    uint8_t altset_;
    int timeout_;
    uint8_t capabilities_[24];

    uint8_t usbtmc_intf_cap_;
    uint8_t usbtmc_dev_cap_;
    uint8_t usb488_intf_cap_;
    uint8_t usb488_dev_cap_;

    int usb_tmc_status_; 
	uint8_t term_char_enabled_;
	uint8_t term_char_;
	uint8_t usbtmc_last_write_bTag_;
	uint8_t usbtmc_last_read_bTag_;
    uint8_t bTag_;
    unsigned char buf_[DEF_BUFFER_SIZE];
};
typedef std::shared_ptr<UsbTmcPort> UsbTmcPortPtr;
}

#endif

