
#ifndef __ID_MANAGER_HPP__
#define __ID_MANAGER_HPP__

#include "basic.hpp"
#include "atomic"

namespace hippo
{
class IdGen{
public:
    static const uint32_t ID_MASK    =(0xffU<<24);
    static const uint32_t ID_RESMAN  =(0x1U<<24);
    static const uint32_t ID_SESSION =(0x2U<<24);
    static const uint32_t ID_FINDLIST=(0x3U<<24);

    static bool isResmanId(uint32_t id){
        if((id & ID_MASK) == ID_RESMAN) return true;  
        else return false;
    }

    static bool isSessionId(uint32_t id){
        if((id & ID_MASK) == ID_SESSION) return true;  
        else return false;
    }

    static bool isFindListId(uint32_t id){
        if((id & ID_MASK) == ID_FINDLIST) return true;  
        else return false;
    }

    static uint32_t genResManId(){
        resman_cnt++; 
        if(resman_cnt > 255) resman_cnt = 0; 
        return resman_cnt|ID_RESMAN;
    }

    static uint32_t genSessionId(uint32_t resman_id){
        sess_cnt++; 
        if(sess_cnt > 65535) sess_cnt = 0; 
        return sess_cnt|ID_SESSION|((resman_id&0xff)<<16);
    }

    static uint32_t getResManIdFromSessionId(uint32_t id){
        return (((id>>16) & 0xffU)|ID_RESMAN); 
    }

    static uint32_t getResManIdFromfindListId(uint32_t id){
        return (((id>>16) & 0xffU)|ID_RESMAN); 
    }
private:     
    static std::atomic<uint32_t> resman_cnt;
    static std::atomic<uint32_t> sess_cnt;
};
}

#endif

