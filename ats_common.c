#include "ats_common.h"
//获取源服务器响应的状态
TSHttpStatus StaServerRespStatusGet(TSHttpTxn txnp)
{
	TSMBuffer resp_bufp;
    TSMLoc resp_loc;
	TSHttpStatus resp_status = TS_HTTP_STATUS_NONE;
	if (TSHttpTxnServerRespGet(txnp, &resp_bufp, &resp_loc) != TS_SUCCESS) {
        return TS_HTTP_STATUS_NONE;                  
    }
	resp_status = TSHttpHdrStatusGet(resp_bufp, resp_loc);
	TSHandleMLocRelease(resp_bufp, TS_NULL_MLOC, resp_loc);
	return resp_status;
}
//设置源服务器响应的状态
TSReturnCode StaServerRespStatusSet(TSHttpTxn txnp,TSHttpStatus status)
{
	TSMBuffer resp_bufp;
    TSMLoc resp_loc;
	TSReturnCode ret;
	if (TSHttpTxnServerRespGet(txnp, &resp_bufp, &resp_loc) != TS_SUCCESS) {
        return TS_ERROR;                  
    }
	ret = TSHttpHdrStatusSet(resp_bufp, resp_loc, status);
	TSHandleMLocRelease(resp_bufp, TS_NULL_MLOC, resp_loc);
	return ret;
}
//删除向源服务器请求时的报文头
TSReturnCode StaSerReqMimeHdrFieldDestroy(TSHttpTxn txnp,char *name,int length)
{
	TSMBuffer bufp;
    TSMLoc hdr_loc;
    TSMLoc field_loc;
	TSReturnCode ret = TS_ERROR;
	if (TSHttpTxnServerReqGet(txnp, &bufp, &hdr_loc) != TS_SUCCESS) {
		return TS_ERROR;
	}
	field_loc = TSMimeHdrFieldFind(bufp, hdr_loc, name, length);
	if(field_loc != TS_NULL_MLOC)
	{
	    ret = TSMimeHdrFieldDestroy(bufp,hdr_loc,field_loc);
		TSHandleMLocRelease(bufp, hdr_loc, field_loc);
	}
	TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);
	return ret;
}
//向客户端请求添加报文头
TSReturnCode StaCliReqMimeHdrFieldAppend(TSHttpTxn txnp,char *name,int name_length,char *value,int value_len)
{
	TSMBuffer bufp;
    TSMLoc hdr_loc;
    TSMLoc field_loc;
	if (TSHttpTxnClientReqGet(txnp, &bufp, &hdr_loc) != TS_SUCCESS) {
		return TS_ERROR;
	}
	field_loc = TSMimeHdrFieldFind(bufp, hdr_loc, name, name_length);
	if(field_loc == TS_NULL_MLOC){
		if (TSMimeHdrFieldCreate(bufp, hdr_loc, &field_loc) != TS_SUCCESS) {
			return TS_ERROR;
		}
		TSMimeHdrFieldAppend(bufp, hdr_loc, field_loc);
		TSMimeHdrFieldValuesClear(bufp, hdr_loc, field_loc);
		TSMimeHdrFieldNameSet(bufp, hdr_loc, field_loc, name, name_length);
		TSMimeHdrFieldValueStringInsert(bufp, hdr_loc, field_loc, -1, value, value_len);
		TSHandleMLocRelease(bufp, hdr_loc, field_loc);
	}
	TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);
	return TS_SUCCESS;
}
//响应删除报文头
TSReturnCode StaSerRespMimeHdrFieldDestroy(TSHttpTxn txnp,char *name,int length)
{
	TSMBuffer bufp;
    TSMLoc hdr_loc;
    TSMLoc field_loc;
	TSReturnCode ret = TS_ERROR;
	if (TSHttpTxnServerRespGet(txnp, &bufp, &hdr_loc) != TS_SUCCESS) {
		return TS_ERROR;
	}
	field_loc = TSMimeHdrFieldFind(bufp, hdr_loc, name, length);
	if(field_loc != TS_NULL_MLOC)
	{
	    ret = TSMimeHdrFieldDestroy(bufp,hdr_loc,field_loc);
		TSHandleMLocRelease(bufp, hdr_loc, field_loc);
	}
	TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);
	return ret;
}

//响应添加报文头
TSReturnCode StaSerRespMimeHdrFieldAppend(TSHttpTxn txnp,char *name,int name_length,char *value,int value_len)
{
	TSMBuffer bufp;
    TSMLoc hdr_loc;
    TSMLoc field_loc;
	if (TSHttpTxnServerRespGet(txnp, &bufp, &hdr_loc) != TS_SUCCESS) {
		return TS_ERROR;
	}
	field_loc = TSMimeHdrFieldFind(bufp, hdr_loc, name, name_length);
	if(field_loc != TS_NULL_MLOC){
	    TSMimeHdrFieldValuesClear(bufp, hdr_loc, field_loc);
	    TSMimeHdrFieldValueStringInsert(bufp, hdr_loc, field_loc, -1, value, value_len);
	    TSHandleMLocRelease(bufp, hdr_loc, field_loc);
	}else{
		if (TSMimeHdrFieldCreate(bufp, hdr_loc, &field_loc) != TS_SUCCESS) {
			return TS_ERROR;
		}
		TSMimeHdrFieldAppend(bufp, hdr_loc, field_loc);
		TSMimeHdrFieldValuesClear(bufp, hdr_loc, field_loc);
		TSMimeHdrFieldNameSet(bufp, hdr_loc, field_loc, name, name_length);
		TSMimeHdrFieldValueStringInsert(bufp, hdr_loc, field_loc, -1, value, value_len);
		TSHandleMLocRelease(bufp, hdr_loc, field_loc);
	}
	TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);
	return TS_SUCCESS;
}
//获取域名
char *GetUrlHost(char *url)
{
	char *p,*temp;
	char url_host[100];
	memset(url_host,'\0',sizeof(url_host));
	p = strstr(url,"http://");
	if(p){
		p = p + strlen("http://");
		temp = strstr(p,"/");
		if(temp)
			strncpy(url_host,p,temp - p);
	}
	p = url_host;
	return p;
}
//获取url的参数
char *GetParamUrl(char *url)
{
	char *p;
	char param_url[MAXSIZE];
	memset(param_url,'\0',sizeof(param_url));
	p = strstr(url,"?");
	if(p){
		strcpy(param_url,p);
	}
	p = param_url;
	return p;
}

//获取url的后缀
int GetUrlType(char* url,int url_len,char* url_type)
{
    int check_count = 10;
	char *p,*temp;
	p = strstr(url,"?");
	int i = 0;
	if(!p){
		p = url + url_len;
		temp = p;
		while(p && (*p == '\n' || *p == '\r' || *p == '\0')){
			temp = p;
			p--;
		}
		p = temp;
	}
	temp = p;
	while(temp && *temp != '.' && i < check_count){
		temp = temp - 1;
		i++;
	}
	if(temp && i < check_count ){
		strncpy(url_type,temp,p - temp);
		return 1;
	}
	return 0;
}



