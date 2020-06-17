
#ifndef __RES_MANAGER_HPP__
#define __RES_MANAGER_HPP__

#include "basic.hpp"
#include "Resource.hpp"
#include "IdManager.hpp"
#include "SessionManager.hpp"

namespace hippo
{
class ResManager;
typedef ::std::shared_ptr<ResManager> ResManagerPtr;
class ResManager{
public:
    ResManager(uint32_t id, ::std::string &url):url_(url), id_(id){};

    ViStatus open();
    ViStatus FindRsrc(ViString expr, ViPFindList vi, ViPUInt32 retCnt, ViChar _VI_FAR desc[]);
    ViStatus FindNext(ViFindList vi, ViChar _VI_FAR desc[]);
    ViStatus ParseRsrc(ViRsrc rsrcName, ViPUInt16 intfType, ViPUInt16 intfNum, 
                       ViChar _VI_FAR rsrcClass[],
                       ViChar _VI_FAR expandedUnaliasedName[],
                       ViChar _VI_FAR aliasIfExists[]);

    ViStatus OpenSession(ViRsrc name, ViAccessMode mode, ViUInt32 timeout, ViPSession vi);
    ViStatus CloseSession(ViSession vi);
    SessionPtr GetSession(ViSession vi);
    ViStatus CloseFindList(ViFindList vi);
    ::std::string url(){
        return url_;
    }
private:
    uint32_t genFindListId(){
        uint32_t tmp = id_;
        tmp <<= 16;
        fl_cnt++; 
        if(fl_cnt > 65535) fl_cnt = 0;
        return ((tmp+fl_cnt)|IdGen::ID_FINDLIST);
    }
    uint16_t fl_cnt;
    SessionManagerPtr smp_;
    ::std::map<ViFindList, ResList> repo_fl;
    ::std::string url_;
    uint32_t id_;
};

class ResManagerMaker{
public:
    static ResManagerPtr getResManager(ViPSession vi, ::std::string &url, ViStatus *status){
        for(auto i:repo){
            auto s = i.first; 
            auto r = i.second; 
            if(r->url() == url){
                *vi = s;
                *status = VI_SUCCESS;
                return r;
            }
        }

        uint32_t id = IdGen::genResManId();
        ResManagerPtr rmp = ::std::make_shared<ResManager>(id, url);
        if(rmp != nullptr){
            repo.insert(::std::pair<ViSession, ResManagerPtr>(id, rmp)); 
            *vi = id;
            *status = VI_SUCCESS;
        }
        *status = VI_ERROR_INV_SETUP;
        return rmp;
    }

    static ResManagerPtr getResManager(ViSession vi, ViStatus *status){
        auto i = repo.find(vi); 
        if(i != repo.end()){
            return i->second;
        }
        return nullptr;
    }

    static SessionPtr  getSession(ViSession vi, ViStatus *status){
        ViSession id = IdGen::getResManIdFromSessionId(vi);
        ResManagerPtr rm = getResManager(id, status);
        if(rm != nullptr){
            return rm->GetSession(vi); 
        }
        return nullptr;
    }

    static void putResManager(ViSession vi){
        for(auto i:repo){
            auto s = i.first; 
            auto r = i.second; 

            if(vi == s)
            {
                repo.erase(s); 
            }
        }
    }
private:
    static ::std::map<ViSession, ResManagerPtr> repo;
};

}

#endif

