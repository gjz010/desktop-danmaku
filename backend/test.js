//const Weixinbot=require('weixinbot');
//const bot=new Weixinbot()
var net=require('net');
var client=new net.Socket();
client.connect(6789,"127.0.0.1",function(){
	console.log("Connected to the server!");
	//console.log(Buffer("野兽先辈").length)
});

setInterval(()=>{
	data="hahaha"
	buf=Buffer(data)
	client.write(buf.length+" "+buf)
	
},50)