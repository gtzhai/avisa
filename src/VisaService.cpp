#include "VisaService.hpp"

namespace hippo
{
    int VisaService::init(){
        frp_ = std::make_unique<FinderRepo>();
        prp_ = std::make_unique<PortRepo>();

        if(frp_==nullptr || prp_==nullptr){
            return -1;
        }

        /*usbtmc*/
        FinderPtr fp = std::static_pointer_cast<Finder>(std::make_shared<UsbFinder>());
        if(fp != nullptr){
            frp_->registerFinder(std::string("USBTMC"), fp);
            prp_->registerPort(fp->pattern(), [](std::string &res, FinderPtr finder){
                                return std::static_pointer_cast<Port>(std::make_shared<UsbTmcPort>(res, finder));
                            });
        }

        /*tcpip mdns*/
        fp = std::static_pointer_cast<Finder>(std::make_shared<MdnsFinder>());
        if(fp != nullptr){
            frp_->registerFinder(std::string("MDNS"), fp);
            prp_->registerPort(fp->pattern(), [](std::string &res, FinderPtr finder){
                                return std::static_pointer_cast<Port>(std::make_shared<MdnsPort>(res, finder));
                            });
        }

        /*com*/
        fp = std::static_pointer_cast<Finder>(std::make_shared<ComFinder>());
        if(fp != nullptr){
            frp_->registerFinder(std::string("COM"), fp);
            prp_->registerPort(fp->pattern(), [](std::string &res, FinderPtr finder){
                                return std::static_pointer_cast<Port>(std::make_shared<ComPort>(res, finder));
                            });
        }
        return 0;
    }

    std::list<VisaURL> VisaService::availableDevs(const std::string &pattern) const{
        std::list<VisaURL> list;
        auto fl = frp_->searchFinder(pattern);

        std::cout <<__func__<<fl.size()<<std::endl;
        if(fl.size() > 0)
        {
            for(auto i : fl){
                auto ls = i->availableDevs(pattern);       
                for(auto j : ls){
                    list.push_back(VisaURL(j, i)); 
                }
            }   
        }

        return list;
    }

    FinderPtr VisaService::getFirstFinder(std::string &res){
        auto fl = frp_->searchFinderWithRes(res);
        std::cout <<__func__<<fl.size()<<std::endl;
        if(fl.size() <= 0) return nullptr;
        return fl.front();
    }

    VisaConnctionPtr VisaService::connect(VisaURL &url, int *error){
        auto pp = prp_->getPort(url.res_, url.fp_);

        std::cout <<__func__<<url.res_<<std::endl;
        *error = 0;
        if(pp != nullptr){
            auto vcp = std::make_shared<VisaConnction>(pp, url); 
            pp->open();
            return vcp;
        }
        return nullptr;
    }

    int VisaService::disconnect(VisaConnctionPtr con){
        con->close(); 
        return 0;
    }
}

