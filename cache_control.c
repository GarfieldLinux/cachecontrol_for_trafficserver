#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include "ts/ts.h"
#include "ts/remap.h"
#include "/root/trafficserver/lib/ts/ink_defs.h"
#include "/root/trafficserver/lib/ts/ink_config.h"

#define PLUGIN_NAME "stateamcache"

#define MAXSIZE      1024
#define SUFFIXCOUNT  20
#define SUFFIXARGLEN 50
#define PATTERNCOUNT 20
typedef struct { 
    char          dest_domain[MAXSIZE];
    char          suffix[SUFFIXCOUNT][SUFFIXARGLEN]; 
	int           suffixcount;
	int           status;
    unsigned int  maxage;    
} pattern_info;

typedef struct {
    pattern_info pr_info[PATTERNCOUNT];
    int          patterncount; 
} pr_list;

void StrTrim(char *pStr)   
{   
    char *pTmpLeft = pStr;   
  
    while (*pTmpLeft == ' ')    
    {   
        pTmpLeft++;   
    }
    while(*pTmpLeft != '\0' && *pTmpLeft != ' ')
    {   
        *pStr = *pTmpLeft;   
        pStr++;   
        pTmpLeft++;
    }

    *pStr = '\0';
}

static int get_value(char *begin,char *value)
{
   char *p,*temp1;
   p = strstr(begin,"=");
   if(p){
       p = p + 1;
	   temp1 = p;
	   while(*temp1 != ' ' && (*temp1 != '\r' || *temp1 != '\n') && *temp1 != '\0' && *temp1 != '@')
		 temp1++;
	   strncpy(value,p,temp1 - p);
	   if(strlen(value) > 0)
	     return 1;
   }
   return 0;
}
static pr_list* load_config_file(const char *config_file) {
    char buffer[MAXSIZE];
    char default_config_file[MAXSIZE];
	TSFile fh;
    pr_list *prl = TSmalloc(sizeof(pr_list));
    prl->patterncount = 0;
	
	if (!config_file) {
	    memset(default_config_file,'\0',sizeof(default_config_file));
        sprintf(default_config_file, "%s/cachecontrol.config", TSPluginDirGet());
        config_file = (const char *)default_config_file;
    }
	
    fh = TSfopen(config_file, "r");

    if (!fh) {
        goto error;
    }
	char *p,*temp1,*temp2;
	char value[MAXSIZE];
	unsigned int  k;
	memset(buffer,'\0',sizeof(buffer));

	while (TSfgets(fh, buffer, sizeof(buffer) - 1)) {
	    if(buffer[0] == '#')
		{
		  memset(buffer,'\0',sizeof(buffer));
		  continue;
		}
	    TSDebug(PLUGIN_NAME, "%s",buffer);
	    p = strstr(buffer,"@dest_domain");
		if(p){
		   memset(value,'\0',sizeof(value));
		   if(get_value(p,value)){
		      memset(prl->pr_info[prl->patterncount].dest_domain,'\0',sizeof(prl->pr_info[prl->patterncount].dest_domain));
		      if(strcmp(value,".*") == 0)
			    prl->pr_info[prl->patterncount].dest_domain[0] = '0';
			  else
			    strcpy(prl->pr_info[prl->patterncount].dest_domain,value);
		   }else{
		      goto error;
		   }
		}else{
		   goto error;
		}
		
		p = strstr(buffer,"@suffix");
		if(p){
		   memset(value,'\0',sizeof(value));
		   if(get_value(p,value)){
		      if(strcmp(value,".*") == 0){
			      prl->pr_info[prl->patterncount].suffixcount = 1;
				  memset(prl->pr_info[prl->patterncount].suffix[0],'\0',sizeof(prl->pr_info[prl->patterncount].suffix[0]));
				  prl->pr_info[prl->patterncount].suffix[0][0] = '0';
			  }else{
				  k = 0;
				  prl->pr_info[prl->patterncount].suffixcount = 0;
				  temp1 = value;
				  temp2 = strstr(temp1,"|");
				  
				  while(temp2)
				  {
					 memset(prl->pr_info[prl->patterncount].suffix[k],'\0',sizeof(prl->pr_info[prl->patterncount].suffix[k]));
					 strcpy(prl->pr_info[prl->patterncount].suffix[k],".");
					 strncat(prl->pr_info[prl->patterncount].suffix[k],temp1,temp2 - temp1);
					 temp1 = temp2 + 1;
					 temp2 = strstr(temp1,"|");
					 k++;
				  }
				  memset(prl->pr_info[prl->patterncount].suffix[k],'\0',sizeof(prl->pr_info[prl->patterncount].suffix[k]));
				  strcpy(prl->pr_info[prl->patterncount].suffix[k],".");
				  strcat(prl->pr_info[prl->patterncount].suffix[k],temp1);
				  prl->pr_info[prl->patterncount].suffixcount = ++k;
				  if(prl->pr_info[k].suffixcount > SUFFIXCOUNT)
					 goto error;
			  }
		   }else{
		      goto error;
		   }
		}else{
		   goto error;
		}
		
		p = strstr(buffer,"@status");
		if(p){
		   memset(value,'\0',sizeof(value));
		   if(get_value(p,value)){
		      k = atoi(value);
			  if(k)
			    prl->pr_info[prl->patterncount].status = k;
			  else
			    goto error;
		   }else{
		      goto error;
		   }
		}else{
		   prl->pr_info[prl->patterncount].status = 200;
		}
		
		p = strstr(buffer,"@maxage");
		if(p){
		   memset(value,'\0',sizeof(value));
		   if(get_value(p,value)){
		      k = atoi(value);
			  if(k)
			    prl->pr_info[prl->patterncount].maxage = k;
			  else
			    goto error;
		   }else{
		      goto error;
		   }
		}else{
		   prl->pr_info[prl->patterncount].maxage = 86400;
		}
		prl->patterncount++;
        memset(buffer,'\0',sizeof(buffer));
    }
    TSfclose(fh);
	
	if(prl->patterncount <= PATTERNCOUNT){
	  TSDebug(PLUGIN_NAME, "Load Config Success!");
	  return prl;
	}
error:
    return NULL;
}
static void
modify_header(TSCont contp,TSHttpTxn txnp)
{
    TSHttpStatus resp_status;
	TSMBuffer resp_bufp;
    TSMLoc resp_loc;
	
	TSMBuffer req_bufp;
    TSMLoc req_loc;
	
	TSMLoc field_loc;
    char *EffectiveUrl=0;
	const char *dest_domain=0;
	char *p;
	//char *temp;
	char maxage[256];
    int Length;
	int k=0;
	int j;
	int frags=0;
	pattern_info *patn;
	pr_list *prl;
    prl = (pr_list *)TSContDataGet(contp);
	
	if (TSHttpTxnClientReqGet(txnp, &req_bufp, &req_loc) != TS_SUCCESS) {
		
		return;
	 }
	 
	field_loc = TSMimeHdrFieldFind(req_bufp, req_loc, TS_MIME_FIELD_HOST, TS_MIME_LEN_HOST);
	if(field_loc != TS_NULL_MLOC){
	    dest_domain = TSMimeHdrFieldValueStringGet(req_bufp,req_loc,field_loc,0,&Length);
		TSHandleMLocRelease(req_bufp, req_loc, field_loc);
		TSHandleMLocRelease(req_bufp, TS_NULL_MLOC, req_loc);
	}else{
		return;    
    }
	
	if (TSHttpTxnServerRespGet(txnp, &resp_bufp, &resp_loc) != TS_SUCCESS) {
        return;                  
    }

	EffectiveUrl = TSHttpTxnEffectiveUrlStringGet(txnp, &Length);
	if(!EffectiveUrl)
	  goto done;
	
	resp_status = TSHttpHdrStatusGet(resp_bufp, resp_loc);

	while(k < prl->patterncount)
	{
	   frags = 0;
	   patn = &prl->pr_info[k];
	   if(resp_status == patn->status)
	   {
		   
	       if(patn->dest_domain[0] == '0' || strncmp(dest_domain,patn->dest_domain,strlen(patn->dest_domain)) == 0){
		      frags = 1;
		   }else{
		      k++;
		      continue;
		   }
		   j = 0;
		   while(frags && j < patn->suffixcount)
		   {
		      p = strstr(EffectiveUrl,patn->suffix[j]);
			  
			  if(p || (patn->suffixcount == 1 && patn->suffix[0][0] == '0')){
			    memset(maxage,'\0',sizeof(maxage));
				sprintf(maxage,"max-age=%d",patn->maxage);				
			    goto addfield;
			  }
			  j++;
		   }
		   
	   }
	   k++;
	}
	goto done;
addfield:
	field_loc = TSMimeHdrFieldFind(resp_bufp, resp_loc, TS_MIME_FIELD_DATE, TS_MIME_LEN_DATE);
	if(field_loc != TS_NULL_MLOC){
	    TSMimeHdrFieldDestroy(resp_bufp,resp_loc,field_loc);
	    TSHandleMLocRelease(resp_bufp, resp_loc, field_loc);
	}
	
	field_loc = TSMimeHdrFieldFind(resp_bufp, resp_loc, TS_MIME_FIELD_AGE, TS_MIME_LEN_AGE);
	if(field_loc != TS_NULL_MLOC){
	    TSMimeHdrFieldDestroy(resp_bufp,resp_loc,field_loc);
	    TSHandleMLocRelease(resp_bufp, resp_loc, field_loc);
	}
	
	field_loc = TSMimeHdrFieldFind(resp_bufp, resp_loc, TS_MIME_FIELD_CACHE_CONTROL, TS_MIME_LEN_CACHE_CONTROL);
	if(field_loc != TS_NULL_MLOC){
	    TSMimeHdrFieldValuesClear(resp_bufp, resp_loc, field_loc);
	    TSMimeHdrFieldValueStringInsert(resp_bufp, resp_loc, field_loc, -1, maxage, strlen(maxage));
	    TSHandleMLocRelease(resp_bufp, resp_loc, field_loc);
	}else{
		if(TSMimeHdrFieldCreate(resp_bufp, resp_loc, &field_loc) == TS_SUCCESS) {
			TSMimeHdrFieldAppend(resp_bufp, resp_loc, field_loc);
			TSMimeHdrFieldValuesClear(resp_bufp, resp_loc, field_loc);
			TSMimeHdrFieldNameSet(resp_bufp, resp_loc, field_loc, TS_MIME_FIELD_CACHE_CONTROL, TS_MIME_LEN_CACHE_CONTROL);
			TSMimeHdrFieldValueStringInsert(resp_bufp, resp_loc, field_loc, -1, maxage, strlen(maxage));
			TSHandleMLocRelease(resp_bufp, resp_loc, field_loc);
		}
	}
done:
    if(EffectiveUrl) 
	{
	   TSfree(EffectiveUrl);
	}
	
	TSHandleMLocRelease(resp_bufp, TS_NULL_MLOC, resp_loc);	
    		
}


static int
cache_control_plugin(TSCont contp , TSEvent event, void *edata)
{
  TSHttpTxn txnp = (TSHttpTxn) edata;
  switch (event) {
  case TS_EVENT_HTTP_READ_RESPONSE_HDR:
    modify_header(contp,txnp);
    break;
  default:
    break;
  }
  TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
  return 0;
}

void TSPluginInit (int argc, const char *argv[])
{
  TSPluginRegistrationInfo info;
  pr_list *prl;
  
  info.plugin_name = "stateam";
  info.vendor_name = "stateamcache";
  info.support_email = "stateamcache@MyCompany.com";

  if (TSPluginRegister(TS_SDK_VERSION_3_0, &info) != TS_SUCCESS) {
     TSError("Plugin registration failed. \n");
     return;
  }
  if(!(prl=load_config_file(NULL)))
  {
     TSError("Plugin load_config_file failed. \n");
     return;
  }
  TSCont cont = TSContCreate(cache_control_plugin, NULL);
  TSContDataSet(cont, prl);
  
  TSHttpHookAdd(TS_HTTP_READ_RESPONSE_HDR_HOOK, cont);
}
