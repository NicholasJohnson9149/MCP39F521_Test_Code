// /*
//  * Project I2C-MCP32F521
//  * Description: 
//  * Author: Nicholas 
//  * Date: July 16th 2020 
//  */
#include "application.h"
#include "Wire.h"
#include <math.h>
#include <string.h>
#include <stdint.h>

int tinkerDigitalWrite(String command);

constexpr size_t I2C_BUFFER_SIZE = 35;
  uint8_t numBytesToRead = 28;
  uint8_t ReadDataBuf[8];
  uint64_t somedata = 0;
  uint32_t checksumTotal = 0;
int i;

typedef struct MCP39F521_Data {
	uint16_t systemStatus;
	uint16_t systemVersion;
	uint16_t voltageRMS;
	uint16_t lineFrequency;
	uint16_t analogInputVoltage;
	int16_t powerFactor;
	uint32_t currentRMS;
	uint32_t activePower;
	uint32_t reactivePower;
	uint32_t apparentPower;
} MCP39F521_Data;

typedef struct MCP39F521_FormattedData {
	uint16_t systemStatus;
	uint16_t systemVersion;
	float voltageRMS;
	float lineFrequency;
	float analogInputVoltage;
	float powerFactor;
	float currentRMS;
	float activePower;
	float reactivePower;
	float apparentPower;
} MCP39F521_FormattedData;

void wireErrors(uint8_t i2c_bus_Status){
  if(i2c_bus_Status == 0){
    Serial.print("I2C bus Status Success::"); Serial.println(i2c_bus_Status);
  }else if(i2c_bus_Status == 1){
    Serial.print("Busy timeout upon entering endTransmission()::"); Serial.println(i2c_bus_Status);
  }else if(i2c_bus_Status == 2){
    Serial.print("Start bit generation timeout::"); Serial.println(i2c_bus_Status);
  }else if(i2c_bus_Status == 3){
    Serial.print("end of address transmission timeout::"); Serial.println(i2c_bus_Status);
  }else if(i2c_bus_Status == 4){
    Serial.print("Data byte transfer timeout::"); Serial.println(i2c_bus_Status);
  }else if(i2c_bus_Status == 5){
    Serial.print("Data byte transfer succeeded, busy timeout immediately after::"); Serial.println(i2c_bus_Status);
  }
}

int registerReadNBytes(int addressHigh, int addressLow, int numBytesToRead, uint8_t *byteArray, int byteArraySize)
{
  #define I2C_ADDRESS 0x74
  uint8_t i2c_bus_Status = 0;
  uint8_t ReadDataBuf[8];
  int i;
  uint32_t checksumTotal = 0;
  if (byteArraySize < numBytesToRead + 3) {
    return 3;
  }
  ReadDataBuf[0] = 0xA5; // Header
  ReadDataBuf[1] = 0x08; // Num bytes
  ReadDataBuf[2] = 0x41; // Command - set address pointer
  ReadDataBuf[3] = addressHigh;
  ReadDataBuf[4] = addressLow;
  ReadDataBuf[5] = 0x4E; // Command - read register, N bytes
  ReadDataBuf[6] = 0x20; 
  ReadDataBuf[7] = 0; // Checksum 0x05E - computed below
  for(i = 0; i < 7; i++) {
    checksumTotal += ReadDataBuf[i];
  }
  ReadDataBuf[7] = checksumTotal % 256; // 0x5E = 94 
  Serial.print("Checksum = "); Serial.println(ReadDataBuf[7], HEX);
  Wire.beginTransmission(I2C_ADDRESS);
  for(i= 0; i < 8; i++) {
    Wire.write(ReadDataBuf[i]);
  }
  i2c_bus_Status = Wire.endTransmission(true);
  wireErrors(i2c_bus_Status);
  delay(50);
  Wire.requestFrom(I2C_ADDRESS, 31);
  int requestDataLength = Wire.available();
  if (requestDataLength==(numBytesToRead + 3)) {
      for (i = 0; i <= numBytesToRead + 3 ; i++) {
        byteArray[i] = Wire.read();
        Serial.print(byteArray[i], HEX); Serial.print(" ");
      }
    Serial.print("\n");
  } else {
    return 5; 
  }
  return 0;
}

void convertdata(MCP39F521_Data *data, MCP39F521_FormattedData *fData)
{
  fData->voltageRMS = data->voltageRMS/10.0f;
  fData->currentRMS = data->currentRMS/10000.0f;
  fData->lineFrequency = data->lineFrequency/1000.0f;
  // Analog Input Voltage represents ADC data for 10 bit ADC
  // The register value/1023 * 3.3v will give the analog input voltage in volts.
  // analogInputVoltage = RegData/1023.0 * 3.3;
  fData->analogInputVoltage = data->analogInputVoltage/1023.0f*3.3;
  float f;
  unsigned char ch;
  f = ((data->powerFactor & 0x8000)>>15) * -1.0;
  for(ch=14; ch > 3; ch--)
    f += ((data->powerFactor & (1 << ch)) >> ch) * 1.0 / (1 << (15 - ch));
  fData->powerFactor = f;
  fData->activePower = data->activePower/100.0f;
  fData->reactivePower = data->reactivePower/100.0f;
  fData->apparentPower = data->apparentPower/100.0f;
}

void printMCP39F521Data(MCP39F521_FormattedData *data)
{
  Serial.print(F("Voltage = ")); Serial.println(data->voltageRMS, 4);
  Serial.print(F("Current = ")); Serial.println(data->currentRMS, 4);
  Serial.print(F("Line Frequency = ")); Serial.println(data->lineFrequency, 4);
  Serial.print("Analog Input Voltage = "); Serial.println(data->analogInputVoltage, 4);
  Serial.print(F("Power Factor = ")); Serial.println(data->powerFactor, 4);
  Serial.print(F("Active Power = ")); Serial.println(data->activePower, 4);
  Serial.print(F("Reactive Power = ")); Serial.println(data->reactivePower, 4);
  Serial.print(F("Apparent Power = ")); Serial.println(data->apparentPower, 4);
}

/*******************************************************************************
 * Function Name  : tinkerDigitalWrite
 * Description    : Sets the specified pin HIGH or LOW
 * Input          : Pin and value
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerDigitalWrite(String command)
{
    bool value = 0;
    //convert ASCII to integer
    int pinNumber = command.charAt(1) - '0';
    //Sanity check to see if the pin numbers are within limits
    if (pinNumber < 0 || pinNumber > 7) return -1;

    if(command.substring(3,7) == "HIGH") value = 1;
    else if(command.substring(3,6) == "LOW") value = 0;
    else return -2;

    if(command.startsWith("D"))
    {
        pinMode(pinNumber, OUTPUT);
        digitalWrite(pinNumber, value);
        return 1;
    }
    else if(command.startsWith("A"))
    {
        pinMode(pinNumber+10, OUTPUT);
        digitalWrite(pinNumber+10, value);
        return 1;
    }
#if Wiring_Cellular
    else if(command.startsWith("B"))
    {
        if (pinNumber > 5) return -4;
        pinMode(pinNumber+24, OUTPUT);
        digitalWrite(pinNumber+24, value);
        return 1;
    }
    else if(command.startsWith("C"))
    {
        if (pinNumber > 5) return -5;
        pinMode(pinNumber+30, OUTPUT);
        digitalWrite(pinNumber+30, value);
        return 1;
    }
#endif
    else return -3;
}

void setup() {
    //WiFi.off();
    Serial.begin(115200);
    Particle.function("digitalwrite", tinkerDigitalWrite);
    //Wire.setSpeed(CLOCK_SPEED_400KHZ);
    //Wire.stretchClock(true);
    Wire.begin();
}

void loop() 
{ 
  MCP39F521_Data data;
  MCP39F521_FormattedData fData;
  int retval = 0;
  uint8_t byteArray[35];

  retval = registerReadNBytes(0x00, 0x02, 28, byteArray, 35);

  if (retval != 0) {
      Serial.print("retval = "); Serial.println(retval);
  } else {
  /* System status */
  data.systemStatus = ((byteArray[3] << 8) | byteArray[2]);
  data.systemVersion = ((byteArray[5] << 8) | byteArray[4]);
  data.voltageRMS = ((byteArray[7] << 8) | byteArray[6]);
  data.lineFrequency = ((byteArray[9] << 8) | byteArray[8]);
  data.analogInputVoltage = ((byteArray[11] << 8) | byteArray[10]);
  data.powerFactor = (((signed char)byteArray[13] << 8) +
                          (unsigned char)byteArray[12]);
  data.currentRMS =      ((uint32_t)(byteArray[17]) << 24 |
                            (uint32_t)(byteArray[16]) << 16 |
                            (uint32_t)(byteArray[15]) << 8 |
                            byteArray[14]);
  data.activePower =     ((uint32_t)(byteArray[21]) << 24 |
                            (uint32_t)(byteArray[20]) << 16 |
                            (uint32_t)(byteArray[19]) << 8 |
                            byteArray[18]);
  data.reactivePower =   ((uint32_t)(byteArray[25]) << 24 |
                            (uint32_t)(byteArray[24]) << 16 |
                            (uint32_t)(byteArray[23]) << 8 |
                            byteArray[22]);
  data.apparentPower =   ((uint32_t)(byteArray[29]) << 24 |
                            (uint32_t)(byteArray[28]) << 16 |
                            (uint32_t)(byteArray[27]) << 8 |
                            byteArray[26]);
  convertdata(&data, &fData);
  printMCP39F521Data(&fData);
  }
  delay(1000);
}