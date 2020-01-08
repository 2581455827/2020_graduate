#接线
	蜂鸣器
		IO->PE13
	火光
		DO->PE10
	温湿度
		+->5V
		-->GND
		SDA->PE11
		SCL->GND
	风扇
		INB->PE12
	烟雾
		AO->PC4
	PM2.5
		VCC->5V
		GND->GND
		查看此链接
		https://item.taobao.com/item.htm?spm=a1z09.2.0.0.a9f92e8drLjMIN&id=601897516408&_u=21kv63vk4254
	光照
		VCC 5V
		GND GND
		ADO GND
		SCL PC1
		SDA PC2
#数据格式
	112234444.455556666
	11温度
	22湿度
	3是否火灾
	4444.4 PM2.5
	5555光强
	6666烟雾
#说明
	烟雾大于1400，自动开风扇
	发现火灾，蜂鸣器报警
