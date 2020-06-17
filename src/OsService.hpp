
#ifndef __OS_SERVICE_HPP__
#define __OS_SERVICE_HPP__

#include <string>
#include <functional>
#include <memory>

namespace hippo
{
typedef std::function<int (void)> CallbackMainRun;
class OsService{
 public:

  OsService(const std::string & name, const std::string& desc, CallbackMainRun cb):name_(name), desc_(desc), cb_(cb){};

  OsService(const OsService& other) = delete;
  OsService& operator=(const OsService& other) = delete;

  OsService(OsService&& other) = delete;
  OsService& operator=(OsService&& other) = delete;

  virtual ~OsService() {}
  
  int run();
  const std::string &name(){return name_;};
  const std::string &desc(){return desc_;};
  const CallbackMainRun  &callback(){return cb_;};

  static OsService* m_service;
  int abort;
 private:
  std::string name_;
  std::string desc_;
  CallbackMainRun cb_;

};
typedef std::shared_ptr<OsService> OsServicePtr;

class OsServiceInstaller{
public:
    static bool install(const OsService &service);
    static bool uninstall(const OsService &service);
};
}

#endif

