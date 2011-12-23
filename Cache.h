#ifndef _CACHE_H_
#define _CACHE_H_

#include <string>
#include "MySocket.h"

#include "CacheEntry.h"
#include <map>
#include <pthread.h>
class Cache {
  public:
    Cache();
    virtual ~Cache();
    void getHTTPResponseNoVote(std::string host, std::string request, std::string url, 
                               int serverPort, MySocket *browserSock, bool isSSL, MySocket *replySock);
    void getHTTPResponseVote(std::string host, std::string request, std::string url, 
                             int serverPort, MySocket *browserSock, bool isSSL, MySocket *replySock);
    MySocket *getReplySocket(std::string host, bool isSSL);
    static void setNumBrowsers(const int num);
  protected:
    void handleResponse(MySocket *browserSock, MySocket *replySock, std::string request);
        //void handleTunnel(MySocket *browserSock, MySocket *replySock);
        //bool copyNetBytes(MySocket *readSock, MySocket *writeSock);

    
        //Need this "string request" parameter, because we have to buffer it, and send it
        //out later if voted
        //Must grab cache_mutex
    int votingFetchInsertWriteback(std::string url, std::string request, int browserId,
                                   MySocket *browserSock, std::string host, bool isSSL, MySocket *replySock);
        //Must grab cache_mutex
    void addToStore(std::string url, CacheEntry *ent);
        //Must grab cache_mutex
    CacheEntry *find(std::string url, std::string request);
        //Must grab cache_mutex
    int sendBrowser(MySocket *browserSock, CacheEntry *ent, int browserId);

    int fetch(CacheEntry *ent, std::string host, bool isSSL, int browserId, MySocket *replySock);
    

    std::map<std::string, CacheEntry*> m_store;

    pthread_mutex_t cache_mutex;
    pthread_cond_t cache_cond;

    static int num_browsers;
};



Cache *cache();

#endif
