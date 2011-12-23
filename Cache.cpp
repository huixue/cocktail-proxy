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
** Copyright (C) Sam King
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

#include "Cache.h"

#include <iomanip>
#include <iostream>

#include <assert.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include "CacheEntry.h"
#include "dbg.h"

using namespace std;

static Cache globalCache;
int Cache::num_browsers = 0;

static string reply404 = "HTTP/1.1 404 Not Found\r\nServer: twproxy\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";


extern int serverPorts[];

MySocket *Cache::getReplySocket(string host, bool isSSL)
{
    assert(host.find(':') != string::npos);
    assert(host.find(':') < (host.length()-1));
    string portStr = host.substr(host.find(':')+1);
    string hostStr = host.substr(0, host.find(':'));
    int port;
    int ret = sscanf(portStr.c_str(), "%d", &port);
    assert((ret == 1) && (port > 0));
    MySocket *replySock = NULL;
    try {
        //cout << "making connection to " << hostStr << ":" << port << endl;
        replySock = new MySocket(hostStr.c_str(), port);
        if(isSSL) {
            replySock->enableSSLClient();
        }
    } catch(char *e) {
        cout << e << endl;
    } catch(...) {
        cout << "could not connect to " << hostStr << ":" << port << endl;
    }
    return replySock;
}

//XXX: should check url, method, cookie, possible even port
CacheEntry *Cache::find(string url, string /*request*/) {
    map<string, CacheEntry *>::iterator i = m_store.find(url);
    if(i == m_store.end())
        return NULL;
    else
        return (CacheEntry *)(i->second);
}

void Cache::addToStore(string url, CacheEntry *ent) {
    assert(ent != NULL);
    m_store.insert(pair<string, CacheEntry *>(url, ent));
}

int Cache::votingFetchInsertWriteback(string url, string request, int browserId, MySocket *browserSock, string host, bool isSSL, MySocket *replySock)
{
    assert(browserId >= 0);
        //find() will take care of all checks, including same url, same method, different
        //cookie for the same browser or different browser
    CacheEntry *ent = find(url, request);
    if(ent == NULL) {
            //first request of this url, wait till someone vote and fetch then write
            //back to brower, or keep waiting forever
        cache_dbg("browser: %d ,%s no exist\n", browserId, url.c_str());
        ent = new CacheEntry(num_browsers, url, request);
        ent->setReqState(CACHE_NEW);
        cache_dbg("browser: %d ,%s SET to cache_new\n", browserId, url.c_str());
        int ret = ent->updateReqVec(browserId);

        vector<int> v = ent->getReqVec();
        printf("%d %d %d %s\n", v[0], v[1], v[2], url.c_str());
    
        
        assert(ret == 1);
        addToStore(url, ent);
        while(ent->getReqState() != CACHE_IN) {
            cache_dbg("CACHE_NEW cache before WAIT browser %d  %s\n", browserId, url.c_str());
            pthread_cond_wait(&cache_cond, &cache_mutex);
            cache_dbg("CACHE_NEW cache AFTER WAIT browser %d  %s\n", browserId, url.c_str());
        }
        

        v = ent->getReqVec();
        printf("%d waked up\n%d %d %d %s\n", browserId, v[0], v[1], v[2], url.c_str());

        sendBrowser(browserSock, ent, browserId);
    }
    else if(ent->getReqState() == CACHE_IN) {
            //this request is fetched
        cache_dbg("browser: %d ,%s is CACHE_IN\n", browserId, url.c_str());
        int ret = ent->updateReqVec(browserId);
        
        vector<int> v = ent->getReqVec();
        printf("%d %d %d %s\n", v[0], v[1], v[2], url.c_str());
        
            //opera is really making two same requests to http://google.com, no difference
//        assert(ret == 1);
        sendBrowser(browserSock, ent, browserId);
    }
    else if(ent->getReqState() == CACHE_FETCHING) {
            //somebody is fetching the request, wait till done
        cache_dbg("browser: %d ,%s is CACHE_FETCHING\n", browserId, url.c_str());
        int ret = ent->updateReqVec(browserId);

        vector<int> v = ent->getReqVec();
        printf("fetching\n%d %d %d %s\n", v[0], v[1], v[2], url.c_str());

        
//        assert(ret == 1);
        while(ent->getReqState() != CACHE_IN) {
            cache_dbg("CACHE_FETCHING cache before WAIT browser %d  %s\n", browserId, url.c_str());
            pthread_cond_wait(&cache_cond, &cache_mutex);
            cache_dbg("CACHE_FETCHING cache AFTER WAIT browser %d  %s\n", browserId, url.c_str());
        }


        v = ent->getReqVec();
        printf("%d waked up\n%d %d %d %s\n", browserId, v[0], v[1], v[2], url.c_str());

        
        sendBrowser(browserSock, ent, browserId);
    }
    else if(ent->getReqState() == CACHE_NEW) {
            //vote for some previous request from someone, and FETCH
            //it won't be my own old request, find() is going to take of that
        int ret = ent->updateReqVec(browserId);
        vector<int> v = ent->getReqVec();
        printf("%d %d %d %s\n", v[0], v[1], v[2], url.c_str());
//        assert(ret == 1);
        cache_dbg("browser: %d ,%s SET to cache_fetching\n", browserId, url.c_str());
        ent->setReqState(CACHE_FETCHING);
        cache_dbg("cache UNlock browser %d  %s\n", browserId, url.c_str());
        pthread_mutex_unlock(&cache_mutex);
        fetch(ent, host, isSSL, browserId, replySock);
        pthread_mutex_lock(&cache_mutex);
        cache_dbg("cache lock browser %d  %s\n", browserId, url.c_str());
        cache_dbg("browser: %d ,%s SET to cache_in\n", browserId, url.c_str());
        ent->setReqState(CACHE_IN);
        sendBrowser(browserSock, ent, browserId);
    }
    else
        assert(false);
    return 0;
}

int Cache::sendBrowser(MySocket *browserSock, CacheEntry *ent, int browserId) {
    cache_dbg("sendBrowser send to browser %d, response length %d\n", browserId, ent->getResponse().length());
    bool ret = browserSock->write_bytes(ent->getResponse().c_str(), ent->getResponse().length());
    
    ent->updateRespVec(browserId);
    return 0;
}

void Cache::getHTTPResponseVote(string host, string request, string url, int serverPort, 
                                MySocket *browserSock, bool isSSL, MySocket *replySock)
{    
    int browserId = -1;
    pthread_mutex_lock(&cache_mutex);
    browserId = serverPort - serverPorts[0];
    cache_dbg("cache lock browser %d  %s\n", browserId, url.c_str());
    votingFetchInsertWriteback(url, request, browserId, browserSock, host, isSSL, replySock);
    cache_dbg("cache UNlock browser %d  %s\n", browserId, url.c_str());
    pthread_mutex_unlock(&cache_mutex);
    cache_dbg("cache BROADCAST browser %d  %s\n", browserId, url.c_str());
    pthread_cond_broadcast(&cache_cond);
}

static void dbg_fetch(int ret) {
    switch(ret) {
        case ENOT_CONNECTED:
            cache_dbg("ESOCKET_CONNECTED returned by replySock->read()\n");
            break;
        case ECONN_CLOSED:
                //cache_dbg("ESOCKET_CLOSED returned by replySock->read()\n");
            break;
        case ESOCKET_ERROR:
            cache_dbg("ESOCKET_ERROR returned by replySock->read()\n");
            break;
        default:
            cache_dbg("%d bytes read by replySock->read()\n", ret);
            break;
    }
}


int Cache::fetch(CacheEntry *ent, string host, bool isSSL, int browserId, MySocket *replySock) {
    if(replySock == NULL) {
        cout << "returning 404" << endl;
        ent->appendResponse(reply404);
        return -1;
    }
    cache_dbg("CACHE: SENDING request %s\n", ent->getRequest().c_str());
    if(!replySock->write_bytes(ent->getRequest())) {
        cout << "returning 404" << endl;
        ent->appendResponse(reply404);
        return -1;
    }
    printf("FETCH CALLED %s, response length: %d\n", ent->getUrl().c_str(), ent->getResponse().length());
    unsigned char buf[1024];
    int num_bytes;
    cache_dbg("browser %d: BEFORE FETCHING %s\n", browserId, ent->getUrl().c_str());
    while((num_bytes = replySock->read(buf, sizeof(buf))) > 0) {
        cache_dbg("CACHE: fetch read %d bytes\n", num_bytes);
        ent->appendResponse((const char *)buf, num_bytes);
        cache_dbg("ent->resposne length: %d\n", ent->getResponse().length());
    }
    dbg_fetch(num_bytes);

    printf("FETCHED %s, response length: %d\n", ent->getUrl().c_str(), ent->getResponse().length());
    cache_dbg("browser %d: done FETCHING %s, response length: %d\n", browserId, ent->getUrl().c_str(), ent->getResponse().length());
    cache_dbg("browser %d: fetched %s\n%s\n", browserId, ent->getUrl().c_str(), ent->getResponse().c_str());
    delete replySock;
    return 0;
}

void Cache::handleResponse(MySocket *browserSock, MySocket *replySock, string request)
{
    if(!replySock->write_bytes(request)) {
        // XXX FIXME we should do something other than 404 here
        browserSock->write_bytes(reply404);
        return;
    }
    unsigned char buf[1024];
    int num_bytes;
    bool ret;
    while((num_bytes = replySock->read(buf, sizeof(buf))) > 0) {        
        ret = browserSock->write_bytes(buf, num_bytes);
        if(!ret) {
            break;
        }
    }
}

void Cache::getHTTPResponseNoVote(string host, string request, string url, int serverPort, 
                                  MySocket *browserSock, bool isSSL, MySocket *replySock)
{
    if(replySock == NULL) {
        cout << "returning 404" << endl;
        browserSock->write_bytes(reply404);
        return;
    }
    handleResponse(browserSock, replySock, request);

    delete replySock;
}

void Cache::setNumBrowsers(const int num) 
{
    num_browsers = (int)num;
}



Cache *cache()
{
    return &globalCache;    
}


Cache::Cache()
{
    pthread_mutex_init(&cache_mutex, NULL);
    pthread_cond_init(&cache_cond, NULL);
}
Cache::~Cache()
{
    pthread_cond_destroy(&cache_cond);
    pthread_mutex_destroy(&cache_mutex);
}



/*
bool Cache::copyNetBytes(MySocket *readSock, MySocket *writeSock)
{
    unsigned char buf[1024];
    int ret;

    ret = readSock->read(buf, sizeof(buf));
    if(ret <= 0)
        return false;

    return writeSock->write_bytes(buf, ret);
}

void Cache::handleTunnel(MySocket *browserSock, MySocket *replySock)
{
    if(!browserSock->write_bytes(CONNECT_REPLY))
        return;

    int bFd = browserSock->getFd();
    int rFd = replySock->getFd();    

    int ret;
    fd_set readSet;

    int maxFd = (bFd > rFd) ? bFd : rFd;

    while(true) {
        FD_ZERO(&readSet);

        FD_SET(rFd, &readSet);
        FD_SET(bFd, &readSet);

        ret = select(maxFd+1, &readSet, NULL, NULL, NULL);

        if(ret <= 0)
            break;

        if(FD_ISSET(rFd, &readSet)) {
            if(!copyNetBytes(replySock, browserSock)) {
                break;
            }
        }

        if(FD_ISSET(bFd, &readSet)) {
            if(!copyNetBytes(browserSock, replySock)) {
                break;
            }
        }
    }
}
*/
