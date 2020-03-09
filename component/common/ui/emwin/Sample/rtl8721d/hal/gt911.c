#include "gt911.h"
#include "string.h"
#include "diag.h"
#include "i2c_api.h"

#define delay_ms	DelayMs
#define printf		DBG_8195A
#define XSIZE	 	480
#define YSIZE		272
#define  ADDR   0x5d
#define I2C_BUS_CLK  100000

i2c_t obj;
_m_tp_dev tp_dev;
const u16 GT911_TPX_TBL[5]={GT_TP1_REG,GT_TP2_REG,GT_TP3_REG,GT_TP4_REG,GT_TP5_REG};

const u8 GT911_CFG_TBL[]=   
{ 
  0x68,0x20,0x03,0xE0,0x01,0x05,0x3D,0x00,0x01,0x48, 
  0x28,0x0D,0x50,0x32,0x03,0x05,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x18,0x1A,0x1E,0x14,0x8A,0x2A,0x0C,
  0x30,0x38,0x31,0x0D,0x00,0x00,0x02,0xB9,0x03,0x2D,
  0x00,0x00,0x00,0x00,0x00,0x03,0x64,0x32,0x00,0x00,
  0x00,0x1D,0x41,0x94,0xC5,0x02,0x07,0x00,0x00,0x04,
  0xA5,0x1F,0x00,0x94,0x25,0x00,0x88,0x2B,0x00,0x7D,
  0x33,0x00,0x74,0x3C,0x00,0x74,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x18,0x16,0x14,0x12,0x10,0x0E,0x0C,0x0A,
  0x08,0x06,0x04,0x02,0xFF,0xFF,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x24,0x22,0x21,0x20,0x1F,0x1E,0x1D,0x1C,
  0x18,0x16,0x13,0x12,0x10,0x0F,0x0A,0x08,0x06,0x04,
  0x02,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,
 
};  

u8 GT911_Send_Cfg(u8 mode)
{
	u8 buf[2];
	u8 i=0;
	buf[0]=0;
	buf[1]=mode;
	for(i=0;i<sizeof(GT911_CFG_TBL);i++)buf[0]+=GT911_CFG_TBL[i];
	buf[0]=(~buf[0])+1;
	
	GT911_WR_Reg(GT_CFGS_REG,(u8*)GT911_CFG_TBL,sizeof(GT911_CFG_TBL));
	GT911_WR_Reg(GT_CHECK_REG,buf,2);
	
	return 0;
} 

void GT_RST(int x)
{
	GPIO_WriteBit(RST_PIN, x);
}

void GT911_RD_Reg(u16 reg,u8 *buf,u8 len)
{
	u16 r;
	r = reg&0xff;

	reg = (reg>>8)|(r<<8);
	i2c_write(&obj, ADDR,&reg, 2, 1);
	i2c_read(&obj, ADDR, buf, len, 1);
}
	  
u8 GT911_WR_Reg(u16 reg,u8 *buf,u8 len)
{
	u8 *temp;
	temp =  pvPortMalloc(len + 2);
	temp[1] = reg&0xff;
	temp[0] = (reg>>8);

	_memcpy(temp + 2, buf, len);	
	 i2c_write(&obj, ADDR, temp, len + 2, 2);
	 vPortFree(temp);
	 
	 return 1;
}

u8 GT911_WR_Reg8(u16 reg,u8 *buf)
{
	u8 temp[3];
	temp[2] = *buf;
	temp[1] = reg&0xff;
	temp[0] = (reg>>8);

	i2c_write(&obj, ADDR, temp, 3, 2);
	 
	return 1;
}

u8 GT911_Init(void)
{
	u8 temp[4];
	u16 i = 0;
	GPIO_InitTypeDef GPIO_InitStructure;					
	I2C_InitTypeDef I2C_InitStruct;
	
	tp_dev.touchtype = 1;
	/*init RST/INT pin*/
	GPIO_InitStructure.GPIO_Pin = RST_PIN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	PAD_PullCtrl(RST_PIN, GPIO_PuPd_UP);
	GPIO_Init(&GPIO_InitStructure);
	PAD_PullCtrl(RST_PIN, GPIO_PuPd_UP);
	GPIO_WriteBit(RST_PIN, 0);
	
	i2c_init(&obj, SDA_PIN, SCL_PIN);  
	i2c_frequency(&obj, I2C_BUS_CLK);
	
	GT_RST(0);
	DelayMs(10);
	GT_RST(1);
	DelayMs(50);

	GT911_RD_Reg(GT_PID_REG,temp,4);
	temp[4]=0;
	printf("%s\r\n",temp);	
	printf("TouchPad_ID:%d,%d,%d\r\n",temp[0],temp[1],temp[2]);	
	
	if(strcmp((char*)temp,"911")==0)
	{
		temp[0]=0X02;			
		GT911_WR_Reg(GT_CTRL_REG,temp,1);
		GT911_RD_Reg(GT_CFGS_REG,temp,1);
		printf("version:%x\r\n",temp[0]);
		if(temp[0]<0X68)
		{
			printf("Default Ver:%x\r\n",temp[0]);
			GT911_Send_Cfg(1);
		}
			
		DelayMs(10);
		temp[0]=0X00;	 
		GT911_WR_Reg(GT_CTRL_REG,temp,1); 
		return 0;
	} 

	return 1;
}

u8 GT911_Scan(u8 mode)
{
	u8 buf[4];
	u8 i=0;
	u8 res=0;
	u8 temp;
	u8 tempsta;
	
 	static u8 t=0;
	t++;
	
	if((t%2)==0||t<2)
	{
		GT911_RD_Reg(GT_GSTID_REG,&mode,1);	
 		if(mode&0X80&&((mode&0XF)<6))
		{
			temp=0;
			GT911_WR_Reg8(GT_GSTID_REG,&temp);		
		}		
		if((mode&0XF)&&((mode&0XF)<6))
		{
			temp=0XFF<<(mode&0XF);		 
			tempsta=tp_dev.sta;			
			tp_dev.sta=(~temp)|TP_PRES_DOWN|TP_CATH_PRES; 
			tp_dev.x[4]=tp_dev.x[0];	
			tp_dev.y[4]=tp_dev.y[0];
			for(i=0;i<1;i++) 
			{
				if(tp_dev.sta&(1<<i))	
				{
					GT911_RD_Reg(GT911_TPX_TBL[i],buf,4);	
					
					if(tp_dev.touchtype&0X01)
					{
						tp_dev.x[i]=(((u16)(buf[1]&0X0F)<<8)+buf[0]);
						//if(tp_dev.x[i]<420)tp_dev.x[i]=415-tp_dev.x[i];
						tp_dev.y[i]=((u16)(buf[3]&0X0F)<<8)+buf[2];

						tp_dev.x[i] = XSIZE-1- (tp_dev.x[i] -100)*480/700;
						tp_dev.y[i] = (tp_dev.y[i] -100)*YSIZE/380;
					}else{
						tp_dev.y[i]=((u16)(buf[1]&0X0F)<<8)+buf[0];
						tp_dev.x[i]=((u16)(buf[3]&0X0F)<<8)+buf[2];
						}
					
					//if((buf[0]&0XF0)!=0X80)tp_dev.x[i]=tp_dev.y[i]=0;
 					//printf("buf:%d,%d,%d,%d\n",buf[0],buf[1],buf[2],buf[3]);
					//printf("x[%d]:%d,y[%d]:%d\r\n",i,tp_dev.x[i],i,tp_dev.y[i]);	
				}			
			} 
			res=1;
			if(tp_dev.x[0]>XSIZE||tp_dev.y[0]>YSIZE)
			{ 
				if((mode&0XF)>1)		
				{
					tp_dev.x[0]=tp_dev.x[1];
					tp_dev.y[0]=tp_dev.y[1];
					t=0;				
				}else					
				{
					tp_dev.x[0]=tp_dev.x[4];
					tp_dev.y[0]=tp_dev.y[4];
					mode=0X80;		
					tp_dev.sta=tempsta;	
				}
			}else t=0;					
		}
	}
	
	if((mode&0X8F)==0X80)
	{ 
		if(tp_dev.sta&TP_PRES_DOWN)	
		{
			tp_dev.sta&=~(1<<7);	
		}else						
		{ 
			tp_dev.x[0]=0xffff;
			tp_dev.y[0]=0xffff;
			tp_dev.sta&=0XE0;
		}	 
	}
	
	if(t>240)t=2;
	
	return res;
}
 
void GT911_Reset_Sequence(uint8_t ucAddr) //δʹ��
{ 
	switch(ucAddr) 
	{ 
		case 0xBA: 
			GT911_RST_0(); 
			GT911_INT_0(); 
			delay_ms(30); 
			GT911_RST_1(); 
			GT911_INT_0();  
			delay_ms(30);
			GT911_INT_0(); 
			delay_ms(30); 
			GT911_INT_1(); 
			break; 
		case 0x28:
			GT911_RST_0();
			GT911_INT_1();
			delay_ms(30);
			GT911_RST_1(); 
			GT911_INT_1(); 
			delay_ms(30); 
			GT911_INT_0(); 
			delay_ms(30); 
			GT911_INT_1(); 
			break; 
		default: 
			GT911_RST_0(); 
			GT911_INT_0(); 
			delay_ms(30);
			GT911_RST_1(); 
			GT911_INT_0();
			delay_ms(30);
			GT911_INT_0(); 
			delay_ms(30);
			GT911_INT_1(); 
			break; 

	} 
}

void GT911_Soft_Reset(void)
{ 
	uint8_t buf[1]; 
	buf[0] = 0x01; 
	GT911_WR_Reg(GT911_COMMAND_REG, (uint8_t *)buf, 1); 
} 

