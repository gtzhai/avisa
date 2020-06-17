#include "ComFinder.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <regex>

namespace hippo
{
#ifdef _WIN32
#include <windows.h>
bool getRegKeyValues(std::string regKeyPath, std::list<std::string> & portsList)
{
	//https://msdn.microsoft.com/en-us/library/ms724256

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

	HKEY hKey;

	TCHAR		achValue[MAX_VALUE_NAME];					// buffer for subkey name
	DWORD		cchValue = MAX_VALUE_NAME;					// size of name string 
	TCHAR		achClass[MAX_PATH] = TEXT("");				// buffer for class name 
	DWORD		cchClassName = MAX_PATH;					// size of class string 
	DWORD		cSubKeys = 0;								// number of subkeys 
	DWORD		cbMaxSubKey;								// longest subkey size 
	DWORD		cchMaxClass;								// longest class string 
	DWORD		cKeyNum;									// number of values for key 
	DWORD		cchMaxValue;								// longest value name 
	DWORD		cbMaxValueData;								// longest value data 
	DWORD		cbSecurityDescriptor;						// size of security descriptor 
	FILETIME	ftLastWriteTime;							// last write time 

	int iRet = -1;
	bool bRet = false;

	std::string m_keyValue;

	TCHAR m_regKeyPath[MAX_KEY_LENGTH];

	TCHAR strDSName[MAX_VALUE_NAME];
	memset(strDSName, 0, MAX_VALUE_NAME);
	DWORD nValueType = 0;
	DWORD nBuffLen = 10;

#ifdef UNICODE
	int iLength;
	const char * _char = regKeyPath.c_str();
	iLength = MultiByteToWideChar(CP_ACP, 0, _char, strlen(_char) + 1, NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, _char, strlen(_char) + 1, m_regKeyPath, iLength);
#else
	strcpy_s(m_regKeyPath, MAX_KEY_LENGTH, regKeyPath.c_str());
#endif

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, m_regKeyPath, 0, KEY_READ, &hKey))
	{
		// Get the class name and the value count. 
		iRet = RegQueryInfoKey(
			hKey,                    // key handle 
			achClass,                // buffer for class name 
			&cchClassName,           // size of class string 
			NULL,                    // reserved 
			&cSubKeys,               // number of subkeys 
			&cbMaxSubKey,            // longest subkey size 
			&cchMaxClass,            // longest class string 
			&cKeyNum,                // number of values for this key 
			&cchMaxValue,            // longest value name 
			&cbMaxValueData,         // longest value data 
			&cbSecurityDescriptor,   // security descriptor 
			&ftLastWriteTime);       // last write time 

		if (!portsList.empty())
		{
			portsList.clear();
		}

		// Enumerate the key values. 
		if (cKeyNum > 0 && ERROR_SUCCESS == iRet)
		{
			for (int i = 0; i < (int)cKeyNum; i++)
			{
				cchValue = MAX_VALUE_NAME;
				achValue[0] = '\0';
				nBuffLen = MAX_KEY_LENGTH;//防止 ERROR_MORE_DATA 234L 错误

				if (ERROR_SUCCESS == RegEnumValue(hKey, i, achValue, &cchValue, NULL, NULL, (LPBYTE)strDSName, &nBuffLen))
				{

#ifdef UNICODE
					int iLen = WideCharToMultiByte(CP_ACP, 0, strDSName, -1, NULL, 0, NULL, NULL);
					char* chRtn = new char[iLen * sizeof(char)];
					WideCharToMultiByte(CP_ACP, 0, strDSName, -1, chRtn, iLen, NULL, NULL);
					m_keyValue = std::string(chRtn);
					delete chRtn;
					chRtn = NULL;
#else
					m_keyValue = std::string(strDSName);
#endif
					portsList.push_back(m_keyValue);
				}
			}
		}
		else
		{

		}
	}

	if (portsList.empty())
	{
		bRet = false;
	}
	else
	{
		bRet = true;
	}


	RegCloseKey(hKey);

	return bRet;
}

static std::list<std::string> getComList()
{
    std::list<std::string> list;
	std::list<std::string> comList;

	std::string m_regKeyPath = std::string("HARDWARE\\DEVICEMAP\\SERIALCOMM");
	p_serialPortInfoWinBase->getRegKeyValues(m_regKeyPath, comList);

    for(auto i:comList){
        std::string::size_type n;

        if((n=i.find_first_of("COM")) != std::string::npos)  
        {
            std::string nums = i.substr(3); 
            list.push_back("ASRL"+nums+"::INSTR");
        }
    }

	return list;
}

std::string ComFinder::res2Dev(const std::string &res){
    std::smatch result;

    if (std::regex_search(res.cbegin(), res.cend(), result, std::regex("ASRL([0-9.]{1,})::"))) {
        std::string dev = result[1];
        return "COM"+dev;
    } else {
        std::cout <<"res parser error"<<std::endl;
        return std::string();
    }
}

#else
#include <dirent.h>   //scandir
#include <sys/types.h>
#include <sys/stat.h>   //S_ISLNK
#include <unistd.h>     // readlink close

#include <string.h>     //basename memset strcmp

#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>      //ioctl
#include <linux/serial.h> //struct serial_struct
static std::string get_driver(const std::string& tty)
{
    struct stat st;
    std::string devicedir = tty;

    // Append '/device' to the tty-path
    ///sys/class/tty/ttyS0/device
    devicedir += "/device";

    // Stat the devicedir and handle it if it is a symlink
    if (lstat(devicedir.c_str(), &st)==0 && S_ISLNK(st.st_mode))
    {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        // Append '/driver' and return basename of the target
        // VCs, ptys, etc don't have /sys/class/tty/<tty>/device/driver's
        // /sys/class/tty/ttyS0/device/driver
        devicedir += "/driver";

        if (readlink(devicedir.c_str(), buffer, sizeof(buffer)) > 0)
        {
            //std::cout << "readlink " << devicedir << ", baseName " << basename(buffer) << std::endl;

            return basename(buffer);
        }
        else
        {
            //std::cout << "readlink error " << devicedir << std::endl;
        }
    }
    else
    {
        //std::cout << "lstat error " << devicedir << std::endl;
    }
    return "";
}

static void register_comport( std::list<std::string>& comList, std::list<std::string>& comList8250, const std::string& dir) {
    // Get the driver the device is using
    std::string driver = get_driver(dir);

    // Skip devices without a driver
    if (driver.size() > 0) {
        std::string devfile = std::string("/dev/") + basename(dir.c_str());

        //std::cout << "driver : " << driver << ", devfile : " << devfile << std::endl;

        // Put serial8250-devices in a seperate list
        if (driver == "serial8250") {
            comList8250.push_back(devfile);
        }
        else
        {
            std::cout << "driver : " << driver << ", devfile : " << devfile << std::endl;
            comList.push_back(devfile);
        }
    }
}

static void probe_serial8250_comports(std::list<std::string>& comList, std::list<std::string> comList8250) {
    struct serial_struct serinfo;
    std::list<std::string>::iterator it = comList8250.begin();

    // Iterate over all serial8250-devices
    while (it != comList8250.end()) {

        // Try to open the device
        int fd = open((*it).c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);

        if (fd >= 0) {
            // Get serial_info
            if (ioctl(fd, TIOCGSERIAL, &serinfo)==0) {
                // If device type is no PORT_UNKNOWN we accept the port
                if (serinfo.type != PORT_UNKNOWN)
                {
                    comList.push_back(*it);
                    //std::cout << "+++  " << *it << std::endl;
                }
                else
                {
                    //std::cout << "PORT_UNKNOWN " << *it << std::endl;
                }
            }
            close(fd);
        }
        it ++;
    }
}
// ttyS*    // Standard UART 8250 and etc
// ttyO*    // OMAP UART 8250 and etc
// ttyUSB*  // Usb/serial converters PL2303 and etc
// ttyACM*  // CDC_ACM converters (i.e. Mobile Phones)
// ttyGS*   // Gadget serial device (i.e. Mobile Phones with gadget serial driver)
// ttyMI*   // MOXA pci/serial converters
// ttymxc*  // Motorola IMX serial ports (i.e. Freescale i.MX)
// ttyAMA*  // AMBA serial device for embedded platform on ARM (i.e. Raspberry Pi)
// ttyTHS*  // Serial device for embedded platform on ARM (i.e. Tegra Jetson TK1)
// rfcomm*  // Bluetooth serial device
// ircomm*  // IrDA serial device
// tnt*     // Virtual tty0tty serial device
// pts/*    // Virtual pty serial device
static std::list<std::string> getComList()
{
    int n;
    struct dirent **namelist;
    std::list<std::string> list;
    std::list<std::string> comList;
    std::list<std::string> comList8250;
    const char* sysdir = "/sys/class/tty/";

    // Scan through /sys/class/tty - it contains all tty-devices in the system
    n = scandir(sysdir, &namelist, NULL, NULL);
    if (n > 0)
    {
        while (n--)
        {
            if (strcmp(namelist[n]->d_name,"..") && strcmp(namelist[n]->d_name,"."))
            {
                // Construct full absolute file path
                std::string devicedir = sysdir;
                devicedir += namelist[n]->d_name;

                // Register the device
                register_comport(comList, comList8250, devicedir);
            }
            free(namelist[n]);
        }
        free(namelist);

        // Only non-serial8250 has been added to comList without any further testing
        // serial8250-devices must be probe to check for validity
        probe_serial8250_comports(comList, comList8250);
    }
    else
    {
        perror("scandir");
    }

    for(auto i:comList){
        std::string::size_type n;

        if((n=i.find_first_of("ttyS")) != std::string::npos)  
        {
            std::string nums = i.substr(4); 
            list.push_back("ASRL"+nums+"::INSTR");
        }
        else if((n=i.find_first_of("ttyO")) != std::string::npos)  
        {
            std::string nums = i.substr(4); 
            list.push_back("ASRL"+nums+"::INSTR::OMAP");
        }
        else if((n=i.find_first_of("ttyUSB")) != std::string::npos)  
        {
            std::string nums = i.substr(6); 
            list.push_back("ASRL"+nums+"::INSTR::USB");
        }
        else if((n=i.find_first_of("ttyACM")) != std::string::npos)  
        {
            std::string nums = i.substr(6); 
            list.push_back("ASRL"+nums+"::INSTR::ACM");
        }
        else if((n=i.find_first_of("ttyGS")) != std::string::npos)  
        {
            std::string nums = i.substr(5); 
            list.push_back("ASRL"+nums+"::INSTR::GS");
        }
    }

    return list;
}

std::string ComFinder::res2Dev(const std::string &res){
    std::smatch result;

    if (std::regex_search(res.cbegin(), res.cend(), result, std::regex("ASRL([0-9.]{1,})::"))) {
        std::string num = result[1];
        std::string::size_type n;
        std::string dev("/dev/");

        if((n=res.find_first_of("::INSTR::OMAP")) != std::string::npos)  
        {
            dev = dev + "ttyO" + num;
        }
        else if((n=res.find_first_of("INSTR::USB")) != std::string::npos)  
        {
            dev = dev + "ttyUSB" + num;
        }
        else if((n=res.find_first_of("INSTR::ACM")) != std::string::npos)  
        {
            dev = dev + "ttyACM" + num;
        }
        else if((n=res.find_first_of("INSTR::GS")) != std::string::npos)  
        {
            dev = dev + "ttyGS" + num;
        }
        else{
            dev = dev + "ttyS" + num;
        }
        return dev;
    } else {
        std::cout <<"res parser error"<<std::endl;
        return std::string();
    }
}
#endif

std::list<std::string>  ComFinder::availableDevs(const std::string &pattern) const{
    std::list<std::string> list = getComList();
    return list;
}

std::list<std::string>  ComFinder::availableDevs() const{
    return availableDevs(DEF_PATTERN);
}
/*----------------------------------------------------------------------------------------*/
bool ComPort::open(){
    asio::error_code ec;
    port.open(ComFinder::res2Dev(url_), ec);
    if(ec){
        return false; 
    }
    port.set_option(asio::serial_port::baud_rate(115200));
	port.set_option(asio::serial_port::flow_control());
	port.set_option(asio::serial_port::parity());
	port.set_option(asio::serial_port::stop_bits());
	port.set_option(asio::serial_port::character_size(8));
    return true;
}

void ComPort::close(){
    if(port.is_open()){
        port.close();
    }
}

int ComPort::write(const char *data, ssize_t len){
    if(port.is_open()){
        return port.write_some(asio::buffer(data, len));
    } else {
        return -1;
    }
}

int ComPort::read(char *data, ssize_t len){
    if(port.is_open()){
        return port.read_some(asio::buffer(data, len));
    } else {
        return -1;
    }
}

int ComPort::setOption(int option, const void *data, ssize_t len){
    return 0;
}

int ComPort::getOption(int option, void *data, ssize_t len){
    return 0;
}

}

