
#ifndef __RES_HPP__
#define __RES_HPP__

#include "basic.hpp"

namespace hippo
{
class Resource{
public:
    Resource(std::string &res):res_(res){}; 
    Resource(const char *res):res_(res){}; 
    Resource(const Resource &res) {res_ = res.resStr();};
    void operator= (const Resource &res) {res_ = res.resStr();};
    const std::string &resStr()const { return res_;};
private:
    std::string res_;
};
typedef std::shared_ptr<Resource> ResourcePtr;

typedef std::list<Resource>::iterator  ResListIterator;
struct ResList{
public:
    ResListIterator iterator;
    std::list<Resource> list;
};
typedef std::shared_ptr<ResList> ResourceListPtr;

}

#endif

