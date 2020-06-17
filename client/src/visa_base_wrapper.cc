#include "ResManager.hpp"

using namespace hippo;
using namespace std;

extern  "C" ViStatus _VI_FUNC  viOpenDefaultRM (ViPSession vi){
    ViStatus status;
    std::string url("ws://127.0.0.1:5026/");
    ResManagerPtr rm = ResManagerMaker::getResManager(vi, url, &status);

    if(rm != nullptr)
    {
        return rm->open();
    }
    else
    {
        return status;
    }
}

extern  "C" ViStatus _VI_FUNC  viOpenRM (ViPSession vi, ViString url){

    ViStatus status;
    std::string u(url);
    ResManagerPtr rm = ResManagerMaker::getResManager(vi, u, &status);
    if(rm != nullptr)
    {
        return rm->open();
    }
    else
    {
        return status;
    }

}

extern  "C"    ViStatus _VI_FUNC  viFindRsrc(ViSession sesn, ViString expr, ViPFindList vi,
                                   ViPUInt32 retCnt, ViChar _VI_FAR desc[]){

    ViStatus status;
    ResManagerPtr rm = ResManagerMaker::getResManager(sesn, &status);
    if(rm != nullptr)
    {
        return rm->FindRsrc(expr, vi, retCnt, desc);
    }
    else
    {
        return status;
    }
}

extern  "C" ViStatus _VI_FUNC  viFindNext(ViFindList vi, ViChar _VI_FAR desc[]){
    ViStatus status;
    ViSession sesn = IdGen::getResManIdFromfindListId(vi);
    ResManagerPtr rm = ResManagerMaker::getResManager(sesn, &status);
    if(rm != nullptr)
    {
        return rm->FindNext(vi, desc);
    }
    else
    {
        return status;
    }
}


extern  "C" ViStatus _VI_FUNC  viParseRsrcEx(ViSession sesn, ViRsrc rsrcName, ViPUInt16 intfType,
                                   ViPUInt16 intfNum, ViChar _VI_FAR rsrcClass[],
                                   ViChar _VI_FAR expandedUnaliasedName[],
                                   ViChar _VI_FAR aliasIfExists[]){

    ViStatus status;
    ResManagerPtr rm = ResManagerMaker::getResManager(sesn, &status);
    if(rm != nullptr)
    {
        return rm->ParseRsrc(rsrcName, intfType, intfNum,
                       rsrcClass, expandedUnaliasedName, aliasIfExists);
    }
    else
    {
        return status;
    }

}

extern  "C" ViStatus _VI_FUNC  viParseRsrc(ViSession sesn, ViRsrc rsrcName,
                                   ViPUInt16 intfType, ViPUInt16 intfNum){
    return viParseRsrcEx(sesn, rsrcName, intfType, intfNum, NULL, NULL, NULL);
}


extern  "C" ViStatus _VI_FUNC  viOpen(ViSession sesn, ViRsrc name, ViAccessMode mode,
                                   ViUInt32 timeout, ViPSession vi){

    ViStatus status;
    ResManagerPtr rm = ResManagerMaker::getResManager(sesn, &status);
    if(rm != nullptr)
    {
        return rm->OpenSession(name, mode, timeout, vi);
    }
    else
    {
        return status;
    }

}

extern  "C" ViStatus _VI_FUNC  viClose(ViObject vi){

    ViStatus status;

    if(IdGen::isResmanId(vi)){
        ResManagerMaker::putResManager(vi);
    }
    else if(IdGen::isSessionId(vi)){
        ViSession id = IdGen::getResManIdFromSessionId(vi);
        ResManagerPtr rm = ResManagerMaker::getResManager(id, &status);
        if(rm != nullptr){
            rm->CloseSession(vi); 
        }
    }
    else if(IdGen::isFindListId(vi)){
        ViSession id = IdGen::getResManIdFromfindListId(vi);
        ResManagerPtr rm = ResManagerMaker::getResManager(id, &status);
        if(rm != nullptr){
            rm->CloseFindList(vi); 
        }
    }
    else{
    
    }
    return status;
}

extern  "C" ViStatus _VI_FUNC  viWrite(ViSession vi, ViBuf  buf, ViUInt32 cnt, ViPUInt32 retCnt){
    ViStatus status;
    SessionPtr s = ResManagerMaker::getSession(vi, &status);
    if(s != nullptr){
        int ret = 0; 
        ret = s->send(buf, cnt);
        if(retCnt){
            *retCnt = ret;
        }
        if(ret > 0) status = VI_SUCCESS;
    }
    return status;
}

extern  "C" ViStatus _VI_FUNC  viWriteFromFile(ViSession vi, ViConstString filename, ViUInt32 cnt,
                                    ViPUInt32 retCnt){

}


extern  "C" ViStatus _VI_FUNC  viWriteAsync(ViSession vi, ViBuf  buf, ViUInt32 cnt, ViPJobId  jobId){

}


extern  "C" ViStatus _VI_FUNC  viRead(ViSession vi, ViPBuf buf, ViUInt32 cnt, ViPUInt32 retCnt){
    ViStatus status;
    SessionPtr s = ResManagerMaker::getSession(vi, &status);
    if(s != nullptr){
        int ret = 0; 
        ret = s->recv(buf, cnt);
        if(retCnt){
            *retCnt = ret;
        }
        if(ret > 0) status = VI_SUCCESS;
    }
    return status;
}

extern  "C" ViStatus _VI_FUNC  viReadToFile(ViSession vi, ViConstString filename, ViUInt32 cnt,
                                    ViPUInt32 retCnt){


}

 extern  "C" ViStatus _VI_FUNC  viReadAsync(ViSession vi, ViPBuf buf, ViUInt32 cnt, ViPJobId  jobId){


}


extern  "C" ViStatus _VI_FUNC  viSetAttribute(ViObject vi, ViAttr attrName, ViAttrState attrValue){

    ViStatus status;

    if(IdGen::isResmanId(vi)){

    }
    else if(IdGen::isSessionId(vi)){
        SessionPtr s = ResManagerMaker::getSession(vi, &status);
        if(s != nullptr){
            int ret = s->setOption(attrName, (const void *)(&attrValue), sizeof(ViAttrState));
        }
    }
    else if(IdGen::isFindListId(vi)){

    }
    else{
    
    }
    return status;
}

extern  "C" ViStatus _VI_FUNC  viGetAttribute(ViObject vi, ViAttr attrName, void _VI_PTR attrValue){
    ViStatus status;

    if(IdGen::isResmanId(vi)){

    }
    else if(IdGen::isSessionId(vi)){
        SessionPtr s = ResManagerMaker::getSession(vi, &status);
        if(s != nullptr){
            int ret = s->getOption(attrName, (void *)attrValue, sizeof(ViAttrState));
        }
    }
    else if(IdGen::isFindListId(vi)){

    }
    else{
    
    }
    return status;

}

extern  "C" ViStatus _VI_FUNC  viAssertTrigger(ViSession vi, ViUInt16 protocol){

}

extern  "C" ViStatus _VI_FUNC  viReadSTB(ViSession vi, ViPUInt16 status){

}

extern  "C" ViStatus _VI_FUNC  viClear(ViSession vi){

}

