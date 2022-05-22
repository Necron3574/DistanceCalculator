#include <lpc17xx.h>
#include<stdlib.h>
#include<stdio.h>
#define RS_CTRL 0x08000000 //P0.27, 1<<27
#define EN_CTRL 0x10000000 //P0.28, 1<<28
#define DT_CTRL 0x07800000 //P0.23 to P0.26 data lines, 1<<23
#define PRESCALE (25-1)
#define ITM_Port8(n) (*((volatile unsigned char *)(0xE0000000+4*n)))
#define ITM_Port16(n) (*((volatile unsigned short*)(0xE0000000+4*n)))
#define ITM_Port32(n) (*((volatile unsigned long *)(0xE0000000+4*n)))
#define DEMCR (*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA 0x01000000


struct __FILE {
	int handle;
 };
FILE __stdout;
FILE __stdin;



void init(void);
void clearPorts(void);
void writeCmd(void);
void writeData(void);
void disp(char *);
void disp_with_delay(char *);
void move(int l, int c);
void clear_display(void);
void initTimer0(void);
void startTimer0(void);
unsigned int stopTimer0(void);
void delayUS(unsigned int microseconds);
void delayMS(unsigned int milliseconds);
int cmd=0x0, data=0x00, flag=0, j,i,j1;
void init()
{
	LPC_PINCON->PINSEL1 = 0xFC003FFF;
	LPC_GPIO0->FIODIRH = 0x1F80;
	clearPorts();
	flag=0;

	for(i=0;i<3;i++)		//wake up
	{
		cmd = 0x3<<23;
		writeCmd();
		for(j=0;j<100;j++);
	}
	cmd = 0x2<<23;		//return home
	writeCmd();

	data = 0x28;		//inform that there are 2 lines, default font
	writeData();

	data = 0x01;		//clear display
	writeData();

	data = 0x06;		//increment cursor after writing, don't shift data
	writeData();

	data = 0x80;		//move to first line, first column
	writeData();

	data = 0x0C;		//switch on LCD, show cursor and blink
	writeData();
}

void clearPorts()
{
	LPC_GPIO0->FIOCLRH = 0x1F80;
}

void writeCmd()
{
	clearPorts();
	LPC_GPIO0->FIOPIN = cmd;
	if(flag==0)
	{
		LPC_GPIO0->FIOCLR = RS_CTRL;
	}
	else if(flag==1)
	{
		LPC_GPIO0->FIOSET = RS_CTRL;
	}
	LPC_GPIO0->FIOSET = EN_CTRL;
	for(j=0;j<50;j++);
	LPC_GPIO0->FIOCLR = EN_CTRL;
}

void writeData()
{
	cmd = (data & 0xF0)<<19;
	writeCmd();
	cmd = (data & 0x0F)<<23;
	writeCmd();
	for(j=0;j<10;j++);
}

void disp(char *to_disp)
{
	i = 0;
	while(to_disp[i]!='\0')
	{
		data = to_disp[i];
		flag=1;
		writeData();
		i++;
	}
}

void disp_with_delay(char *to_disp)
{
	startTimer0();
	i = 0;
	while(to_disp[i]!='\0')
	{
		data = to_disp[i];
		flag=1;
		writeData();
		i++;
		for(j=0;j<50000;j++);
	}
	stopTimer0();
	LPC_GPIO1->FIOPIN = 0x00<<23;
	LPC_GPIO0->FIOPIN = 0xF9<<4;
}

void move(int l, int c)
{
	flag=0;
	if(l==1)
		data = 0x80;
	else
		data = 0xC0;
	data = data+c;
	writeData();
	for(j=0;j<800;j++);
}

void clear_display()
{
	flag = 0;
	data = 0x01;
	writeData();
	//for(j=0;j<10000;j++);
}

int fputc(int ch, FILE *f) {
if (DEMCR & TRCENA) {
while (ITM_Port32(0) == 0);
ITM_Port8(0) = ch;
}
return(ch);
}
int triggerSignal;




void initTimer0(void) //PCLK must be = 25Mhz!
{
	LPC_TIM0->CTCR = 0x0;
	LPC_TIM0->PR = PRESCALE; //Increment TC at every 24999+1 clock cycles
	LPC_TIM0->TCR = 0x02; //Reset Timer
}

void startTimer0(void)
{
	LPC_TIM0->TCR = 0x02; //Reset Timer
	LPC_TIM0->TCR = 0x01; //Enable timer
}

unsigned int stopTimer0(void)
{
	LPC_TIM0->TCR = 0x00; //Disable timer
	return LPC_TIM0->TC;
}

void delayUS(unsigned int microseconds) //Using Timer0
{
	LPC_TIM0->TCR = 0x02; //Reset Timer
	LPC_TIM0->TCR = 0x01; //Enable timer
	while(LPC_TIM0->TC < microseconds); //wait until timer counter reaches the desired delay
	LPC_TIM0->TCR = 0x00; //Disable timer
}



void delayMS(unsigned int milliseconds) //Using Timer0
{
	delayUS(milliseconds * 1000);
}



void delay_trigger(){
	LPC_GPIO0->FIOSET = (0x1<<15);
	delayUS(10);
	LPC_GPIO0->FIOCLR = (0x1<<15);
}

float echo_monitor(){
	float pulse_time = 0,distance=0;
	while((LPC_GPIO0->FIOPIN & (0x1<<16)) == 0x0); //Wait till echo is low
	startTimer0(); //Initialize the echo timer
	while((LPC_GPIO0->FIOPIN & (0x1<<16)) == 0x1<<16); //Wait till echo is high
	pulse_time = stopTimer0(); //Get count of echo timer
	distance = (0.0343*pulse_time)/2;
	return distance;
}

int main(){
	float distance;
	char dist[]="WELCOME";
	char msg1[16]={"THIS IS OUR"};
	char msg3[16] = {"DISTANCE"};
	char msg4[16] = {"CALCULATOR"};
	char msg[16] = {"DIST: "};
	SystemInit(); //CLK = 12 MHz and PCLK = 25MHz
	SystemCoreClockUpdate();


	move(1, 3);
	disp_with_delay(&msg1[0]);
	clear_display();
	for(j1=0;j1<100000;j1++);
	move(1, 4);
	disp_with_delay(&msg3[0]);
	move(2, 3);
	disp_with_delay(&msg4[0]);
	for(j1=0;j1<10000;j1++);



	//Trigger, Echo
	LPC_PINCON->PINSEL0 = 0x0; //SET TO GPIO
	LPC_PINCON->PINSEL1 = 0x0; //SET TO GPIO (For the echo pin)
	LPC_GPIO0->FIODIR = 0x0FFF0;
	//Timer setup
	init();


	while(1){
	clear_display();
	move(1, 0);
	disp(&msg[0]);
	initTimer0();

	delay_trigger();
	distance = echo_monitor();

	printf("%f\n",distance);
	sprintf(dist, "%f", distance);
	disp(dist);
	delayMS(200);
	}
}
