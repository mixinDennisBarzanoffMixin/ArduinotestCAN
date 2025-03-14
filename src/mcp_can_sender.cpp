#include <SPI.h>
#include <mcp_can.h>

const int spiCSPin = 10;
int ledHIGH    = 1;
int ledLOW     = 0;

MCP_CAN CAN(spiCSPin);

void setup()
{
    Serial.begin(9600);

    while (CAN_OK != CAN.begin(MCP_ANY, CAN_40KBPS, MCP_8MHZ))
    {
        Serial.println("CAN BUS init Failed");
        delay(100);
    }
    Serial.println("CAN BUS Shield Init OK!");
}

unsigned char stmp[8] = {ledHIGH, 1, 2, 3, ledLOW, 5, 6, 7};
    
void loop()
{   
  unsigned char helloMsg[5] = {'h', 'e', 'l', 'l', 'o'};
  CAN.sendMsgBuf(0x43, 0, 5, helloMsg);
  delay(1000);
}