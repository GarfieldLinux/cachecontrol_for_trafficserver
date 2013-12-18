stateam_trafficserver
=====================

ats自带的cache.config不适合正向使用，当使用其ttl配置的时候会把range请求也当完整文件缓存，以致后续出现用户请求到非完整文件的错误。

因此我们自己写了个修改max-age的插件。可以通过域名、配合状态码来对缓存进行控制。

####编译安装

    tsxs -c cache_control.c -o cache_control.so
    
复制cache_control.so和cachecontrol.config配置文件到ats的插件目录下

配置plugin.config添加cache_control.so即可


####cachecontrol.config配置方式

    @dest_domain=.* @suffix=jpg|gif|png|flv|mp4|f4v|rar|zip|exe|iso|xls|doc|docx|xlsx|pdf @status=200 @maxage=5184000 
    
    @dest_domain=.* @suffix=.* @status=404 @maxage=120

由于加个@有助于代码处理，所以就这样做了。可以用#号进行注释。

####说明
dest_domain、suffix是必须的，status默认是200，maxage默认是86400

####另外
建议配合ats如下选项使用

    traffic_line -s proxy.config.http.cache.required_headers -v 2

    traffic_line -x


####关键代码

    define SUFFIXCOUNT  20   //设置每条配置最多可以匹配多少个文件后缀
    define PATTERNCOUNT 20   //设置最多可以有多少条配置

