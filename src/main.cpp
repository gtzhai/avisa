#include <string>
#include <list>
#include "VisaWsServer.hpp"
#include "OsService.hpp"

int main(int argc, char **argv){
    const std::string name("AVISA");
    const std::string desc("AVISA WebSocket Server");
    hippo::VisaWsServerPtr vwsp = std::make_shared<hippo::VisaWsServer>(5026);
    int ret = 0;

    ret = vwsp->init();    
    if(0){
        hippo::CallbackMainRun cb = [vwsp](void){
            vwsp->start();    
            return 0;
        };
        hippo::OsServicePtr osp = std::make_shared<hippo::OsService>(name, desc, cb);
        ret = osp->run();
    }else{
        vwsp->start();    
    }
                    
    return ret;
}

