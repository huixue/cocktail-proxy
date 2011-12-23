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

#include "MySocket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/asn1.h>

#include <cstdlib>
#include "dbg.h"

using namespace std;

#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }

#define CHK_NULL(x) if ((x)==NULL) do{ printf("%d chk_null failed\n", __LINE__); exit (1);} while(0);


#define HOME "./"
#define CERTF  HOME "cacert.pem"
#define KEYF  HOME  "privkey.pem"


MySocket::MySocket(const char *inetAddr, int port)
{
    struct sockaddr_in server;
    struct addrinfo hints;
    struct addrinfo *res;

    isSSL = false;
    ctx = NULL;
    ssl = NULL;

    // set up the new socket (TCP/IP)
    sockFd = socket(AF_INET,SOCK_STREAM,0);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int ret = getaddrinfo(inetAddr, NULL, &hints, &res);
    if(ret != 0) {
        string str;
        str = string("Could not get host ") + string(inetAddr);
        throw MySocketException(str.c_str());
    }
    
    server.sin_addr = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
    server.sin_port = htons((short) port);
    server.sin_family = AF_INET;
    freeaddrinfo(res);
    
    // conenct to the server
    if( connect(sockFd, (struct sockaddr *) &server,
                sizeof(server)) == -1 ) {
        throw MySocketException("Did not connect to the server");
    }
}

MySocket::MySocket(int socketFileDesc)
{
    sockFd = socketFileDesc;
    isSSL = false;
    ctx = NULL;
    ssl = NULL;
}

MySocket::~MySocket(void)
{
    close();
}


EVP_PKEY * MySocket::readPublicKey(const char *certfile)
{
    FILE *fp = fopen (certfile, "r");
    X509 *x509;
    EVP_PKEY *pkey;
    if (!fp)
        return NULL;
    x509 = PEM_read_X509(fp, NULL, 0, NULL);
    if (x509 == NULL) {
        ERR_print_errors_fp (stderr);
        return NULL;
    }
    fclose (fp);
    pkey=X509_extract_key(x509);
    X509_free(x509);
    if (pkey == NULL)
        ERR_print_errors_fp (stderr);
    return pkey;
}

EVP_PKEY *MySocket::readPrivateKey(const char *keyfile)
{
    FILE *fp = fopen(keyfile, "r");
    EVP_PKEY *pkey;
    if (!fp)
        return NULL;
    pkey = PEM_read_PrivateKey(fp, NULL, 0, NULL);
    fclose (fp);
    if (pkey == NULL)
        ERR_print_errors_fp (stderr);
    return pkey;
}

X509 *MySocket::readX509(const char *certfile) 
{
    FILE *fp = fopen(certfile, "r");
    if(!fp)
        return NULL;
        //X509 *PEM_read_X509(FILE *fp, X509 **x, pem_password_cb *cb, void *u);
    X509 *cert = PEM_read_X509(fp, NULL, NULL, NULL);    CHK_NULL(cert);
    fclose(fp);
    return cert;
}

X509 *MySocket::makeAndInitCert()
{
    X509 *new_cert = X509_new();
    X509_set_version(new_cert, 2);
    long serialNumber = rand() % RAND_MAX;
    ASN1_INTEGER_set(X509_get_serialNumber(new_cert), serialNumber);
    X509_gmtime_adj(X509_get_notBefore(new_cert), 0);
    X509_gmtime_adj(X509_get_notAfter(new_cert), (long)60*60*24*365);
    return new_cert;
}

void MySocket::initNewName(X509_NAME *new_name, X509_NAME *server_cert_subj_name)
{
    assert(new_name != NULL);
    assert(server_cert_subj_name != NULL);
        //now setup "CN"
    int serv_cert_subjname_ent_num = X509_NAME_entry_count(server_cert_subj_name);
    mitm_dbg("subject name entry number: %d\n", serv_cert_subjname_ent_num);
    unsigned char *ent_data_str = NULL;
    char *ent_obj_str = NULL;
    X509_NAME_ENTRY *e = NULL;
    ASN1_STRING *asn1_string = NULL;
    ASN1_OBJECT *asn1_obj = NULL;
    int n = -1;
    for(int i; i < serv_cert_subjname_ent_num; i++) {
        e = X509_NAME_get_entry(server_cert_subj_name, i);
        asn1_string = X509_NAME_ENTRY_get_data(e);
        asn1_obj = X509_NAME_ENTRY_get_object(e);
        ASN1_STRING_to_UTF8(&ent_data_str, asn1_string);
        n = OBJ_obj2nid(asn1_obj);
        ent_obj_str = (char *)OBJ_nid2ln(n);
        
        mitm_dbg("name entry %d: %s %s\n", i, ent_obj_str, ent_data_str);
            //XXX: ent_data_str and ent_obj_str are pointers to inside fields, should not free them
        if(strncmp(ent_obj_str, "commonName", 10) == 0) {
            mitm_dbg("setting CN to %s\n", ent_data_str);
            if(!X509_NAME_add_entry_by_txt(new_name, "CN", MBSTRING_ASC, ent_data_str, -1, -1, 0)) {
                ERR_print_errors_fp(stderr);
                exit(7);
            }    
        }
    }
    if(!X509_NAME_add_entry_by_txt(new_name, "C", MBSTRING_ASC, (const unsigned char *)"US", -1, -1, 0)) {
        ERR_print_errors_fp(stderr);
        exit(8);
    }
}

//commented some debug codes, keep them for now    
X509 *MySocket::generateFakeCert(MySocket *clientSock)
{
        //do NOT free server_cert or these names, because enableSSLClient() is going to free it
    
    //get the certificate from the proxy <--> remotesite connection
    X509 *server_cert = SSL_get_peer_certificate (clientSock->ssl);    CHK_NULL(server_cert);
    mitm_dbg("Original server certificate:\n");
        //also, do NOT free these two names
    X509_NAME *server_cert_subj_name = X509_get_subject_name(server_cert);    CHK_NULL(server_cert_subj_name);
    X509_NAME *server_cert_issuer_name = X509_get_issuer_name(server_cert);    CHK_NULL(server_cert_issuer_name);    
/*
    char * server_cert_subj_name_str = X509_NAME_oneline(server_cert_subj_name, 0, 0);
    mitm_dbg("subject name: %s\n", server_cert_subj_name_str);
    char *server_cert_issuer_name_str = X509_NAME_oneline(server_cert_issuer_name, 0, 0);
    mitm_dbg("issuer name: %s\n", server_cert_issuer_name_str);
    mitm_dbg("subject name entry number: %d\n", serv_cert_subjname_ent_num);
    mitm_dbg("issuer name entry number: %d\n", X509_NAME_entry_count(server_cert_issuer_name));
*/  
    X509 *new_cert = makeAndInitCert();
    X509_NAME *new_name = X509_get_subject_name(new_cert);
    initNewName(new_name, server_cert_subj_name);
/*    
    char *newname_subjname = X509_NAME_oneline(new_name, 0, 0);
    mitm_dbg("new_name's subject name: %s\n", newname_subjname);
*/
    X509 *myCA = readX509(CERTF);    CHK_NULL(myCA);
    mitm_dbg("FAKE CA cert:\n");
    X509_NAME *caName = X509_get_subject_name(myCA);    CHK_NULL(caName);
/*    
    char *caName_str = X509_NAME_oneline(caName, 0, 0);
    mitm_dbg("FAKE CA name: %s\n", caName_str);

    X509_NAME *caIssuerName = X509_get_issuer_name(myCA);   CHK_NULL(caIssuerName);
    char *caIssuerName_str = X509_NAME_oneline(caIssuerName, 0, 0);
    mitm_dbg("FAKE CA issuer name: %s\n", caIssuerName_str);
*/  
        //set issuer name to myCA's subject name
    X509_set_issuer_name(new_cert, caName);
        //do NOT free these two keys, they are used by new_cert
    EVP_PKEY *privKey = readPrivateKey(KEYF);    CHK_NULL(privKey);
    EVP_PKEY *pubKey = readPublicKey(CERTF);    CHK_NULL(pubKey);
    X509_set_pubkey(new_cert, pubKey);
        //sign it
    if(!X509_sign(new_cert, privKey, EVP_md5()))
        CHK_NULL(NULL);
    
    return new_cert;
}

void MySocket::close(void)
{
    if(sockFd<0) return;
    
    ::close(sockFd);

    sockFd = -1;

    isSSL = false;
    
    if(ssl != NULL)
        SSL_free(ssl);

    if(ctx != NULL)
        SSL_CTX_free(ctx);

    ssl = NULL;
    ctx = NULL;
}

void MySocket::enableSSLServer(MySocket *clientSock)
{
    if(sockFd < 0) return;

    ctx = SSL_CTX_new (SSLv23_server_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(2);
    }
    
    if (SSL_CTX_use_certificate_chain_file(ctx, CERTF) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(3);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(4);
    }
    
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr,"Private key does not match the certificate public key\n");
        exit(5);
    }   

    X509 *new_cert = generateFakeCert(clientSock);    CHK_NULL(new_cert);
    if(SSL_CTX_use_certificate(ctx, new_cert) != 1) {
        ERR_print_errors_fp(stderr);
        exit(6);
    }
    
    ssl = SSL_new (ctx);                           CHK_NULL(ssl);
    SSL_set_fd (ssl, sockFd);
    int err = SSL_accept (ssl);                        CHK_SSL(err);
    mitm_dbg("SSL connection using %s\n", SSL_get_cipher (ssl));
    isSSL = true;
}

void MySocket::enableSSLClient(void)
{
    if(sockFd < 0) return;

    ctx = SSL_CTX_new (SSLv23_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(2);
    }

    ssl = SSL_new (ctx);                         CHK_NULL(ssl);
    SSL_set_fd (ssl, sockFd);
    int err = SSL_connect (ssl);                     CHK_SSL(err);
    
    mitm_dbg("SSL connection using %s\n", SSL_get_cipher (ssl));
/*
    X509 *server_cert = SSL_get_peer_certificate (ssl);       CHK_NULL(server_cert);
    mitm_dbg("Server certificate:\n");

    char *str = X509_NAME_oneline (X509_get_subject_name (server_cert),0,0);
    CHK_NULL(str);
    mitm_dbg("\t subject: %s\n", str);
    OPENSSL_free (str);

    str = X509_NAME_oneline (X509_get_issuer_name  (server_cert),0,0);
    CHK_NULL(str);
    mitm_dbg("\t issuer: %s\n", str);
    OPENSSL_free (str);
    X509_free (server_cert);
*/
    isSSL = true;
}

int MySocket::write(const void *buffer, int len)
{
    if(sockFd<0) return ENOT_CONNECTED;
    
    int ret;
    
    if(isSSL) {
        ret = SSL_write(ssl, buffer, len);
    } else {
        ret = ::write(sockFd, buffer, len);
    }    

    if(ret != len) return ESOCKET_ERROR;
    
    return ret;
}

bool MySocket::write_bytes(string buffer)
{
    return write_bytes(buffer.c_str(), buffer.size());
}

bool MySocket::write_bytes(const void *buffer, int len)
{
    const unsigned char *buf = (const unsigned char *) buffer;
    int bytesWritten = 0;

    while(len > 0) {
        bytesWritten = this->write(buf, len);
        if(bytesWritten <= 0) {
            return false;
        }
        buf += bytesWritten;
        len -= bytesWritten;
    }

    return true;

}

int MySocket::read(void *buffer, int len)
{
    if(sockFd<0) return ENOT_CONNECTED;
    
    int ret;
    
    if(isSSL) {
        ret = SSL_read(ssl, buffer, len);
    } else {
        ret = ::read(sockFd, buffer, len);
    }
    
    if(ret == 0) return ECONN_CLOSED;
    if(ret < 0) return ESOCKET_ERROR;
  
  return ret;
}


/*
//we should not use default ctor anymore  
MySocket::MySocket(void)
{
    sockFd = -1;
    isSSL = false;
    ctx = NULL;
    ssl = NULL;
}
*/
void MySocket::__enableSSLServer(void)
{
    if(sockFd < 0) return;

    ctx = SSL_CTX_new (SSLv23_server_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(2);
    }
    
    if (SSL_CTX_use_certificate_chain_file(ctx, CERTF) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(3);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(4);
    }
    
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr,"Private key does not match the certificate public key\n");
        exit(5);
    }   

    ssl = SSL_new (ctx);                           CHK_NULL(ssl);
    SSL_set_fd (ssl, sockFd);
    int err = SSL_accept (ssl);                        CHK_SSL(err);
  
    printf ("SSL connection using %s\n", SSL_get_cipher (ssl));
    isSSL = true;
}
