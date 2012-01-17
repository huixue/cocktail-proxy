#include "CacheEntry.h"

CacheEntry::CacheEntry(int num_browsers, std::string _url, std::string _request)
{
        url = _url;
        method = METHOD_UNDEFINED;
        cache_state = CACHE_UNDEFINED;
        m_numBrowsers = num_browsers;
        request = _request;
        for(int i = 0; i < num_browsers; i++) {
                req_vec.push_back(0);
                resp_vec.push_back(0);
        }
}

CacheEntry::~CacheEntry()
{
}

void CacheEntry::setReqState(REQUEST_CACHE_STATE s)
{
        cache_state = s;
}

REQUEST_CACHE_STATE CacheEntry::getReqState()
{
        return cache_state;
}

int CacheEntry::updateReqVec(int browserId)
{
        assert(browserId < m_numBrowsers);
        return ++req_vec[browserId];
}

int CacheEntry::updateRespVec(int browserId)
{
        assert(browserId < m_numBrowsers);
//        assert(resp_vec[browserId] == 0);
        return ++resp_vec[browserId];
}

std::string CacheEntry::getRequest()
{
        return request;
}

std::string CacheEntry::getResponse()
{
        return response;
}

void CacheEntry::appendResponse(std::string part)
{
        response.append(part.c_str());
}

void CacheEntry::appendResponse(const char *part, int size)
{
        response.append(part, size);
}

std::string CacheEntry::getUrl()
{
        return url;
}
