开发团队：[stateam团队站点](http://www.stateam.net) - [www.stateam.net](http://www.stateam.net)    

公司介绍：广州可道技术


----    

2014-10-10 重构优化了算法，效率有比较大提升。     


----     
[测试环境：centos6，ats版本：4.2.0]

ats自带的cache.config不适合正向使用，当使用其ttl配置的时候会把range请求也当完整文件缓存，以致后续出现用户请求到非完整文件的错误。

因此我们自己写了个修改max-age的插件。可以通过域名、配合状态码来对缓存进行控制。

####make编译安装

    make

我们写了makefile文件，直接make编译安装即可
复制cachecontrol.so配置文件到ats的插件目录下    
cachecontrol.config控制文件放到 /sysconfig 目录下

配置plugin.config添加cachecontrol.so即可


####cachecontrol.config配置方式

    @dest_domain=.* @suffix=jpg|gif|png|flv|mp4|f4v|rar|zip|exe|iso|xls|doc|docx|xlsx|pdf @status=200 @maxage=5184000 
    
    @dest_domain=.* @suffix=.* @status=404 @maxage=120

由于加个@有助于代码处理，所以就这样做了。可以用#号进行注释。

####说明
dest_domain、suffix是必须的，status默认是200，maxage默认是86400

由于缓存代理服务器之间会继承age，会导致修改max-age不成功，所以代码中把源返回的header中的date字段删除。

####另外
建议配合ats如下选项使用

    traffic_line -s proxy.config.http.cache.required_headers -v 2
    traffic_line -s proxy.config.http.cache.heuristic_max_lifetime -v 17280000

    traffic_line -x

####注意

cache.config的配置会覆盖此插件功能，如果使用此插件建议就不用使用cache.config了，除非你已经很清楚它们之间的影响关系。

proxy.config.http.cache.heuristic_max_lifetime上面这项的值要设置得不比你设置的maxage小。


####关键代码说明

    define SUFFIXCOUNT  20   //设置每条配置最多可以匹配多少个文件后缀
    define PATTERNCOUNT 20   //设置最多可以有多少条配置

