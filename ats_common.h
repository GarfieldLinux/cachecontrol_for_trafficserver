#ifndef ATS_COMMON
#define ATS_COMMON
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ts/ts.h"

#define MAXSIZE 1024

TSHttpStatus StaServerRespStatusGet(TSHttpTxn txnp);
TSReturnCode StaServerRespStatusSet(TSHttpTxn txnp,TSHttpStatus status);
TSReturnCode StaSerReqMimeHdrFieldDestroy(TSHttpTxn txnp,char *name,int length);
TSReturnCode StaCliReqMimeHdrFieldAppend(TSHttpTxn txnp,char *name,int name_length,char *value,int value_len);
TSReturnCode StaSerRespMimeHdrFieldDestroy(TSHttpTxn txnp,char *name,int length);
TSReturnCode StaSerRespMimeHdrFieldAppend(TSHttpTxn txnp,char *name,int name_length,char *value,int value_len);
char *GetUrlHost(char *url);
char *GetParamUrl(char *url);
int GetUrlType(char* url,int url_len,char* url_type);
#endif // ATS_COMMON
