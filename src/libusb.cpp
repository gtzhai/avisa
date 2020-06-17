#include <libusb-1.0/libusb.h>
#include <string>
#include <string.h>
#include <list>
#include "spdlog/spdlog.h"

#define USB_TMC_HEADER_SIZE 12  /**< length of a USBTMC header */
#define USB_TMC_DATA_MAX_SIZE 10*1024
#define TMC_BULK_BUFFER_SIZE  (USB_TMC_DATA_MAX_SIZE+USB_TMC_HEADER_SIZE+3) 

typedef struct usb_dev_info{
    int c;
    int i;
    int s;
    int bulk_in_ep;
    int bulk_out_ep;
    int intr_ep;
    uint8_t str[3];

    uint8_t usbtmc_intf_cap;
    uint8_t usbtmc_dev_cap;
    uint8_t usb488_intf_cap;
    uint8_t usb488_dev_cap;

    int usb_tmc_status; 
	uint8_t term_char_enabled;
	uint8_t term_char;
	uint8_t usbtmc_last_write_bTag;
	uint8_t usbtmc_last_read_bTag;
    uint8_t bTag;
    unsigned char buf[TMC_BULK_BUFFER_SIZE];

    int transfer_timeout;
}USBDevInfo;

/*--------------------------------------------------------------------------*/
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

int usbtmc_bulk_transfer_send(libusb_device_handle *handle, USBDevInfo &info, unsigned char *data, int len){
    int actual;
    int ret;
    int total = 0;

    while(total < len)
    {
        ret = libusb_bulk_transfer(handle, info.bulk_out_ep, data+total, len-total, &actual, info.transfer_timeout);
        if (ret < 0){
            return ret;
        }else{
            total += actual;
        }
    }

    return total;
}

ssize_t usbtmc_write_REQUEST_DEV_DEP_MSG_IN(libusb_device_handle *handle, USBDevInfo &info, int transfer_len)
{
    unsigned char buf[USB_TMC_HEADER_SIZE];
    USBDevInfo *ctx = &info;

	buf[0] = REQUEST_DEV_DEP_MSG_IN;
	buf[1] = ctx->bTag; /* Transfer ID (bTag) */
	buf[2] = ~(ctx->bTag); /* Inverse of bTag */
	buf[3] = 0; /* Reserved */
	buf[4] = transfer_len & 0xff; /* Max transfer (first byte) */
	buf[5] = (transfer_len >> 8) & 0xff; /* Second byte */
	buf[6] = (transfer_len >> 16) & 0xff; /* Third byte */
	buf[7] = (transfer_len >> 24) & 0xff; /* Fourth byte */
	buf[8] = (ctx->term_char_enabled<<1);
	buf[9] = ctx->term_char; /* Term character */
	buf[10] = 0; /* Reserved */
	buf[11] = 0; /* Reserved */

    /* Store bTag (in case we need to abort) */
	ctx->usbtmc_last_write_bTag = ctx->bTag;
	
	/* Increment bTag -- and increment again if zero */
	ctx->bTag++;
	if (ctx->bTag == 0)
	    ctx->bTag++;
	
    return usbtmc_bulk_transfer_send(handle, info, buf, sizeof(buf));
}

ssize_t usbtmc_read(libusb_device_handle *handle, USBDevInfo &info, char *data, ssize_t len)
{
	int ret, actual, remaining, done, transfer_len, bytes;
	unsigned long int num_of_characters;
    const int buf_size = TMC_BULK_BUFFER_SIZE;
    unsigned char *buf = info.buf;
    USBDevInfo *ctx = &info;
	

	remaining = len;
	done = 0;
	
	while (remaining > 0)
	{

		if (remaining > USB_TMC_DATA_MAX_SIZE)
		{
			transfer_len = USB_TMC_DATA_MAX_SIZE;
		}
		else
		{
			transfer_len = remaining;
		}
		
        ret = usbtmc_write_REQUEST_DEV_DEP_MSG_IN(handle, info, transfer_len);
		if (ret < 0)
		{
			printf("REQUEST_DEV_DEP_MSG_IN error:%d\n", ret);
			return ret;
		}
	
        ret = libusb_bulk_transfer(handle, info.bulk_in_ep, buf, buf_size, &actual, info.transfer_timeout);
		/* Store bTag (in case we need to abort) */
		ctx->usbtmc_last_read_bTag = ctx->bTag;
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

ssize_t usbtmc_write(libusb_device_handle *handle, USBDevInfo &info, const char *data, ssize_t len)
{
	int ret, n, remaining, done, transfer_len;
	unsigned long int num_of_bytes;
	unsigned char last_transaction;
    unsigned char *buf = info.buf;
    USBDevInfo *ctx = &info;
	
	remaining = len;
	done = 0;
	
	while (remaining > 0) /* Still bytes to send */
	{

		if (remaining > USB_TMC_DATA_MAX_SIZE)
		{
			/* Use maximum size (limited by driver internal buffer size) */
			transfer_len = USB_TMC_DATA_MAX_SIZE; /* Use maximum size */
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
		buf[0x01] = ctx->bTag; /* Transfer ID (bTag) */
		buf[0x02] = ~ctx->bTag; /* Inverse of bTag */
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
        ret = usbtmc_bulk_transfer_send(handle, info, buf, num_of_bytes);
	
		/* Store bTag (in case we need to abort) */
		ctx->usbtmc_last_write_bTag = ctx->bTag;
		
		/* Increment bTag -- and increment again if zero */
		ctx->bTag++;
		if (ctx->bTag == 0)
			ctx->bTag++;
		
		if (ret < 0)
		{
			return ret;
		}
		
		remaining -= transfer_len;
		done += transfer_len;
	}
	
	return done;
}


int usbtmc_dev_get_capabilities(libusb_device_handle *handle, USBDevInfo &info){
    int ret;
    USB_TMC_Capabilities cap;

	ret = libusb_control_transfer(handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
	    USBTMC_REQUEST_GET_CAPABILITIES, 0, info.i, (unsigned char *)&cap, sizeof(cap), info.transfer_timeout);

    if (ret == sizeof(cap)) {
        if(cap.USBTMC_status == USBTMC_STATUS_SUCCESS)
        {
	        info.usbtmc_intf_cap  = cap.TMCInterface;
	        info.usbtmc_dev_cap   = cap.TMCDevice;
	        info.usb488_intf_cap  = cap.USB488Interface;
	        info.usb488_dev_cap   = cap.USB488Device;
        }
	}

    return ret;
}

bool test_usbtmc_dev(libusb_device *dev, USBDevInfo &info)
{
        libusb_device_handle *handle;
        char string[128];
        int c;
        int ret = 0;

        if(libusb_open(dev, &handle) != LIBUSB_SUCCESS)
            return false;

        libusb_set_auto_detach_kernel_driver(handle, 1);

        if (libusb_get_configuration(handle, &c) == 0
	        && (info.c != c)) {
		    if ((ret = libusb_set_configuration(handle, info.c)) < 0) 
            {
	            printf("Failed to set configuration: %s.",
	        		       libusb_error_name(ret));
                return false;
	        }
	    }

	    if ((ret = libusb_claim_interface(handle, info.i)) < 0) {
	    	printf("Failed to claim interface: %s.",
	    	       libusb_error_name(ret));
            return false;
	    }

	    /*if ((ret = libusb_set_interface_alt_setting(handle, info.i, info.s)) < 0) {
	    	printf("Failed to set interface alt: %s.",
	    	       libusb_error_name(ret));
            return false;
	    }*/


        //get string desc
        for (int i=0; i<3; i++) {
	        if (info.str[i] == 0) {
	            continue;
	        }

	        if (libusb_get_string_descriptor_ascii(handle, info.str[i], (unsigned char*)string, sizeof(string)) > 0) {
	            printf("   String (0x%02X): \"%s\"\n", info.str[i], string);
	        }
	    }

        usbtmc_dev_get_capabilities(handle, info);

        const char *str = "*IDN?\n";
        char buf[256] = {0};
        usbtmc_write(handle, info, str, strlen(str));
        usbtmc_read(handle, info, buf, sizeof(buf));
        printf("idn is: %s\n", buf);

        libusb_close(handle);
        return true;
}
/*--------------------------------------------------------------------------*/

static void for_each_usb_devs(std::list<std::string>& list, libusb_device **devs) {

    libusb_device *dev;
    int i = 0, j = 0;
	uint8_t path[8]; 
    uint8_t string_index[3];

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
                    if(idesc.bInterfaceClass != LIBUSB_CLASS_APPLICATION)
                            continue;

                    if(idesc.bInterfaceSubClass != 0x03)
                            continue;

                    if(idesc.bInterfaceProtocol > 0x01)
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
                    string_index[0] = desc.iManufacturer;
	                string_index[1] = desc.iProduct;
	                string_index[2] = desc.iSerialNumber;

                    USBDevInfo info;
                    info.bulk_in_ep = bulk_in_ep;
                    info.bulk_out_ep = bulk_out_ep;
                    info.intr_ep = intr_in_ep;
                    info.c = cdesc->bConfigurationValue;
                    info.i = idesc.bInterfaceNumber;
                    info.s = idesc.bAlternateSetting;
                    memcpy(info.str, string_index, sizeof(string_index));
                    info.transfer_timeout = 1000;

                    test_usbtmc_dev(dev, info);
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

std::list<std::string> availableUsbDevice(){

    libusb_context *ctx;
    std::list<std::string> list;

    if(libusb_init(&ctx) != LIBUSB_SUCCESS){
		fprintf(stderr, "libusb init error\n");
        return list; 
    }

    libusb_device **devs = nullptr;
    ssize_t cnt = libusb_get_device_list(ctx, &devs);
    if(cnt <= 0){
		fprintf(stderr, "get device error\n");
        libusb_exit(ctx); 
        return list;
    }

    for_each_usb_devs(list, devs);

    libusb_free_device_list(devs, 1);
    libusb_exit(ctx); 

    return list;
}

