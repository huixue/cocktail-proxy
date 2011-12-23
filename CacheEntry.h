//CacheEntry is ONLY used when in voting mode, non-voting mode does NOT have cache
//because Squid can easily do that for us(except MITM need more tricks).
#ifndef __CACHESTORE_H_
#define __CACHESTORE_H_

#include <string>
#include <vector>
#include <map>

#include "assert.h"

enum HTTP_METHODS {
    GET = 0,
    POST,
    METHOD_UNDEFINED,
};

enum REQUEST_CACHE_STATE {
    CACHE_NEW = 0,
    CACHE_FETCHING,
    CACHE_IN,
    CACHE_UNDEFINED,
};

class CacheEntry {
  public:
    CacheEntry(int num_browsers, std::string _url, std::string _request);

    virtual ~CacheEntry();
    
    void setReqState(REQUEST_CACHE_STATE s);
    
    REQUEST_CACHE_STATE getReqState();
    
    int updateReqVec(int browserId);
    
    int updateRespVec(int browserId);

    std::vector<int> getReqVec() { return req_vec; };
    
    std::string getRequest();
    
    std::string getResponse();
    
    void appendResponse(std::string part);
    
    void appendResponse(const char *part, int size);
    
    std::string getUrl();
    
  protected:
    int m_numBrowsers;
    std::string url;
    HTTP_METHODS method;
    std::vector<int> req_vec;
    std::vector<int> resp_vec;
    std::string request;
    std::string response;    
    volatile REQUEST_CACHE_STATE cache_state;
};



#endif
