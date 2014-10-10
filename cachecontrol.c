#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ts/ts.h"
#include "common.h"
#include "ats_common.h"

#define PLUGIN_NAME "longcache"
static Conf_Cache_Controls  cache_controls;
static int get_value(char *begin,char *value);
static int load_config_file(const char *config_file);
static void insert_cache_controls(int cur_count,char *dest_domain,int status,unsigned int maxage,char * suffix);
static void copy_cache_controls(Conf_Cache_Control *item_old,int capacity_old,Conf_Cache_Control *item_new,int capacity_new);
static void insert_item(Conf_Cache_Control *item,int capacity,char *dest_domain,int status,unsigned int maxage,char * suffix);

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
static void copy_cache_controls(Conf_Cache_Control *item_old,int capacity_old,Conf_Cache_Control *item_new,int capacity_new)
{
	Conf_Cache_Control item_temp;
	int i;
	for(i=0;i < capacity_old;i++)
	{
		item_temp = item_old[i];
		if(item_temp.flags == 1)
			insert_item(item_new,capacity_new,item_temp.dest_domain,item_temp.status,item_temp.maxage,item_temp.suffix);
	}
}

static void insert_item(Conf_Cache_Control *item,int capacity,char *dest_domain,int status,unsigned int maxage,char * suffix)
{
	int hash_val,new_hash_val;
	Conf_Cache_Control item_cache_control;
	char suffix_temp[20]={0};
	if(suffix[0] != '.')
		sprintf(suffix_temp,".%s",suffix);
	else
		strcpy(suffix_temp,suffix);
	hash_val = hash(suffix_temp)%capacity;
	item_cache_control = item[hash_val];
	while(item_cache_control.flags == 1 && item_cache_control.next != NULL){
		item_cache_control = *(item_cache_control.next);
		hash_val = item_cache_control.hash_val;
	}
	new_hash_val = hash_val;
	while(item[new_hash_val].flags == 1){
		new_hash_val = (new_hash_val + 1)%capacity;
	}
	memset(item[new_hash_val].suffix,'\0',sizeof(item[new_hash_val].suffix));
	strcpy(item[new_hash_val].suffix,suffix_temp);
	memset(item[new_hash_val].dest_domain,'\0',sizeof(item[new_hash_val].dest_domain));
	strcpy(item[new_hash_val].dest_domain,dest_domain);
	item[new_hash_val].flags = 1;
	item[new_hash_val].status = status;
	item[new_hash_val].maxage = maxage;
	item[new_hash_val].next = NULL;
	item[new_hash_val].hash_val = new_hash_val;
	//TSDebug(PLUGIN_NAME, "suffix:%s,hash_val:%d",item[new_hash_val].suffix,new_hash_val);
	if(new_hash_val != hash_val){
		item[hash_val].next = &item[new_hash_val];
		//TSDebug(PLUGIN_NAME, "cur_suffix:%s,curl_hash_val:%d,pre_suffix:%s,pre_hash_val:%d",
		//	item[hash_val].next->suffix,item[hash_val].next->hash_val,item[hash_val].suffix,item[hash_val].hash_val);
	}
}
static void insert_cache_controls(int cur_count,char *dest_domain,int status,unsigned int maxage,char * suffix)
{
	Conf_Cache_Control *item;
	int capacity;
	if(cur_count >= (cache_controls.capacity/2))
	{
		TSDebug(PLUGIN_NAME, "cache_controls.capacity:%d",cache_controls.capacity);
		capacity = next_prime(cache_controls.capacity + 1);
		item = calloc(capacity,sizeof(Conf_Cache_Control));
		if(cache_controls.item && cache_controls.capacity > 0){
			copy_cache_controls(cache_controls.item,cache_controls.capacity,item,capacity);
			TSDebug(PLUGIN_NAME, "free free");
			free(cache_controls.item);
		}
		cache_controls.capacity = capacity;
		cache_controls.item = item;
	}
	insert_item(cache_controls.item,cache_controls.capacity,dest_domain,status,maxage,suffix);
	
}
static int load_config_file(const char *config_file) {
    char default_config_file[MAXSIZE];
	TSFile fh;

	if (!config_file) {
	    memset(default_config_file,'\0',sizeof(default_config_file));
        sprintf(default_config_file, "%s/cachecontrol.config", "/sysconfig");
        config_file = (const char *)default_config_file;
    }
	
    fh = TSfopen(config_file, "r");

    if (!fh) {
        goto error;
    }
	
	char *p,*temp1;
	char value[MAXSIZE];
	char buffer[MAXSIZE];
	char dest_domain[MAXSIZE];
	char valueTemp[20];
	unsigned int  k;
	int status;
	unsigned int maxage;
	int cur_count = 0;
	memset(buffer,'\0',sizeof(buffer));
	while (TSfgets(fh, buffer, sizeof(buffer) - 1)) {
	    if(buffer[0] == '#')
		{
		  memset(buffer,'\0',sizeof(buffer));
		  continue;
		}
	    //TSDebug(PLUGIN_NAME, "%s",buffer);
	    p = strstr(buffer,"@dest_domain");
		if(p){
		   memset(value,'\0',sizeof(value));
		   if(get_value(p,value)){
			   memset(dest_domain,'\0',sizeof(dest_domain));
			   strcpy(dest_domain,value);
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
			    status = k;
			  else
			    goto error;
		   }else{
		      goto error;
		   }
		}else{
		   status = 200;
		}
		
		p = strstr(buffer,"@maxage");
		if(p){
		   memset(value,'\0',sizeof(value));
		   if(get_value(p,value)){
		      k = atoi(value);
			  if(k){
			    maxage = k;
			  }else{
			    if(strncmp(value,"0",1) == 0){
				   maxage = 0;
				}else{
				   goto error;
				}
			  }
		   }else{
		      goto error;
		   }
		}else{
		   maxage = 86400;
		}
		
		p = strstr(buffer,"@suffix");
		if(p){
		   memset(value,'\0',sizeof(value));
		   if(get_value(p,value)){
				temp1 = value;
				p = strstr(temp1,"|");
				while(p){
					memset(valueTemp,'\0',sizeof(valueTemp));
					strncpy(valueTemp,temp1,p - temp1);
					if(strlen(valueTemp) > 0){
						insert_cache_controls(cur_count,dest_domain,status, maxage,valueTemp);
						cur_count++;
					}
					temp1 = p + 1;
					if(temp1)
						p = strstr(temp1,"|");
					else
						p = temp1;
				}
				if(temp1 && strlen(temp1) > 0){
					insert_cache_controls(cur_count,dest_domain,status, maxage,temp1);
					cur_count++;
				}
		   }else{
		      goto error;
		   }
		}else{
		   goto error;
		}
        memset(buffer,'\0',sizeof(buffer));
    }
    TSfclose(fh);
	Conf_Cache_Control item;
	int j;
	for(j = 0; j < cache_controls.capacity; j++)
	{
		item = cache_controls.item[j];
		if(item.flags == 1)
			TSDebug(PLUGIN_NAME,"suffix:%s,hash_val:%d,dest_domain:%s,maxage:%d,status:%d,flags:%d",
				item.suffix,item.hash_val,item.dest_domain,item.maxage,item.status,item.flags);
	}
	TSDebug(PLUGIN_NAME,"Load Config Success!");
	return 1;
error:
	if (fh) {
        TSfclose(fh);
    }
    return 0;
}
static void
modify_header(TSCont contp,TSHttpTxn txnp)
{
	char *url;
    int  url_length;
	char *url_host;
	int  hash_val;
	char filename[10];
	char maxage[256];
	TSHttpStatus resp_status;
	url = TSHttpTxnEffectiveUrlStringGet(txnp, &url_length);
	if(!url)
		return;
	url_host = GetUrlHost(url);
	resp_status = StaServerRespStatusGet(txnp);
	memset(filename,'\0',sizeof(filename));
	GetUrlType(url,url_length,filename);
	if(strlen(filename) <= 0){
		memset(filename,'\0',sizeof(filename));
		strcpy(filename,".*");
	}
	
	hash_val = hash(filename)%cache_controls.capacity;
	Conf_Cache_Control item = cache_controls.item[hash_val];
	//TSDebug(PLUGIN_NAME,"filename:%s,hash_val:%d",filename,hash_val);
	while(item.flags == 1){
		if(!strcmp(item.suffix,filename) && resp_status == item.status && 
			(!strcmp(url_host,item.dest_domain) || !strcmp(".*",item.dest_domain))){
			memset(maxage,'\0',sizeof(maxage));
			sprintf(maxage,"max-age=%d",item.maxage);
			//TSDebug(PLUGIN_NAME,"maxage:%s,filename:%s",maxage,filename);
			goto addfield;
		}
		if(item.next){
			item = *(item.next);
			//TSDebug(PLUGIN_NAME,"suffix:%s,hash_val:%d,status:%d",item.suffix,item.hash_val,item.status);
		}
		else
			break;
	}
	if(TS_HTTP_STATUS_OK != resp_status)
		TSHttpTxnServerRespNoStoreSet(txnp,1);
	goto done;
addfield:
	StaSerRespMimeHdrFieldDestroy(txnp,(char *)TS_MIME_FIELD_DATE, TS_MIME_LEN_DATE);
	StaSerRespMimeHdrFieldDestroy(txnp,(char *)TS_MIME_FIELD_AGE, TS_MIME_LEN_AGE);
	StaSerRespMimeHdrFieldAppend(txnp,(char *)TS_MIME_FIELD_CACHE_CONTROL, TS_MIME_LEN_CACHE_CONTROL,maxage,strlen(maxage));
done:
	if(url){
		TSfree(url);
	}
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
  /*
  TSPluginRegistrationInfo info;
  
  info.plugin_name = (char*)"stateam";
  info.vendor_name = (char*)"stateam";
  info.support_email = (char*)"stateam@MyCompany.com";

  if (TSPluginRegister(TS_SDK_VERSION_3_0, &info) != TS_SUCCESS) {
     return;
  }*/
  if(!load_config_file(NULL))
  {
     TSError("Plugin load_config_file failed. \n");
     return;
  }
  TSCont cont = TSContCreate(cache_control_plugin, NULL);
  TSHttpHookAdd(TS_HTTP_READ_RESPONSE_HDR_HOOK, cont);
}
