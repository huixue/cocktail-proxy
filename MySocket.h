#ifndef MYSOCKET_H
#define MYSOCKET_H

#define ENOT_CONNECTED -1
#define EBROKEN_PIPE   -2
#define ECONN_CLOSED   -3
#define ESOCKET_ERROR  -4

#include "MySocketException.h"

#include <string>
#include <openssl/ssl.h>

class MySocket {
  public:
        /*
         * this is the constructor.  It accepts a string representation of
         * and ip address ("192.168.0.1") or domain name ("www.cs.uiuc.edu")
         * and connects.  Will throw an HostNotFound exception if the attepted
         * connection fails.  MySocket uses the TCP protocol.
         *
         * @param inetAddr either ip address, or the domain name
         * @param port the port to connect to
         */
    MySocket(const char *inetAddr, int port);

        /*
         * this constructor will generally not be used except for by ServerSockets
         */
    MySocket(int socketFileDesc);

        /*
         * default constructor, makes sure the state is properly specified
         */
        //hx: I don't think we should give default ctor anymore
//    MySocket(void);
    ~MySocket(void);

        /*
         * reads the open socket.  See the read system call
         * 
         * @param buffer buffer of length len, where the data will be stored
         * @param len the length of the buffer
         *
         * @return if there is no error, the number of bytes read in.
         *   ECONN_CLOSED - connection was closed
         *   EBROKEN_PIPE - broken pipe
         *   ENOT_CONNECTED - a connection was never established
         */
    int read(void *buffer, int len);

        /*
         * writes to the open socket, see the write system call.
         *
         * @param buffer the buffer where the data is stored
         * @param len the length of the buffer
         *
         * @return if there is no error, the number of bytes wrote.
         *   ECONN_CLOSED - connection was closed
         *   EBROKEN_PIPE - broken pipe
         *   ENOT_CONNECTED - a connection was never established
         */
    int write(const void *buffer, int len);

    bool write_bytes(std::string buffer);
    bool write_bytes(const void *buffer, int len);
    void __enableSSLServer(void);
    void enableSSLServer(MySocket *);
    void enableSSLClient(void);
  
        /*
         * a helper function so select can be used
         */
    int getFd(void) { return sockFd; }

    void close(void);
  
  protected:
        //this is the function which generate a fake certificate, based on
        //the proxy <--> remotesite connection.
    X509 *generateFakeCert(MySocket *clentSock);
        //these are helper functions to make fake certificate
    EVP_PKEY *readPublicKey(const char *certfile);
    EVP_PKEY *readPrivateKey(const char *keyfile);
    X509 *readX509(const char *certfile);
    X509 *makeAndInitCert();
    void initNewName(X509_NAME *new_name, X509_NAME *server_cert_subj_name);
    
    int sockFd;
    void brokenPipe(int sigNo);
    bool isSSL;
    SSL_CTX *ctx;
    SSL *ssl;
};

#endif
