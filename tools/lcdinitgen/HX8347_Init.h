#define LCD_delay delay
#define WriteComm writecommand
#define WriteData writedata

void LCD_WR_REG(u16 Index, u16 CongfigTemp) {
  WriteComm(Index);
  WriteData(CongfigTemp);
}

void init() {
		//Power Voltage Setting 
LCD_WR_REG(0x1A,0x02);     //BT 
LCD_WR_REG(0x1B,0x88);     //VRH 
 
//****VCOM offset**/// 
LCD_WR_REG(0x23,0x00);     //SEL_VCM 
LCD_WR_REG(0x24,0xEE);     //VCM 
LCD_WR_REG(0x25,0x15);     //VDV 
 
LCD_WR_REG(0x2D,0x03);     //NOW[2:0]=011 
 
//Power on Setting 
LCD_WR_REG(0x18,0x1E);     //Frame rate 72Hz 
LCD_WR_REG(0x19,0x01);     //OSC_EN='1', start Osc 
LCD_WR_REG(0x01,0x00);     //DP_STB='0', out deep sleep 
LCD_WR_REG(0x1F,0x88);    //STB=0 
LCD_delay(5); 
LCD_WR_REG(0x1F,0x80);      //DK=0 
LCD_delay(5); 
LCD_WR_REG(0x1F,0x90);    //PON=1 
LCD_delay(5); 
LCD_WR_REG(0x1F,0xD0);    //VCOMG=1 
LCD_delay(5); 
 LCD_WR_REG(0x2F,0x00);
//262k/65k color selection 
LCD_WR_REG(0x17,0x05);     //default 0x06 262k color // 0x05 65k color 
 
//SET PANEL 
LCD_WR_REG(0x16,0xA0);		//A0：横屏；  00：竖屏
LCD_WR_REG(0x36,0x09);     //REV_P, SM_P, GS_P, BGR_P, SS_P 
LCD_WR_REG(0x29,0x31);     //400 lines 
LCD_WR_REG(0x71,0x1A);     //RTN 

//Gamma 2.2 Setting     
 LCD_WR_REG(0x40,0x01); 
 LCD_WR_REG(0x41,0x08); 
 LCD_WR_REG(0x42,0x04); 
 LCD_WR_REG(0x43,0x2D); 
 LCD_WR_REG(0x44,0x30); 
 LCD_WR_REG(0x45,0x3E); 
 LCD_WR_REG(0x46,0x02); 
 LCD_WR_REG(0x47,0x69); 
 LCD_WR_REG(0x48,0x07); 
 LCD_WR_REG(0x49,0x0E); 
 LCD_WR_REG(0x4A,0x12); 
 LCD_WR_REG(0x4B,0x14); 
 LCD_WR_REG(0x4C,0x17); 
 
 LCD_WR_REG(0x50,0x01); 
 LCD_WR_REG(0x51,0x0F); 
 LCD_WR_REG(0x52,0x12); 
 LCD_WR_REG(0x53,0x3B); 
 LCD_WR_REG(0x54,0x37); 
 LCD_WR_REG(0x55,0x3E); 
 LCD_WR_REG(0x56,0x16); 
 LCD_WR_REG(0x57,0x7D); 
 LCD_WR_REG(0x58,0x08); 
 LCD_WR_REG(0x59,0x0B); 
 LCD_WR_REG(0x5A,0x0D); 
 LCD_WR_REG(0x5B,0x11); 
 LCD_WR_REG(0x5C,0x18); 
 LCD_WR_REG(0x5D,0xFF); 
 
//LCD_WR_REG(0x1B,0x1B); //VRH 
//LCD_WR_REG(0x25,0x1B); //NVRH 
//LCD_WR_REG(0x1A,0x06); //BT 
//LCD_WR_REG(0x1E,0x13); //FS1 

 //Power Voltage Setting
WriteComm(0x1B);WriteData(0x001a); //VRH=4.65V 1b
WriteComm(0x1A);WriteData(0x0055); //BT (VGH~15V,VGL~-10V,DDVDH~5V)
WriteComm(0x24);WriteData(0x0010); //VMH(VCOM High voltage ~4.2V) 80
WriteComm(0x25);WriteData(0x0038); //VML(VCOM Low voltage -1.2V)58
 
//Display ON Setting 
LCD_WR_REG(0x28,0x38);      //GON=1, DTE=1, D=10 
LCD_delay(40); 
LCD_WR_REG(0x28,0x3C);      //GON=1, DTE=1, D=11 
 
WriteComm(0x22);               //Start GRAM write 

}