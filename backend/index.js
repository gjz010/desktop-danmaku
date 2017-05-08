const Weixinbot=require('weixinbot');
//const bot=new Weixinbot()
var net=require('net');
var bot=new Weixinbot();
var client=new net.Socket();
client.connect(6789,"127.0.0.1",function(){
	console.log("Connected to the server!");
	//console.log(Buffer("野兽先辈").length)
});

bot.on('qrcode', (qrcodeUrl) => {
  console.log(qrcodeUrl)
})
bot.on('friend', (msg) => {
  console.log(msg.Member.NickName + ': ' + msg.Content);
  bot.sendText(msg.FromUserName, 'Got it');

});
function escape2Html(str) { 
 var arrEntities={'lt':'<','gt':'>','nbsp':' ','amp':'&','quot':'"'}; 
 return str.replace(/&(lt|gt|nbsp|amp|quot);/ig,function(all,t){return arrEntities[t];}); 
} 
bot.on('group',(msg)=>{
	//console.log(msg.Group.NickName);
	console.log(msg.Content);
	data=escape2Html(msg.Content);
	data=data.replace(/<span.*><\/span>/g,"");
	if(data.indexOf("url")!=-1){
		console.log("Emoji filtered.");
		return;
	}
	if(msg.Group.NickName.indexOf("1-4")!=-1 || msg.Group.NickName.indexOf("203B")!=-1){
			if(data!="")
				if(msg.Group.NickName.indexOf("舒衡哲")!=-1){
					arr=data.split("<br/>");
					for(str in arr){
						str=arr[str]
						client.write(Buffer(str).length+"*"+str);
						
					}
				}else{
					data=data.replace("<br/>"," ");
					if(data.length<=30)
					client.write(Buffer(data).length+" "+data);
					
				}
			
			//client.publish("desktop_danmaku",msg.Content);
	}
	
});
bot.run();
