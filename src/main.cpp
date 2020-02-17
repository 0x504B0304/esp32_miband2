/**
 * A BLE client example that is rich in capabilities.
 */

#include "BLEDevice.h"
#include "HardwareSerial.h"
#include <miband2.h>
#include <math.h>
BLEAddress *MiBandAddress=nullptr;
BLEScan* pBLEScan=nullptr;
MiBand2 MyMiBand;
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice)
   {
	// Serial.println(advertisedDevice.toString().c_str());
    if (advertisedDevice.getName()==std::string("MI Band 2") ) 
		{
		MiBandAddress = new BLEAddress(advertisedDevice.getAddress());
		MyMiBand.SetMac(*MiBandAddress);
		pBLEScan->stop();
		} // Found our server
  	} // onResult
}; // MyAdvertisedDeviceCallbacks

void setup() {
	BLEDevice::init("");
	Serial.begin(115200);
	// Retrieve a Scanner and set the callback we want to use to be informed when we
	// have detected a new device.  Specify that we want active scanning and start the
	// scan to run for 30 seconds.
	pBLEScan = BLEDevice::getScan();

	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(10);
	MyMiBand.Connect();
	// MyMiBand.SetGetButtonCount(true);
	MyMiBand.SetGetRealtimeAccel(true);
} // End of setup.


	// This is the Arduino main loop function.
void loop()
{
	// MyMiBand.UpdateActiveData();
	// log_i("Steps:%d;Distance:%d;Calories:%d;",MyMiBand.Steps,MyMiBand.Distance,MyMiBand.Calories);
	// log_i("ButtonCount:%d",MyMiBand.ButtonCount);
	//log_i("X:%.3f;Y:%.3f;Z:%.3f",MyMiBand.XAccel,MyMiBand.YAccel,MyMiBand.ZAccel);
	//Serial.printf("X:%.3f Y:%.3f Z:%.3f\n",MyMiBand.XAccel,MyMiBand.YAccel,MyMiBand.ZAccel);
	Serial.printf("total:%.3f\n",sqrt(MyMiBand.XAccel*MyMiBand.XAccel+MyMiBand.YAccel*MyMiBand.YAccel+MyMiBand.ZAccel*MyMiBand.ZAccel));
} // End of loop