/* Copyright (C)2004 Landmark Graphics
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#define OPENSSL_NO_KRB5
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef __cplusplus
extern "C" {
#endif

SSL_CTX *hpsecnet_serverinit(char *certfile, char *privkeyfile);
SSL *hpsecnet_accept(int socket, SSL_CTX *sslctx);
void hpsecnet_disconnect(SSL *ssl);
SSL_CTX *hpsecnet_clientinit(void);
SSL *hpsecnet_connect(int socket, SSL_CTX *sslctx);
int hpsecnet_send(SSL *ssl, char *buf, int len);
int hpsecnet_recv(SSL *ssl, char *buf, int len);
void hpsecnet_term(SSL_CTX *sslctx);
char *hpsecnet_strerror(void);

#ifdef __cplusplus
}
#endif
