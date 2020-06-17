#include "UsbFinder.hpp"
#include "spdlog/spdlog.h"
#include <regex>

namespace hippo
{

UsbFinder::UsbFinder(){
    setPattern(DEF_PATTERN);
    if(libusb_init(&ctx) != LIBUSB_SUCCESS){
		fprintf(stderr, "libusb init error\n");
    }
}

UsbFinder::~UsbFinder(){

    if(ctx != nullptr){
        libusb_exit(ctx); 
    }
}

bool UsbFinder::get_string_descriptor_of_serial(libusb_device *dev, uint8_t serial_string_index, char *string) const
{
        libusb_device_handle *handle;
        int c;
        int ret = 0;

        /*need root privage*/
        if((libusb_open(dev, &handle)) != LIBUSB_SUCCESS)
        {
            printf("libusb open failed\n");
            return false;
        }

	    if(libusb_get_string_descriptor_ascii(handle, serial_string_index, (unsigned char*)string, sizeof(string)) > 0) {
	            printf("   String (0x%02X): \"%s\"\n", serial_string_index, string);
        }

        libusb_close(handle);
        return true;
}

void UsbFinder::for_each_usb_devs(std::list<std::string>& list, libusb_device **devs, const std::string &pattern) const {

    libusb_device *dev;
    int i = 0, j = 0;
	uint8_t path[8]; 
    uint8_t string_index[3];
    uint8_t dst_ic = LIBUSB_CLASS_APPLICATION;
    uint8_t dst_isc = 0x03;
    uint8_t dst_ip  = 0x01;
    std::string res_str;

    std::string::size_type n;

    n = pattern.rfind(TAIL_FLAG);
    if (n != std::string::npos) {
        uint8_t tmp[3] = {0};
        std::string tail = pattern.substr(n+TAIL_FLAG.size()+DELIM_FLAG.size());
        std::regex word_regex(DELIM_FLAG);
        auto words_begin = std::sregex_iterator(tail.begin(), tail.end(), word_regex);
        auto words_end   = std::sregex_iterator();
 
        for (std::sregex_iterator iter = words_begin; iter != words_end; ++iter, i++) {
            std::smatch match = *iter;
            std::string match_str = match.str();
            if (match_str.size() > 2) {
                auto val = std::stoul(match_str, nullptr, 0);
                tmp[i] = val;
            }else{
                tmp[i] = 0;
            }
        }

        dst_ic = tmp[0];
        dst_isc = tmp[1];
        dst_ip = tmp[2];
        res_str = pattern.substr(0, n-DELIM_FLAG.size());
    }
    else{
        res_str = pattern;
    }

    printf("dst:%x,%x,%x\n", dst_ic, dst_isc, dst_ip);

    i = 0;
	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return;
		}

		printf("%04x:%04x (bus %d, device %d)",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));

        string_index[0] = desc.iManufacturer;
	    string_index[1] = desc.iProduct;
	    string_index[2] = desc.iSerialNumber;


        for(int c = 0;  c < desc.bNumConfigurations; c++)
        {
            libusb_config_descriptor *cdesc;

            if(libusb_get_config_descriptor(dev, c, &cdesc) != LIBUSB_SUCCESS)
            {
                break;
            }

            for(int f = 0; (f < cdesc->bNumInterfaces); f++)
            {
                libusb_interface const &intf = cdesc->interface[f];

                for(int as = 0; as < intf.num_altsetting; as++)
                {
                    libusb_interface_descriptor const &idesc = intf.altsetting[as];

                    printf("0x%x, 0x%x, 0x%x\n", idesc.bInterfaceClass, idesc.bInterfaceSubClass, idesc.bInterfaceProtocol);
                    if(dst_ic != 0)
                    {
                        if(idesc.bInterfaceClass != dst_ic)
                                continue;

                        if(dst_isc != 0)
                        {
                            if(idesc.bInterfaceSubClass != dst_isc)
                                    continue;
                        }
                    }

                    std::cout<<"basic mattch:"<<std::endl;
                    char serial[128];
                    if(get_string_descriptor_of_serial(dev, string_index[2], serial)){
                        char buf[512] = {0};

                        sprintf(buf, "USB0::0x%x::0x%x::%s::%d::INSTR", desc.idVendor, desc.idProduct, serial, idesc.bInterfaceNumber);
                        std::string dev_res(buf);

                        std::cout<<"dev res is :"<<dev_res<<std::endl;
                        std::cout<<"pattern is :"<<res_str<<std::endl;
                        std::regex regex(res_str);
                        if(std::regex_match(dev_res, regex)){
                            list.push_back(dev_res);
                        }
                        break;
                    }
                }
            }

            libusb_free_config_descriptor(cdesc);
        }
        

		r = libusb_get_port_numbers(dev, path, sizeof(path));
		if (r > 0) {
			printf(" path: %d", path[0]);
			for (j = 1; j < r; j++)
				printf(".%d", path[j]);
		}
		printf("\n");
	}
}

std::list<std::string> UsbFinder::availableDevs(const std::string &pattern) const{
    std::list<std::string> list;
    libusb_device **devs = nullptr;

    ssize_t cnt = libusb_get_device_list(ctx, &devs);
    if(cnt <= 0){
		fprintf(stderr, "get device error\n");
        return list;
    }

    for_each_usb_devs(list, devs, pattern);

    libusb_free_device_list(devs, 1);

    return list;
}

std::list<std::string> UsbFinder::availableDevs() const{
    return availableDevs(DEF_PATTERN);
}

/*---------------------------------------------------------------------------------*/
bool UsbPort::check(){

    libusb_device **devs = nullptr;
    libusb_device *dev;
    int i = 0, j = 0;
	uint8_t path[8]; 
    uint8_t string_index[3];
    uint8_t dst_ic = LIBUSB_CLASS_APPLICATION;
    uint8_t dst_isc = 0x03;
    uint8_t dst_ip  = 0x01;
    libusb_context *ctx = finder_->ctx;

    ssize_t cnt = libusb_get_device_list(ctx, &devs);
    if(cnt <= 0){
		fprintf(stderr, "get device error\n");
        return false;
    }


    struct libusb_device_descriptor desc;

	while ((dev = devs[i++]) != NULL) {
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return false;
		}

		printf("check:%04x:%04x (bus %d, device %d)\n",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));

        string_index[0] = desc.iManufacturer;
	    string_index[1] = desc.iProduct;
	    string_index[2] = desc.iSerialNumber;

        printf("check:%04x:%04x,%s\n", url_.vid(), url_.pid(), url_.serial().c_str());
        if(desc.idVendor==url_.vid() && desc.idProduct==url_.pid())
        {
            char serial[256] = {0};

            if(finder_->get_string_descriptor_of_serial(dev, string_index[2], serial)){

                printf("check:%s\n", serial);
                if(url_.serial().compare(serial) == 0){
                    dev_ = dev;
                    desc_ = desc;
                    return true;
                }
            }
        }
	}

    return false;
}
/*---------------------------------------------------------------------------------*/

}

