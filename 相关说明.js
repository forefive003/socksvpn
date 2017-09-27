1.准备工作:
1.1)运行socks_server,如下:

./socks_server -r 139.*.*.48 -p 22225 -m rc4 -e "password"    //旧接口

./socks_server -url "http://www.domain.com/getip" -m rc4 -e "password" -sn "AABBCCDD002233bb" -d 1  //新接口,去掉-r参数和-p参数,采用第5点进行动态获取relay_server IP
其中：
-url是动态获取relay_server IP的http url
-sn是socks_server序列号
-d是debug,默认是不打印日志信息和保存日志信息,只有特意指定了-d参数才支持打印和保存日志信息,取值范围(0不支持,1支持)

socks_server通过参数指定的url,如:http://www.domain.com/getip,可以得到如下json报文:
{
	"ip":"202.*.*.10", //relay_server ip	
	"port":22228 //relay_server port
}

relay_server动态获取到IP地址和端口后,用于socks_server初始化。
当relay_server踢掉了当前的这个socks_server时,需要重新通过-url的参数值来动态获取一个新的relay_server IP重新初始化。
这里的踢掉,需要relay_server删除被踢掉的socks_server内存信息。

1.2)运行relay_server,如下:
./relay_server -api "http://www.domain.com/api" -sn "xnkwlhqicwnk" -password "aaaa"
其中：
-api是http通信接口
-sn是relay_server序列号
-password是relay_server访问密码

relay_server运行成功后,通过api参数值发起http post进行注册
接口: http://www.domain.com/api   (此接口由relay_server运行时指定)
请求参数JSON报文:
{
	"cmd":1,
	"relaysn":"sakjgukagklnknk",//relay服务器id
	"password":"aaaa",//relay服务器id
	"port":22228 //relay_server port 新增20170719
}
响应JSON报文(0为成功,1为失败),如下:

成功json报文:
{
	"code":0    
}
失败json报文:
{
	"code":1   
}

备注:当relay_server进行注册时,管理平台需要获取其relaysn,password,外网ip,便于后期访问其socket接口

-----------------------------------------------------------------------------------------------------

2.socks_server成功上线后,relay server发起http post提交,更新管理平台数据库数据,如下:
接口: http://www.domain.com/api   (此接口由relay_server运行时指定)
请求参数JSON报文:
{
	"cmd":2,
	"relaysn":"sakjgukagklnknk"	//relay服务器id 新增20170719
	"sn":"AABBCCDD002233bb",//socks_server序列号,通过运行时参数指定
	"pub-ip":"202.1.1.2", //socks_server外网ip
	"pri-ip":"192.168.1.1",//socks_server内网ip
	"online":1 //上线
}
响应JSON报文(0为成功,1为失败),如下:

成功json报文:
{
	"code":0    
}
失败json报文:
{
	"code":1   
}

-----------------------------------------------------------------------------------------------------

3.relay server 发起http get请求,获取当前配置:
接口: http://www.domain.com/api?relaysn=sakjgukagklnknk (此接口由relay_server运行时指定)
请求参数:
-relaysn是relay_server序列号

响应JSON报文(0为成功,1为失败),如下:
成功json报文:
{
	code:0,
	"list":[
			{
				"sn":"AABBCCDD002233",//socks_server sn
				"pub-ip":"202.1.1.2", //socks_server 公网ip
				"pri-ip":"192.168.1.1", //socks_server 内网ip
				"users":[
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
						]
		    },
		    {
				"sn":"AABBCCDD002244",
				"pub-ip":"202.1.1.3", 
				"pri-ip":"192.168.0.1",
				"users":[
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
						]
		    },
	    ......
	    ]
}

失败json报文:
{
	code:1,
	"list":[]
}

-----------------------------------------------------------------------------------------------------

4.管理平台通过以下接口进行设置用户名和密码,以及授权管理。
relay_server监听22223端口,允许外部程序(管理平台程序)通过其api让relay_server更新本地内存数据
请求参数JSON报文:
{
	"cmd":1,
	"relaysn":"sakjgukagklnknk",//relay_server需要验证其合法性
	"password":"aaaa",//relay_server需要验证其合法性
	"body":{
				"sn":"AABBCCDD002233bb",//socks_server序列号,通过运行时参数指定
				"pub-ip":"202.1.1.2", //socks_server外网ip
				"pri-ip":"192.168.1.1",//socks_server内网ip
				"users":[
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
							｛ "username":"hahaha1", "passwd":"xxxx", "enabled":1},
						]
			}
}

备注:其中的users json数组数量为1至5,即支持批量设置

响应JSON报文(0为成功,1为失败),如下:

成功json报文:
{
	"code":0    
}
失败json报文:
{
	"code":1   
}

5.负载均衡接口,管理平台通过relay_server监听的22223端口,告知让某个socks_server进行网络切换
请求参数JSON报文:
{
	"cmd":2,
	"relaysn":"sakjgukagklnknk",//relay_server需要验证其合法性
	"password":"aaaa",//relay_server需要验证其合法性
	"sn":"AABBCCDD002233bb"//socks_server sn
}

响应JSON报文(0为成功,1为失败),如下:

成功json报文:
{
	"code":0    
}
失败json报文:
{
	"code":1   
}

注:relay_server收到其命令后,通过socks_server和relay_server之间的连接告知socks_server进行重新获取动态relay_server IP地址,同时,relay_server需要删除被踢掉的socks_server内存信息。






