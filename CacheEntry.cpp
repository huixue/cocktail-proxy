/*======================================================== 
** University of Illinois/NCSA 
** Open Source License 
**
** Copyright (C) 2011,The Board of Trustees of the University of 
** Illinois. All rights reserved. 
**
** Developed by: 
**
**    Research Group of Professor Sam King in the Department of Computer 
**    Science The University of Illinois at Urbana-Champaign 
**    http://www.cs.uiuc.edu/homes/kingst/Research.html 
**
** Copyright (C) Hui Xue
**
** Permission is hereby granted, free of charge, to any person obtaining a 
** copy of this software and associated documentation files (the 
** Software), to deal with the Software without restriction, including 
** without limitation the rights to use, copy, modify, merge, publish, 
** distribute, sublicense, and/or sell copies of the Software, and to 
** permit persons to whom the Software is furnished to do so, subject to 
** the following conditions: 
**
** Redistributions of source code must retain the above copyright notice, 
** this list of conditions and the following disclaimers. 
**
** Redistributions in binary form must reproduce the above copyright 
** notice, this list of conditions and the following disclaimers in the 
** documentation and/or other materials provided with the distribution. 
** Neither the names of Sam King or the University of Illinois, 
** nor the names of its contributors may be used to endorse or promote 
** products derived from this Software without specific prior written 
** permission. 
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
** IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
** ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
** SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE. 
**========================================================== 
*/

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
