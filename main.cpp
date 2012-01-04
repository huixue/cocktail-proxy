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

#include <iostream>
#include <queue>

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "MyServerSocket.h"
#include "HTTPRequest.h"
#include "Cache.h"
#include "dbg.h"
#include "time.h"

#include "UI.h"

using namespace std;

int serverPorts[] = {8808, 8809, 8810};
#define NUM_SERVERS (sizeof(serverPorts) / sizeof(serverPorts[0]))

static string CONNECT_REPLY = "HTTP/1.1 200 Connection Established\r\n\r\n";

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned long numThreads = 0;

struct client_struct {
    MySocket *sock;
    int serverPort;
    queue<MySocket *> *killQueue;
};

struct server_struct {
    int serverPort;
};

pthread_t server_threads[NUM_SERVERS];
static int gVOTING = 0;

void run_client(MySocket *sock, int serverPort)
{
    HTTPRequest *request = new HTTPRequest(sock, serverPort);    
//    httpreq_dbg("%d: ", serverPort);
    if(!request->readRequest()) {
        cout << "did not read request" << endl;
    } else {    
        bool error = false;
        bool isSSL = false;

        string host = request->getHost();
        string url = request->getUrl();

        MySocket *replySock = NULL;
        
        if(request->isConnect()) {
            host = request->getHost();
            url = request->getUrl();        
//            cerr << serverPort << " connect request for " << host << " " << url << endl;
            if(!sock->write_bytes(CONNECT_REPLY)) {
                error = true;
            } else {
                delete request;
                replySock = cache()->getReplySocket(host, true);
                    //need proxy <--> remotesite socket for information needed to fake a certificate
                sock->enableSSLServer(replySock);
                isSSL = true;
                request = new HTTPRequest(sock, serverPort);
                if(!request->readRequest()) {
                    error = true;
                }
            }            
        } else
            replySock = cache()->getReplySocket(host, false);
        
        if(!error) {
            string req = request->getRequest();
                //these two lines here are for HTTPS, otherwise host and url are the ones in CONNECT
            host = request->getHost();
            url = request->getUrl();

            cerr << serverPort << " request for " << host << " " << url << endl;

            int isUI = uimgr()->isUIRequest(url);
            if(isUI != 0) {
                if(isUI > 0)
                    uimgr()->processUILog(sock, url, serverPort);
                else
                    uimgr()->processUIGet(sock, serverPort);
            }
            else {
                if(gVOTING == 0) {
                    cache()->getHTTPResponseNoVote(host, req, url, serverPort, sock, isSSL, replySock);
                } else {
                    cache()->getHTTPResponseVote(host, req, url, serverPort, sock, isSSL, replySock);
                }
            }
        }
    }    

    sock->close();
    delete request;
}

void *client_thread(void *arg)
{
    struct client_struct *cs = (struct client_struct *) arg;
    MySocket *sock = cs->sock;
    int serverPort = cs->serverPort;
    queue<MySocket *> *killQueue = cs->killQueue;

    delete cs;
    
    pthread_mutex_lock(&mutex);
    numThreads++;
        //cout << "before numThread = " << numThreads << endl;
    pthread_mutex_unlock(&mutex);

    run_client(sock, serverPort);

    pthread_mutex_lock(&mutex);
    numThreads--;
        //cout << "after numThread = " << numThreads << endl;

    // This is a hack because linux is having trouble freeing memory
    // in a different thread, so instead we will let the server thread
    // free this memory 
    killQueue->push(sock);
    pthread_mutex_unlock(&mutex);    

    return NULL;
}

void start_client(MySocket *sock, int serverPort, queue<MySocket *> *killQueue)
{
    struct client_struct *cs = new struct client_struct;
    cs->sock = sock;
    cs->serverPort = serverPort;
    cs->killQueue = killQueue;

    pthread_t tid;
    int ret = pthread_create(&tid, NULL, client_thread, cs);
    assert(ret == 0);
    ret = pthread_detach(tid);
    assert(ret == 0);
}

void *server_thread(void *arg)
{
    struct server_struct *ss = (struct server_struct *)arg;
    int port = ss->serverPort;
    delete ss;
    
    MyServerSocket *server = new MyServerSocket(port);
    assert(server != NULL);
    MySocket *client;
    queue<MySocket *> killQueue;
    while(true) {
        try {
            client = server->accept();
        } catch(MySocketException e) {
            cerr << e.toString() << endl;
            exit(1);
        }
        pthread_mutex_lock(&mutex);
        while(killQueue.size() > 0) {
                //ui_dbg("server_thread killing killQueue\n");
            delete killQueue.front();
            killQueue.pop();
        }
        pthread_mutex_unlock(&mutex);
        start_client(client, port, &killQueue);
    }    
    return NULL;
}


pthread_t start_server(int port)
{
    cerr << "starting server on port " << port << endl;
    server_struct *ss = new struct server_struct;
    ss->serverPort = port;
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, server_thread, ss);
    assert(ret == 0);
    return tid;
}

static void get_opts(int argc, char *argv[])
{
    int c;
    while((c = getopt(argc, argv, "v")) != EOF) {
        switch(c) {
            case 'v':
                gVOTING = 1;
                break;
            default:
                cerr << "Wrong Argument." << endl;
                exit(1);
                break;
        }
    }
}

static pthread_mutex_t *lock_cs;
static long *lock_count;

static void pthreads_locking_callback(int mode, int type, char *file, int line)
{
#ifdef undef
    fprintf(stderr,"thread=%4d mode=%s lock=%s %s:%d\n",
            CRYPTO_thread_id(),
            (mode&CRYPTO_LOCK)?"l":"u",
            (type&CRYPTO_READ)?"r":"w",file,line);
#endif
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&(lock_cs[type]));
        lock_count[type]++;
    } else {
        pthread_mutex_unlock(&(lock_cs[type]));
    }
}

static unsigned long pthreads_thread_id(void)
{
    unsigned long ret;

    ret=(unsigned long)pthread_self();
    return(ret);
}

//openssl_thread_setup(): stole from mttest.c in openssl/crypto/threads/mttest.c
static void openssl_thread_setup()
{
    int i;
    
    lock_cs = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    lock_count = (long *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(long));
    for (i = 0; i < CRYPTO_num_locks(); i++)
    {
        lock_count[i] = 0;
        pthread_mutex_init(&(lock_cs[i]),NULL);
    }
    
    CRYPTO_set_id_callback((unsigned long (*)())pthreads_thread_id);
    CRYPTO_set_locking_callback((void (*)(int, int, const char *, int))pthreads_locking_callback);
}
//openssl_thread_cleanup(): stole from mttest.c in openssl/crypto/threads/mttest.c
static void openssl_thread_cleanup()
{
    int i;

    CRYPTO_set_locking_callback(NULL);
    fprintf(stderr,"cleanup\n");
    for (i = 0; i < CRYPTO_num_locks(); i++)
    {
        pthread_mutex_destroy(&(lock_cs[i]));
        fprintf(stderr,"%8ld:%s\n",lock_count[i], CRYPTO_get_lock_name(i));
    }
    OPENSSL_free(lock_cs);
    OPENSSL_free(lock_count);

    fprintf(stderr,"done cleanup\n");

}


int main(int argc, char *argv[])
{
        //if started with "-v" option, voting will be enabled.
        //Otherwise, just a plain proxy            
    get_opts(argc, argv);  
    // get socket write errors from write call
    signal(SIGPIPE, SIG_IGN);

    // initialize ssl library
    SSL_load_error_strings();
    SSL_library_init();

    openssl_thread_setup();

    cout << "number of servers: " << NUM_SERVERS << endl;

        //when generating serial number for X509, need random number
    srand(time(NULL));
    Cache::setNumBrowsers(NUM_SERVERS);
    
    pthread_t tid;
    int ret;
    for(unsigned int idx = 0; idx < NUM_SERVERS; idx++) {
        tid = start_server(serverPorts[idx]);
        server_threads[idx] = tid;
    }

    for(unsigned int idx = 0; idx < NUM_SERVERS; idx++) {
        ret = pthread_join(server_threads[idx], NULL);
        assert(ret == 0);
    }
    
    openssl_thread_cleanup();
    
    return 0;
}
