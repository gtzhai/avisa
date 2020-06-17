#ifndef __SCANNER_HPP__
#define __SCANNER_HPP__

#include <string>
#include <list>
#include <memory>
#include <functional>
#include <map>
#include <iostream>

namespace hippo
{
/**
 *
 * finder 
 *
 */
class Finder{
public:

    virtual std::list<std::string> availableDevs() const { return availableDevs("");};
    virtual std::list<std::string> availableDevs(const std::string &pattern) const{std::list<std::string> list; return list;};

    void setPattern(const std::string &p){
        pattern_ = p; 
    }

    std::string &pattern(){
        return pattern_; 
    }

protected:
    std::string pattern_;
};
typedef std::shared_ptr<Finder> FinderPtr;

/**
 *
 * port
 *
 */
class Port{
public:
    Port(std::string &res, FinderPtr finder): res_(res){};
    
    virtual bool open(){ return false;};
    virtual void close() {};
    virtual int read(char *data, ssize_t len){ return 0;};
    virtual int write(const char *data, ssize_t len) {return 0;};
    virtual int setOption(int option, const void *data, ssize_t len) {return -1;};
    virtual int getOption(int option, void *data, ssize_t len) {return -1;};

    std::string &pattern(){
        return pattern_; 
    }
protected:
    std::string res_;
    std::string pattern_;
};
typedef std::shared_ptr<Port> PortPtr;

class FinderRepo{
public:
    int registerFinder(std::string name, FinderPtr f);
    void unregisterFinder(std::string &name);

    std::list<FinderPtr >searchFinder(const std::string &pattern);
    std::list<FinderPtr> searchFinderWithRes(const std::string &res);
private:
    std::map<std::string, FinderPtr> repo;
};
typedef std::unique_ptr<FinderRepo> FinderRepoPtr;

class PortRepo{
public:
    typedef std::function<PortPtr (std::string &res, FinderPtr finder)> CallbackMakePort;

    int registerPort(std::string &name, CallbackMakePort cb);
    void unregisterPort(std::string &name);

    PortPtr getPort(std::string &res, FinderPtr finder);
private:
    std::map<std::string, CallbackMakePort> repo;
};
typedef std::unique_ptr<PortRepo> PortRepoPtr;

}

#endif

