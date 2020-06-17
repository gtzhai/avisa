#include "UsbTmcPort.hpp"
#include <string.h>

namespace hippo
{
#define USB_TMC_HEADER_SIZE 12  /**< length of a USBTMC header */

/* USB TMC status values */
#define USBTMC_STATUS_SUCCESS				0x01
#define USBTMC_STATUS_PENDING				0x02
#define USBTMC_STATUS_FAILED				0x80
#define USBTMC_STATUS_TRANSFER_NOT_IN_PROGRESS		0x81
#define USBTMC_STATUS_SPLIT_NOT_IN_PROGRESS		0x82
#define USBTMC_STATUS_SPLIT_IN_PROGRESS			0x83

/* USB TMC requests values */
#define USBTMC_REQUEST_INITIATE_ABORT_BULK_OUT		1
#define USBTMC_REQUEST_CHECK_ABORT_BULK_OUT_STATUS	2
#define USBTMC_REQUEST_INITIATE_ABORT_BULK_IN		3
#define USBTMC_REQUEST_CHECK_ABORT_BULK_IN_STATUS	4
#define USBTMC_REQUEST_INITIATE_CLEAR			5
#define USBTMC_REQUEST_CHECK_CLEAR_STATUS		6
#define USBTMC_REQUEST_GET_CAPABILITIES			7
#define USBTMC_REQUEST_INDICATOR_PULSE			64

/* USBTMC USB488 Subclass commands (bRequest values, Ref.: Table 9) */
#define READ_STATUS_BYTE             128
#define REN_CONTROL                  160
#define GO_TO_LOCAL                  161
#define LOCAL_LOCKOUT                162


/* USBTMC MsgID. Values, Ref.: Table 2 */
#define DEV_DEP_MSG_OUT              1   /**< device dependent command message */
#define REQUEST_DEV_DEP_MSG_IN       2   /**< command message that requests the device to send a USBTMC response */
#define DEV_DEP_MSG_IN               2   /**< response message to the REQUEST_DEV_DEP_MSG_IN */
#define VENDOR_SPECIFIC_OUT          126 /**< vendor specific command message */
#define REQUEST_VENDOR_SPECIFIC_IN   127 /**< command message that requests the device to send a vendor specific USBTMC response */
#define VENDOR_SPECIFIC_IN           127 /**< response message to the REQUEST_VENDOR_SPECIFIC_IN */


/* bmTransfer attributes */
#define bmTA_EOM       0x01  /**< bmTransfer Attribute: End of Message */
#define bmTA_TERMCHAR  0x02  /**< bmTransfer Attribute: Terminate transfer with Terminate Character */

/* defines for the device capablilities, Ref.: Table 37 and Table 8 USB488 */
#define HAS_INDICATOR_PULSE 0x04
#define TALK_ONLY       0x02
#define LISTEN_ONLY     0x01
#define TERMCHAR_BULKIN 0x01
#define IS_488_2        0x04
#define ACCEPTS_LOCAL_LOCKOUT 0x02
#define TRIGGER         0x01
#define SCPI_COMPILIANT 0x08
#define SR1_CAPABLE     0x04
#define RL1_CAPABLE     0x02
#define DT1_CAPABLE     0x01

typedef struct 
{
	uint8_t USBTMC_status;
	uint8_t reserved0;
	uint8_t bcdUSBTMC_lsb;
	uint8_t bcdUSBTMC_msb;
	uint8_t TMCInterface;
	uint8_t TMCDevice;
	uint8_t reserved1[6];
	/* place here USB488 subclass capabilities */
	uint8_t bcdUSB488_lsb;
	uint8_t bcdUSB488_msb;
	uint8_t USB488Interface;
	uint8_t USB488Device;
	uint8_t reserved2[8];
} USB_TMC_Capabilities;

int UsbTmcPort::get_capabilities(){

    int ret;
    USB_TMC_Capabilities cap;

	ret = libusb_control_transfer(handle_, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
	    USBTMC_REQUEST_GET_CAPABILITIES, 0, interf_, (unsigned char *)&cap, sizeof(cap), timeout_);

    if (ret == sizeof(cap)) {
        if(cap.USBTMC_status == USBTMC_STATUS_SUCCESS)
        {
	        usbtmc_intf_cap_  = cap.TMCInterface;
	        usbtmc_dev_cap_   = cap.TMCDevice;
	        usb488_intf_cap_  = cap.USB488Interface;
	        usb488_dev_cap_   = cap.USB488Device;
            memcpy(capabilities_, &cap, sizeof(cap));
        }
	}

    return ret;
}

bool UsbTmcPort::open(int timeout){

    if(!check())
    {
        printf("%s:can not find the dev\n", __func__); 
        return false;
    }

    for(int c = 0;  c < desc_.bNumConfigurations; c++)
    {
        libusb_config_descriptor *cdesc;

        if(libusb_get_config_descriptor(dev_, c, &cdesc) != LIBUSB_SUCCESS)
        {
            break;
        }

        int f = url_.iface();
        {
            libusb_interface const &intf = cdesc->interface[f];

            for(int as = 0; as < intf.num_altsetting; as++)
            {
                libusb_interface_descriptor const &idesc = intf.altsetting[as];

                if(idesc.bInterfaceClass != LIBUSB_CLASS_APPLICATION)
                        continue;

                if(idesc.bInterfaceSubClass != 0x03)
                        continue;

                int bulk_in_ep = 0;
                int bulk_out_ep = 0;
                int intr_in_ep = 0;

                for(int ep = 0; ep < idesc.bNumEndpoints; ++ep)
                {
                    libusb_endpoint_descriptor const &edesc = idesc.endpoint[ep];

                    if((edesc.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
                    {
                        switch(edesc.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
                        {
                            case LIBUSB_TRANSFER_TYPE_BULK:
                                if(bulk_in_ep == 0)
                                {
                                    bulk_in_ep = edesc.bEndpointAddress;
                                }
                                break;
                            case LIBUSB_TRANSFER_TYPE_INTERRUPT:
                                if(intr_in_ep == 0)
                                {
                                    intr_in_ep = edesc.bEndpointAddress;
                                }
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        switch(edesc.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
                        {
                            case LIBUSB_TRANSFER_TYPE_BULK:
                                if(bulk_out_ep == 0)
                                {
                                    bulk_out_ep = edesc.bEndpointAddress;
                                }
                            default:
                                break;
                        }
                    }
                }

                if(!bulk_in_ep || !bulk_out_ep){
                    continue;
                }

                printf("is a usbtmc dev: c(%d), i(%d), a(%d), bulkin(%d), builkout(%d), int(%d)\n", 
                                cdesc->bConfigurationValue,
                                idesc.bInterfaceNumber,
                                idesc.bAlternateSetting,
                                bulk_in_ep,
                                bulk_out_ep,
                                intr_in_ep
                                );

                libusb_device_handle *handle;
                char string[128];
                int c;
                int ret = 0;

                if(libusb_open(dev_, &handle) != LIBUSB_SUCCESS)
                {
                    printf("%s:libusb open failed\n", __func__);
                    return false;
                }

                printf("1\n");
                libusb_set_auto_detach_kernel_driver(handle, 1);

                if (libusb_get_configuration(handle, &c) == 0
	                && (cdesc->bConfigurationValue != c)) {
		            if ((ret = libusb_set_configuration(handle, cdesc->bConfigurationValue)) < 0) 
                    {
	                    printf("Failed to set configuration: %s.",
	                		       libusb_error_name(ret));
                        return false;
	                }
	            }

                printf("2\n");
	            if ((ret = libusb_claim_interface(handle, idesc.bInterfaceNumber)) < 0) {
	            	printf("Failed to claim interface: %s.",
	            	       libusb_error_name(ret));
                    return false;
	            }
                handle_ = handle;

                printf("3\n");
                get_capabilities();
                config_ = cdesc->bConfigurationValue;
                interf_ = idesc.bInterfaceNumber;
                altset_ = idesc.bAlternateSetting;
                bulk_in_ep_ = bulk_in_ep;
                bulk_out_ep_ = bulk_out_ep;
                intr_ep_ = intr_in_ep;
                libusb_free_config_descriptor(cdesc);
                printf("%s:suc\n", __func__);
                return true;
            }
        }

        libusb_free_config_descriptor(cdesc);
    }
    return false;
}

bool UsbTmcPort::open() {
    return open(timeout_);
}

void UsbTmcPort::close() {
    if(handle_ != nullptr){
        libusb_close(handle_);
        handle_ = nullptr;
    }
}

int UsbTmcPort::bulk_transfer_send(unsigned char *data, int len){
    int actual;
    int ret;
    int total = 0;

    printf("%s:%p:%s, %d\n", __func__, handle_, data, len);

    while(total < len)
    {
        ret = libusb_bulk_transfer(handle_, bulk_out_ep_, data+total, len-total, &actual, timeout_);
        if (ret < 0){
            return ret;
        }else{
            total += actual;
        }
    }

    return total;
}

int UsbTmcPort::write_REQUEST_DEV_DEP_MSG_IN(int transfer_len)
{
    unsigned char buf[USB_TMC_HEADER_SIZE];

	buf[0] = REQUEST_DEV_DEP_MSG_IN;
	buf[1] = bTag_; /* Transfer ID (bTag) */
	buf[2] = ~(bTag_); /* Inverse of bTag */
	buf[3] = 0; /* Reserved */
	buf[4] = transfer_len & 0xff; /* Max transfer (first byte) */
	buf[5] = (transfer_len >> 8) & 0xff; /* Second byte */
	buf[6] = (transfer_len >> 16) & 0xff; /* Third byte */
	buf[7] = (transfer_len >> 24) & 0xff; /* Fourth byte */
	buf[8] = (term_char_enabled_<<1);
	buf[9] = term_char_; /* Term character */
	buf[10] = 0; /* Reserved */
	buf[11] = 0; /* Reserved */

    /* Store bTag (in case we need to abort) */
	usbtmc_last_write_bTag_ = bTag_;
	
	/* Increment bTag -- and increment again if zero */
	bTag_++;
	if (bTag_ == 0)
	    bTag_++;
	
    return bulk_transfer_send(buf, sizeof(buf));
}

int UsbTmcPort::read(char *data, ssize_t len)
{
	int ret, actual, remaining, done, transfer_len, bytes;
	unsigned long int num_of_characters;
    const int buf_size = DEF_BUFFER_SIZE;
    unsigned char *buf = buf_;

	remaining = len;
	done = 0;
	
	while (remaining > 0)
	{
		if (remaining > DEF_DATA_MAX_SIZE)
		{
			transfer_len = DEF_DATA_MAX_SIZE;
		}
		else
		{
			transfer_len = remaining;
		}
		
        ret = write_REQUEST_DEV_DEP_MSG_IN(transfer_len);
		if (ret < 0)
		{
			printf("REQUEST_DEV_DEP_MSG_IN error:%d\n", ret);
			return ret;
		}
	
        ret = libusb_bulk_transfer(handle_, bulk_in_ep_, buf, buf_size, &actual, timeout_);
		/* Store bTag (in case we need to abort) */
		usbtmc_last_read_bTag_ = bTag_;
        if (ret < 0){
            return ret;
        }
	
		/* How many characters did the instrument send? */
		bytes = buf[4] + (buf[5] << 8) + (buf[6] << 16) + (buf[7] << 24);
	
		/* Copy buffer to user space */
		memcpy(data + done, buf + 12, bytes);
		done += bytes;

		if (bytes < transfer_len)
		{
			/* Short package received (less than requested amount of bytes), exit loop */
			remaining = 0;
		}
	}
	
	return done; /* Number of bytes read (total) */
}

int UsbTmcPort::write(const char *data, ssize_t len){

	int ret, n, remaining, done, transfer_len;
	unsigned long int num_of_bytes;
	unsigned char last_transaction;
    unsigned char *buf = buf_;
	
    printf("%s:%s, %d\n", __func__, data, len);
	remaining = len;
	done = 0;
	
	while (remaining > 0) /* Still bytes to send */
	{

		if (remaining > DEF_DATA_MAX_SIZE)
		{
			/* Use maximum size (limited by driver internal buffer size) */
			transfer_len = DEF_DATA_MAX_SIZE; /* Use maximum size */
			last_transaction = 0; /* This is not the last transfer */
		}
		else
		{
			/* Can send remaining bytes in a single transaction */
			transfer_len = remaining;
			last_transaction = 1; /* Message ends w/ this transfer */
		}
		
		/* Setup IO buffer for DEV_DEP_MSG_OUT message */
		buf[0x00] = DEV_DEP_MSG_OUT;
		buf[0x01] = bTag_; /* Transfer ID (bTag) */
		buf[0x02] = ~bTag_; /* Inverse of bTag */
		buf[0x03] = 0; /* Reserved */
		buf[0x04] = transfer_len & 255; /* Transfer size (first byte) */
		buf[0x05] = (transfer_len >> 8) & 255; /* Transfer size (second byte) */
		buf[0x06] = (transfer_len >> 16) & 255; /* Transfer size (third byte) */
		buf[0x07] = (transfer_len >> 24) & 255; /* Transfer size (fourth byte) */
		buf[0x08] = last_transaction; /* 1 = yes, 0 = no */
		buf[0x09] = 0; /* Reserved */
		buf[0x0a] = 0; /* Reserved */
		buf[0x0b] = 0; /* Reserved */
		
		/* Append write buffer (instrument command) to USBTMC message */
		memcpy(&buf[12], data + done, transfer_len);

		/* Add zero bytes to achieve 4-byte alignment */
		num_of_bytes = 12 + transfer_len;
		if (transfer_len % 4)
		{
			num_of_bytes += 4 - transfer_len % 4;
			for (n = 12 + transfer_len; n < num_of_bytes; n++)
				buf[n] = 0;
		}
	
		/* Create pipe and send USB request */
        ret = bulk_transfer_send(buf, num_of_bytes);
	
		/* Store bTag (in case we need to abort) */
		usbtmc_last_write_bTag_ = bTag_;
		
		/* Increment bTag -- and increment again if zero */
		bTag_++;
		if (bTag_ == 0)
			bTag_++;
		
		if (ret < 0)
		{
			return ret;
		}
		
		remaining -= transfer_len;
		done += transfer_len;
	}
	
	return done;
}

int UsbTmcPort::setOption(int option, const void *data, ssize_t len){
    return 0;
}

int UsbTmcPort::getOption(int option, void *data, ssize_t len){
    return 0;
}

}

