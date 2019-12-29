/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/

/*
	Modified BLE UART for V7RC   
	
	iOS					https://play.google.com/store/apps/details?id=com.v7idea.v7rcliteandroidsdkversion&hl=zh_TW
	Android				https://apps.apple.com/tw/app/v7rc/id1390983964		
	
	
	這是下面論區的討論結果，歡迎引用.... 
	RoboTW 機器人討論區   https://www.facebook.com/groups/540271770146161/
	allen54a0@gmaol.com
	
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"



void parseCommand();
int flagShowControl =0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


int hexConvert2int(char highByte, char lowByte){

    int val=0;
    int highB;
    int lowB;
    

   if(highByte>='A' ){
        highB = highByte- 'A' + 10; 
   }else if(highByte>='0' &&highByte<='9' ){
        highB = highByte-0x30; 
   }

    if(lowByte>='A' ){
        lowB = lowByte- 'A' + 10; 
   }else if(lowByte>='0' &&lowByte<='9' ){
        lowB = lowByte-0x30; 
   }
 
    val = highB *16 + lowB; 
    val = val *10; 
    return val;
}


///// V7RC Code
int datafromV7RC[8]={1500,1500,1500,1500,1500,1500,1500,1500};


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      String rxData;
      String data;

      if (rxValue.length() > 0) {
      //  Serial.println("*********");
     //   Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++){
        //   Serial.print(rxValue[i]);
          rxData+=rxValue[i];
        }

        //   Serial.println();
       // Serial.println("*********");
      }

    /// do Decode Here !!
      //for SRT/SRV  CMD  (4 Servo)
 
 
 
 ///// V7RC Code ---------------------------------------------------------------->>>
        if(rxValue[1]=='R'){
          
        for(int i=0;i<4;i++){
              data = rxData.substring(i*4+3, i*4+7); 
              datafromV7RC[i] = data.toInt();
           }

    
          
        }else{   //for SS8   CMD  (8 Servo)   //SS8 96 96 96 96 96 96 96 96#

           for(int i=0;i<8;i++){
                  
                 datafromV7RC[i] = hexConvert2int(rxValue[i*2+3],rxValue[i*2+4] ) ; 
           }
        }
        

      ////debug Only, send to Vrep.... 

       if(flagShowControl==1){
          Serial.print(rxValue[2]);  /// should be V / T / 8 (2 ch, 4 ch , 8 ch )
          Serial.print(",");
         
          for(int i=0;i<8;i++){
            Serial.print(datafromV7RC[i]);
            Serial.print(",");
            }

            Serial.println(",");
       }  
      
    }

 ///// V7RC Code ----------------------------------------------------------------<<<<<
    
        
        

    
};


void setup() {
  Serial.begin(230400);

  // Create the BLE Device
  ///BLEDevice::init("UART Service");
  BLEDevice::init("BearQ ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
										CHARACTERISTIC_UUID_TX,
										BLECharacteristic::PROPERTY_NOTIFY
									);
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX,
											BLECharacteristic::PROPERTY_WRITE
										);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  //Serial.println("Characteristic defined! Now you can read it in your phone!");
   Serial.println("BearQ ESP32 wait for Connection~");
  
}

void loop() {

     if (Serial.available())
        parseCommand();


    if (deviceConnected) {
        pTxCharacteristic->setValue(&txValue, 1);
        pTxCharacteristic->notify();
        txValue++;
		delay(10); // bluetooth stack will go into congestion, if too many packets are sent
	}

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
      //  Serial.println("start advertising");
          Serial.println("BearQ ESP32 wait for Connection~");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
		// do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}



void parseCommand() {
  char cmd = Serial.read();
  switch (cmd) {
    case 'D':
      
     dumpespLoraData();
      break;

      case '1':
    flagShowControl =1;
      break;

     case '0':
     flagShowControl =0;
      break;
      
  }
}

 

void dumpespLoraData(){

   for(int i=0;i<8;i++){

      Serial.print(map(datafromV7RC[i],1000,2000,-255,255));
      Serial.print(",");
      

   }
   for(int i=0;i<6;i++){

       Serial.print(map(datafromV7RC[i],1000,2000,-255,255));
      Serial.print(",");
      

   }
     Serial.println("");


}
