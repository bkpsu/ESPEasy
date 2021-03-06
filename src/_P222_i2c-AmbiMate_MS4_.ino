#ifdef USES_P222
//#######################################################################################################
//#################### Plugin 222 TE Connectivity AmbiMate MS4 multi sensor (T/H/L/M/CO2)  ##############
//#######################################################################################################
//
// AmbiMate multi-sensor
// like this one: https://www.te.com/global-en/product-2316851-1.html
//


#define PLUGIN_222
#define PLUGIN_ID_222        222
#define PLUGIN_NAME_222       "Environment - TE AmbiMate MS4 [TESTING]"
#define PLUGIN_VALUENAME1_222 "Temperature"
#define PLUGIN_VALUENAME2_222 "Humidity"
#define PLUGIN_VALUENAME3_222 "Light"
#define PLUGIN_VALUENAME4_222 "Motion"
#define PLUGIN_VALUENAME5_222 "Power"
#define PLUGIN_VALUENAME6_222 "Audio"
#define PLUGIN_VALUENAME7_222 "CO2"
#define PLUGIN_VALUENAME8_222 "VOC"

//Default I2C Address of the sensor
#define AMBIMATESENSOR_DEFAULT_ADDR        0x2A

//Multi Sensor Register Addresses
#define AMBIMATESENSOR_GET_STATUS          0x00 // (r/w) 1 byte
#define AMBIMATESENSOR_GET_TEMPERATURE 	   0x01 // (r) 	2 bytes
#define AMBIMATESENSOR_GET_HUMIDITY        0x03 // (r) 	2 bytes
#define AMBIMATESENSOR_GET_LIGHT 		       0x05 // (r) 	2 bytes
#define AMBIMATESENSOR_GET_AUDIO  	       0x07 // (r) 	2 bytes
#define AMBIMATESENSOR_GET_POWER 		       0x09 // (r) 	2 bytes
#define AMBIMATESENSOR_GET_CO2    	       0x0B // (r) 	2 bytes
#define AMBIMATESENSOR_GET_VOC 			       0x0D // (r) 	2 bytes
#define AMBIMATESENSOR_GET_VERSION 		     0x80 // (r) 	1 bytes
#define AMBIMATESENSOR_GET_SUBVERSION      0x81 // (r)  1 bytes
#define AMBIMATESENSOR_GET_OPT_SENSORS     0x82 // (r)	1 bytes
#define AMBIMATESENSOR_SET_SCAN_START_BYTE 0xC0 // (w)  1 bytes
#define AMBIMATESENSOR_SET_AUDIO_EVENT_LVL 0xC1 // (r/w) 1 bytes
#define AMBIMATESENSOR_SET_RESET_BYTE      0xF0 // (w) 1 byte (0xA5 initiates processor reset, all others ignored)

//Options to include/exclude sensors during read
#define AMBIMATESENSOR_READ_PIR_ONLY       0x01 //
#define AMBIMATESENSOR_READ_EXCLUDE_GAS    0x3F //
#define AMBIMATESENSOR_READ_INCLUDE_GAS    0x7F //
#define AMBIMATESENSOR_READ_SENSORS        0x00 //

uint8_t _i2caddrP222;  //Sensor I2C Address
uint8_t opt_sensors;   //Optional Sensors byte
bool good;
uint8_t motion;
bool motionRead;
unsigned long stateTimer;
unsigned long commTimer;
uint8_t sensorState;

enum MS4_state {
  MS4_Start = 0,
  MS4_Waiting,
  MS4_Poll_PIR,
  MS4_Read_PIR,
  MS4_Poll_All,
  MS4_Read_All
};

boolean Plugin_222(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_222;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_222);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {

        for (int i=0; i<4; i++){
          switch(Settings.TaskDevicePluginConfig[event->TaskIndex][i]) //process first sensor ValueCount
          { case 0: {
              strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[i],PSTR(PLUGIN_VALUENAME1_222));
              break;}
            case 1: {
              strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[i],PSTR(PLUGIN_VALUENAME2_222));
              break;}
            case 2: {
              strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[i],PSTR(PLUGIN_VALUENAME3_222));
              break;}
            case 3: {
              strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[i],PSTR(PLUGIN_VALUENAME4_222));
              break;}
            case 4: {
              strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[i],PSTR(PLUGIN_VALUENAME5_222));
              break;}
            case 5: {
              strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[i],PSTR(PLUGIN_VALUENAME6_222));
              break;}
            case 6: {
              strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[i],PSTR(PLUGIN_VALUENAME7_222));
              break;}
            case 7: {
              strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[i],PSTR(PLUGIN_VALUENAME8_222));
              break;}
          }
        }

        break;
      }

    case PLUGIN_INIT:
      {
        // Data and basic information are acquired from the module

        _i2caddrP222 = AMBIMATESENSOR_DEFAULT_ADDR;

        // Read Firmware Version
        unsigned char fw_ver = I2C_read8_reg(_i2caddrP222, AMBIMATESENSOR_GET_VERSION, &good);
        // Read Firmware Subversion
        unsigned char fw_sub_ver = I2C_read8_reg(_i2caddrP222, AMBIMATESENSOR_GET_SUBVERSION, &good);

        String log = F("AmbiMate: Firmware Version: ");
        log += String(fw_ver);
        log += F(" Subversion: ");
        log += String(fw_sub_ver);
        addLog(LOG_LEVEL_INFO, log);

        // Read Optional Sensors byte
        opt_sensors = I2C_read8_reg(_i2caddrP222, AMBIMATESENSOR_GET_OPT_SENSORS, &good);

        sensorState = MS4_Start;

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        byte sensor_1_value = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        byte sensor_2_value = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        byte sensor_3_value = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
        byte sensor_4_value = Settings.TaskDevicePluginConfig[event->TaskIndex][3];

        addFormSeparator(2);

        addFormCheckBox(F("Use Celsius"), F("p222_Celsius"), PCONFIG(0));

        addFormSeparator(2);

          String options_query[8] = { F("Temperature"),
                                       F("Humidity"),
                                       F("Light"),
                                       F("Motion"),
                                       F("Power"),
                                       F("Audio"),
                                       F("CO2"),
                                       F("VOC")  };
          addFormSelector(F("Sensor 1 Value"), F("p222_sensor1value"), 8, options_query, NULL, sensor_1_value );
          addFormSelector(F("Sensor 2 Value"), F("p222_sensor2value"), 8, options_query, NULL, sensor_2_value );
          addFormSelector(F("Sensor 3 Value"), F("p222_sensor3value"), 8, options_query, NULL, sensor_3_value );
          addFormSelector(F("Sensor 4 Value"), F("p222_sensor4value"), 8, options_query, NULL, sensor_4_value );

        addFormSeparator(2);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {

        PCONFIG(0) = isFormItemChecked(F("p222_Celsius"));

        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("p222_sensor1value"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("p222_sensor2value"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F("p222_sensor3value"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = getFormItemInt(F("p222_sensor4value"));

        success = true;
        break;
      }

    case PLUGIN_TEN_PER_SECOND: //For motion event processing
      {        
        unsigned char buf[20];

        switch(sensorState)
        {
          case MS4_Start: //Start
          {
            stateTimer = millis(); // Reset stateTimer to current time
            sensorState = MS4_Waiting; // Go to WAITING
              String log = F("Hit Start");
              addLog(LOG_LEVEL_INFO, log);  
            break;
          }
          case MS4_Waiting: // Waiting
          {
                String log = F("Entered Waiting with stateTimer at");
                log+=stateTimer;
                log+=" and millis() at ";
                log+=millis();
                                addLog(LOG_LEVEL_INFO, log);
            if(millis() > (stateTimer + 500))// PIR timer elapsed
            { //Send PIR request to sensor and start comm timer
              good = I2C_write8_reg(_i2caddrP222, AMBIMATESENSOR_SET_SCAN_START_BYTE, AMBIMATESENSOR_READ_PIR_ONLY);
              commTimer = millis(); 
                String log = F("Hit Waiting if statement with stateTimer at");
                log+=stateTimer;
                log+=" and millis() at ";
                log+=millis();
                addLog(LOG_LEVEL_INFO, log); 
              sensorState = MS4_Poll_PIR; // Go to POLL PIR
            }
            //sendData(event);
            break;
          }
          case MS4_Poll_PIR: // Poll PIR
          {
            if(millis() > (commTimer + 100)) // Commtimer elapsed
            {
                // String log = F("Hit PIR Poll at ");
                // log+=millis();
                // addLog(LOG_LEVEL_INFO, log);               
              sensorState = MS4_Read_PIR; // Go to READ PIR
            }
            break;
          }
          case MS4_Read_PIR: // Read PIR
          {
            uint8_t statusByte = I2C_read8_reg(_i2caddrP222, AMBIMATESENSOR_GET_STATUS, &good);
            if (statusByte & 0x80){ // PIR Event occurred
              motion = 1;
            }
            else
            {
              motion = 0;
            }
            for (int i=0; i<4; i++){
            if(Settings.TaskDevicePluginConfig[event->TaskIndex][i] == 3) //Motion sensor selected
              {
                UserVar[event->BaseVarIndex + i] = (int)motion;
              }
            }
                // String log = F("Hit PIR Read at ");
                // log+=millis();
                // addLog(LOG_LEVEL_INFO, log);                 
            schedule_task_device_timer(event->TaskIndex, millis() + 10); // Send to PLUGIN_READ
            break;
          }
          case MS4_Poll_All: // Polling All
          {
            if(millis() > (commTimer + 100)) // Commtimer elapsed
            {
                String log = F("Hit Poll All");
                addLog(LOG_LEVEL_INFO, log);               
              sensorState = MS4_Read_All; // Go to READ ALL
            }
            break;
          }
          case MS4_Read_All: // Read All
          {
                String log = F("Hit Read All");
                addLog(LOG_LEVEL_INFO, log);               
            // Read Sensors next byte
            good = I2C_write8(_i2caddrP222, AMBIMATESENSOR_READ_SENSORS);

            Wire.requestFrom(0x2A, 15);   // request bytes from slave device

            // Acquire the Raw Data
            unsigned int i = 0;
            while (Wire.available()) { // slave may send less than requested
              buf[i] = Wire.read(); // receive a byte as character
              i++;
            }

            // convert the raw data to engineering units
            //unsigned int status = buf[0];
            float temperatureC = (buf[1] * 256.0 + buf[2]) / 10.0;
            float temperatureF = ((temperatureC * 9.0) / 5.0) + 32.0;
            float Humidity = (buf[3] * 256.0 + buf[4]) / 10.0;
            float light = (buf[5] * 256.0 + buf[6]);
            float audio = (buf[7] * 256.0 + buf[8]);
            float batVolts = ((buf[9] * 256.0 + buf[10]) / 1024.0) * (3.3 / 0.330);
            float co2_ppm = (buf[11] * 256.0 + buf[12]);
            float voc_ppm = (buf[13] * 256.0 + buf[14]);

            for(int i=0; i<4; i++)
            {
              switch(Settings.TaskDevicePluginConfig[event->TaskIndex][i]) //process first sensor ValueCount
              {
                case 0: 
                { 
                  if(PCONFIG(0)){
                    UserVar[event->BaseVarIndex + i] = (buf[1] * 256.0 + buf[2]) / 10.0; //(float)temperatureC;
                  }
                  else{
                    UserVar[event->BaseVarIndex + i] = (float)temperatureF;
                  }
                  break;
                }
                case 1:
                {
                  UserVar[event->BaseVarIndex + i] = (buf[3] * 256.0 + buf[4]) / 10.0; //(float)Humidity;
                  break;
                }
                case 2:
                {
                  UserVar[event->BaseVarIndex + i] = (buf[5] * 256.0 + buf[6]); //(float)light;
                  break;
                }
                case 3:
                {
                //   UserVar[event->BaseVarIndex + i] = (float)motion;
                //     String log = F("AmbiMate: Motion in case statement: ");
                //     log += motion;
                //     addLog(LOG_LEVEL_INFO, log);              
                //   motion = 0;
                  break;
                }
                case 4:
                {
                  UserVar[event->BaseVarIndex + i] = (float)batVolts;
                  break;
                }
                case 5:
                {
                  UserVar[event->BaseVarIndex + i] = (float)audio;
                  break;
                }
                case 6:
                {
                  UserVar[event->BaseVarIndex + i] = (float)co2_ppm;
                  break;
                }
                case 7:
                {
                  UserVar[event->BaseVarIndex + i] = (float)voc_ppm;
                  break;
                }
              }
            }

            schedule_task_device_timer(event->TaskIndex, millis() + 10); // Send to PLUGIN_READ
            break;
          }
        }

        success = false;
        break;
      }

    case PLUGIN_READ:
      {
        success = false;
                String log = F("Entered PLUGIN_READ at ");
                log+=millis();
                addLog(LOG_LEVEL_INFO, log);   

        if(sensorState == MS4_Waiting) // If current state is WAITING, send command for all sensors
        {

          if (opt_sensors & 0x01) //Gas Sensor installed
          {
            // Read All Sensors next byte
            good = I2C_write8_reg(_i2caddrP222, AMBIMATESENSOR_SET_SCAN_START_BYTE, AMBIMATESENSOR_READ_INCLUDE_GAS);
          }
          else
          {
            good = I2C_write8_reg(_i2caddrP222, AMBIMATESENSOR_SET_SCAN_START_BYTE, AMBIMATESENSOR_READ_EXCLUDE_GAS);
          }
          commTimer = millis();
                String log = F("In PLUGIN_READ - WAITING");
                addLog(LOG_LEVEL_INFO, log);            
          sensorState = MS4_Poll_All;
          success = false;
        }
        else if ((sensorState == MS4_Read_PIR) || (sensorState == MS4_Read_All))
        {
          success = true;
                String log = F("In PLUGIN_READ - READ at ");
                log+=millis();
                addLog(LOG_LEVEL_INFO, log);             
          sensorState = MS4_Waiting;
          stateTimer = millis();           
        }    

        break;
      }
  }
  return success;
}


//**************************************************************************/
//Read temperature
//**************************************************************************/
float Plugin_222_readTemperature(){
  return I2C_read16_reg(_i2caddrP222, AMBIMATESENSOR_GET_TEMPERATURE);
}

// **************************************************************************/
// Read humidity
//**************************************************************************/
float Plugin_222_readHumidity() {
  return I2C_read16_reg(_i2caddrP222, AMBIMATESENSOR_GET_HUMIDITY);
}

// **************************************************************************/
// Read light
// **************************************************************************/
float Plugin_222_readLight() {
  return I2C_read16_reg(_i2caddrP222, AMBIMATESENSOR_GET_LIGHT);
}

// **************************************************************************/
// Read audio
// **************************************************************************/
float Plugin_222_readAudio() {
  return I2C_read16_reg(_i2caddrP222, AMBIMATESENSOR_GET_AUDIO);
}

// **************************************************************************/
// Read power
// **************************************************************************/
float Plugin_222_readPower() {
  return I2C_read16_reg(_i2caddrP222, AMBIMATESENSOR_GET_POWER);
}

// **************************************************************************/
// Read Gas
// **************************************************************************/
float Plugin_222_readGas() {
  return I2C_read16_reg(_i2caddrP222, AMBIMATESENSOR_GET_CO2);
}

// **************************************************************************/
// Read Voc
// **************************************************************************/
float Plugin_222_readVoc() {
  return I2C_read16_reg(_i2caddrP222, AMBIMATESENSOR_GET_VOC);
}

#endif // USES_P222
