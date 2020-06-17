#include "ResManager.hpp"

namespace hippo
{
    ::std::map<ViSession, ResManagerPtr> ResManagerMaker::repo;

    ViStatus ResManager::open(){
        if(smp_ == nullptr){
            smp_ = ::std::make_shared<SessionManager>(id_, url_);   
            if(smp_ != nullptr){
                return VI_SUCCESS;
            }
            return VI_ERROR_SYSTEM_ERROR;
        }
        else{
            return VI_SUCCESS;
        }
    }

    ViStatus ResManager::FindRsrc(ViString expr, ViPFindList vi, ViPUInt32 retCnt, ViChar _VI_FAR desc[]){
        ::std::string pattern(expr);

        ResList rl = smp_->availableDevs(pattern);
        if(rl.list.size() > 0)
        {
            rl.iterator = rl.list.begin();

            if(retCnt != NULL)
            {
                *retCnt = rl.list.size(); 
            }

            if(desc != NULL)
            {
                auto res = *(rl.iterator);
                strcpy(desc, res.resStr().c_str()); 
                rl.iterator++;
            }

            if(vi != NULL){
                uint32_t id = genFindListId();
                repo_fl.insert(::std::pair<ViFindList, ResList>(id, rl)); 
                *vi = id;
            }
            return VI_SUCCESS;
        }

        return VI_ERROR_RSRC_NFOUND;
    }

    ViStatus ResManager::FindNext(ViFindList vi, ViChar _VI_FAR desc[]){
        auto rli = repo_fl.find(vi);
        if(rli != repo_fl.end()) 
        {
            ResList rl = rli->second;
            if(desc != NULL)
            {
                auto res = *(rl.iterator);
                strcpy(desc, res.resStr().c_str()); 
                rl.iterator++;
            }
            return VI_SUCCESS;
        }
        return VI_ERROR_RSRC_NFOUND;
    }

    ViStatus ResManager::CloseFindList(ViFindList vi){
        repo_fl.erase(vi);
        return VI_SUCCESS;
    }

    ViStatus ResManager::ParseRsrc(ViRsrc rsrcName, ViPUInt16 intfType,
                       ViPUInt16 intfNum, ViChar _VI_FAR rsrcClass[],
                       ViChar _VI_FAR expandedUnaliasedName[],
                       ViChar _VI_FAR aliasIfExists[]){
        ::std::smatch result;
        ::std::string res(rsrcName);
        
        // USB INSTR Resource:
        // USB[board]::manufacturer ID::model code::serial number[::USB interface number][::INSTR]
        // "USB5::0x03eb::0x2423::123456"
        ::std::regex regexUSB("USB(\\d*)::0x([0-9a-fA-F]{4})::0x([0-9a-fA-F]{4})::([\\d\\-.a-zA-Z]+)(|::\\d+)(|::INSTR)");
        if (std::regex_search(res.cbegin(), res.cend(), result, regexUSB)) {
          *intfType = VI_INTF_USB;
          *intfNum = 0;
          std::stringstream ssBoard(result[1]);
          std::stringstream ssUsbVendorId(result[2]);
          std::stringstream ssUsbDeviceId(result[3]);
          std::stringstream ssUsbSerial(result[4]);
          std::stringstream ssUsbInterfaceNum(result[5]);
          ssBoard >> *intfNum;
          if(rsrcClass) strcpy(rsrcClass, "INSTR");
          if(expandedUnaliasedName) strcpy(expandedUnaliasedName, rsrcName);
          if(aliasIfExists) aliasIfExists[0] = 0;
          return VI_SUCCESS;
        }
        
        // TCPIP SOCKET Resource:
        // TCPIP[board]::host address[::LAN device name][::SOCKET]
        // TCPIP::127.0.0.1
        std::regex regexTCPIP("TCPIP(\\d*)::([0-9.a-zA-Z\\-_]+)(|::([0-9.a-zA-Z\\-_]+))(|::SOCKET)");
        if (std::regex_search(res.cbegin(), res.cend(), result, regexTCPIP)) {
          *intfType = VI_INTF_TCPIP;
          *intfNum = 0;
          std::stringstream ssBoard(result[1]);
          std::stringstream ssHostAddress(result[2]);
          std::stringstream ssHostPort(result[3]);
          ssBoard >> *intfNum;
          if(rsrcClass) strcpy(rsrcClass, "INSTR");
          if(expandedUnaliasedName) strcpy(expandedUnaliasedName, rsrcName);
          if(aliasIfExists) aliasIfExists[0] = 0;
          return VI_SUCCESS;
        }

        // COM INSTR Resource:
        std::regex regexCOM("ASRL(\\d*)::INSTR.*");
        if (std::regex_search(res.cbegin(), res.cend(), result, regexTCPIP)) {
          *intfType = VI_INTF_ASRL;
          *intfNum = 0;
          std::stringstream ssBoard(result[1]);
          ssBoard >> *intfNum;
          if(rsrcClass) strcpy(rsrcClass, "INSTR");
          if(expandedUnaliasedName) strcpy(expandedUnaliasedName, rsrcName);
          if(aliasIfExists) aliasIfExists[0] = 0;
          return VI_SUCCESS;
        }
  
        return VI_ERROR_INV_RSRC_NAME;
    }

    ViStatus ResManager::OpenSession(ViRsrc name, ViAccessMode mode, ViUInt32 timeout, ViPSession vi){
        int error = 0;
        Resource res(name);
        ViSession s = smp_->connect(res, &error);

        if(error < 0) return error;

        if(vi != NULL) *vi = s;
        return VI_SUCCESS;
    }

    ViStatus ResManager::CloseSession(ViSession vi){
        return smp_->disconnect(vi);
    }

    SessionPtr ResManager::GetSession(ViSession vi){
        return smp_->getSession(vi);
    }
}

