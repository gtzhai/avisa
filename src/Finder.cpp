#include "Finder.hpp"
#include <regex>


namespace hippo
{
    int FinderRepo::registerFinder(std::string name, FinderPtr f){

        //std::cout <<__func__<<name<<std::endl;
        for (auto &i: repo){
            auto fn = i.first;
            auto fi = i.second; 

            if(name == fn){
                return 0;
            }
        }

        repo.insert(std::pair<std::string, FinderPtr>(name, f));
        //std::cout <<__func__<<":ret"<<std::endl;
        return 0;
    }

    void FinderRepo::unregisterFinder(std::string &name){
        repo.erase(name); 
    }

    std::list<FinderPtr> FinderRepo::searchFinder(const std::string &pattern){
        std::list<FinderPtr> list;

        for (auto &i: repo){
            auto fn = i.first;
            auto fi = i.second; 

            std::cout <<__func__<<"dest pattern:"+pattern<<" finder pattern: "+ fi->pattern()<<std::endl;
            std::regex r(pattern+".*");
            if(std::regex_match(fi->pattern(), r)){
                list.push_back(fi);                
                std::cout <<"matched"<<std::endl;
            }
        }


        return list;
    }
    std::list<FinderPtr> FinderRepo::searchFinderWithRes(const std::string &res){
        std::list<FinderPtr> list;

        for (auto &i: repo){
            auto fn = i.first;
            auto fi = i.second; 

            std::cout <<__func__<<"res:"+res<<" finder pattern: "+ fi->pattern()<<std::endl;
            std::regex r(fi->pattern());
            if(std::regex_match(res, r)){
                list.push_back(fi);                
                std::cout <<"matched"<<std::endl;
            }
        }


        return list;
    }

    int PortRepo::registerPort(std::string &name, CallbackMakePort cb){

        for (auto &i: repo){
            auto fn = i.first;
            auto fi = i.second; 

            if(name == fn){
                return 0;
            }
        }

        repo.insert(std::pair<std::string&, CallbackMakePort>(name, cb));
        return 0;
    }

    void PortRepo::unregisterPort(std::string &name){
        repo.erase(name); 
    }


    PortPtr PortRepo::getPort(std::string &res, FinderPtr finder){
    
        for (auto &i: repo){
            auto fn = i.first;
            auto fi = i.second; 
            std::regex r(fn);
            std::smatch smatch;

            if(std::regex_match(res, smatch, r)){
                return fi(res, finder);
            }
        }

        return nullptr;
    }
}


