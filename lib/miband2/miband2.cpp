#include "miband2.h"
#include <esp32-hal.h>
#include <esp_log.h>

const std::__cxx11::string UUIDPrefix = "0000";
const std::__cxx11::string UUIDSuffix1 = "-0000-1000-8000-00805f9b34fb";
const std::__cxx11::string UUIDSuffix2 = "-0000-3512-2118-0009af100700";
const std::__cxx11::string MiBand2::Service1UUID = UUIDPrefix + "fee0" + UUIDSuffix1,
                           MiBand2::Service2UUID = UUIDPrefix + "fee1" + UUIDSuffix1,
                           MiBand2::ImmediateAlertServiceUUID = UUIDPrefix + "1802" + UUIDSuffix1,
                           MiBand2::TextNotificationServiceUUID = UUIDPrefix + "1811" + UUIDSuffix1;

const std::__cxx11::string MiBand2::AuthCharUUID = UUIDPrefix + "0009" + UUIDSuffix2,
                           MiBand2::ButtonCharUUID = UUIDPrefix + "0010" + UUIDSuffix2,
                           MiBand2::TextNotificationCharUUID = UUIDPrefix + "2a46" + UUIDSuffix1,
                           MiBand2::StepsCharUUID = UUIDPrefix + "0007" + UUIDSuffix2,
						   MiBand2::SensorControlCharUUID = UUIDPrefix + "0001" + UUIDSuffix2,
						   MiBand2::SensorDataCharUUID = UUIDPrefix + "0002" + UUIDSuffix2;
mbedtls_aes_context MiBand2::Aes;
MiBand2::authentication_flags MiBand2::auth_flag=require_random_number;
BLEClient* MiBand2::pClient=nullptr;
uint8_t MiBand2::SendRandNumCmd[2]={0x02,0x00};
uint8_t MiBand2::EncryptedNum[18]={0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t MiBand2::_KEY[18]={0x01,0x00,0xf5,0xd2,0x29,0x87,0x65,0x0a,0x1d,0x82,0x05,0xab,0x82,0xbe,0xb9,0x38,0x59,0xcf};
uint8_t MiBand2::ButtonCount=0;
volatile float MiBand2::XAccel=0.0f,MiBand2::YAccel=0.0f,MiBand2::ZAccel=0.0f;
bool MiBand2::Connected=false;

MiBand2::MiBand2()
{
    MiBand2::pClient  = BLEDevice::createClient();
}
MiBand2::MiBand2(BLEAddress Mac)
{
    MiBand2::pClient  = BLEDevice::createClient();
    MiBand2::MacAddress=new BLEAddress(Mac);
}

MiBand2::MiBand2(BLEAddress Mac,const uint8_t AuthKey[18])
{
    MiBand2::pClient  = BLEDevice::createClient();
    MiBand2::MacAddress=new BLEAddress(Mac);
    memcpy(_KEY,AuthKey,sizeof(uint8_t)*18);
}
void MiBand2::SetMac(BLEAddress Mac)
{
    MiBand2::MacAddress=new BLEAddress(Mac);
}
void MiBand2::AuthNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData,size_t length,bool isNotify) 
{
    log_d("pData:0x%x,0x%x,0x%x",pData[0],pData[1],pData[2]);
	switch(pData[1])
		{
			case 0x01:
				{
					if (pData[2]==0x01)
							MiBand2::auth_flag= require_random_number;
					else
					MiBand2::auth_flag= auth_failed;
				}break;
			case 0x02:
				{
					if(pData[2]==0x01)
					{
						mbedtls_aes_init(&Aes);
						mbedtls_aes_setkey_enc( &Aes,(_KEY+2), 128 );//因为秘钥前加了前缀，所以加上前缀的偏移量
						mbedtls_aes_crypt_ecb(&Aes, MBEDTLS_AES_ENCRYPT,pData+3,EncryptedNum+2);
						mbedtls_aes_free(&Aes);
						MiBand2::auth_flag= send_encrypted_number;
					}
					else
						MiBand2::auth_flag= auth_failed;
				}break;
			case 0x03:
				{
					if(pData[2]==0x01)
						MiBand2::auth_flag= auth_success;
					else if (pData[2]==0x04)
						MiBand2::auth_flag= send_key;//因为密钥错了，所以重新发送秘钥覆盖手环中原来保存的秘钥
				}break;
				default:
					MiBand2::auth_flag= auth_failed;
    	}
}

bool MiBand2::Authenticate()
{
    if(!Connected)
        MiBand2::pClient->connect(*MacAddress,BLE_ADDR_TYPE_RANDOM);
	Service1=MiBand2::pClient->getService(Service1UUID);
    Service2=MiBand2::pClient->getService(Service2UUID);
    if (Service2 == nullptr)
        return false;
    AuthChar = Service2->getCharacteristic(AuthCharUUID);
    AuthChar->registerForNotify((notify_callback)AuthNotifyCallback);
	while (auth_flag != auth_success) 
	{
		authentication_flags seaved_flag=auth_flag;
		auth_flag=waiting;
		switch(seaved_flag)
		{
			case send_key:
				AuthChar->writeValue(_KEY,18); break;			//Sent prestored key
			case require_random_number:
				AuthChar->writeValue(SendRandNumCmd,2); break;	//Get the random number
			case send_encrypted_number:
				{
				AuthChar->writeValue(EncryptedNum,18);			//Sent the encrypted random number
				} break;
			default:
				;
		}
		if(auth_flag==seaved_flag)
			auth_flag=waiting;
		delay(100);
	} 
	AuthChar->registerForNotify(nullptr,false);
    return true;
}

bool MiBand2::Connect()
{
    if(MacAddress==nullptr||MacAddress==nullptr)
        return false;
    Connected=MiBand2::pClient->connect(*MacAddress,BLE_ADDR_TYPE_RANDOM);
    return Authenticate();
}

void MiBand2::SendTextNotification(std::string content)
{
    int length = content.length();
	uint8_t per_text[2+length]={0x05,0x01};
	memcpy(per_text+2,content.c_str(),length);
    if(TextNotifyChar==nullptr)
    {
        if(TextNotificationService==nullptr)
            TextNotificationService=MiBand2::pClient->getService(TextNotificationServiceUUID);
        TextNotifyChar = TextNotificationService->getCharacteristic(TextNotificationCharUUID);
    }
    if(TextNotifyChar!=nullptr)
        TextNotifyChar->writeValue(per_text,length+2,true);
}

void MiBand2::UpdateActiveData()
{
    // if(Service1==nullptr)
    //     Service1=pClient->getService(Service1UUID);
    BLERemoteCharacteristic * StepsChar=Service1->getCharacteristic(StepsCharUUID);
    if(StepsChar!=nullptr)
    {
        log_d("0x%x",StepsChar->canRead());
        std::string Value=StepsChar->readValue();
        if(Value.length()>0)
        {
            Steps=*(uint32_t*)Value.substr(1,4).c_str();
            Distance=*(uint32_t*)Value.substr(5,8).c_str();
            Calories=*(uint32_t*)Value.substr(9,12).c_str();
        }
    }
}

void MiBand2::ButtonNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData,size_t length,bool isNotify)
{
	MiBand2::ButtonCount++;
}

void MiBand2::SetGetButtonCount(bool enable)
{
	if(ButtonChar==nullptr)
		ButtonChar=Service1->getCharacteristic(ButtonCharUUID);
	log_d("ButtonChar:%x",ButtonChar);
	if(enable)
		ButtonChar->registerForNotify((notify_callback)ButtonNotifyCallback);
	else
		ButtonChar->registerForNotify(nullptr,false);
}

void MiBand2::AccelNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData,size_t length,bool isNotify)
{
	// log_i("\nlegth:%d",length);
	// for(uint8_t i=0;i<length;i++)
	// 	printf("0x%x ",pData[i]);
	// printf("\n");
	//uint32_t XTemp=0,YTemp=0,Ztemp=0;
	int16_t *temp=(int16_t*)pData;
	if(length==20)
	{
		XAccel=(temp[1]+temp[4]+temp[7])/(3);
		//XAccel=((pData[3]<<8)+pData[2]+(pData[9]<<8)+pData[8]+(pData[15]<<8)+pData[14])/3;
		YAccel=(temp[2]+temp[5]+temp[8])/(3);
		ZAccel=(temp[3]+temp[6]+temp[9])/(3);
	}
}

void MiBand2::SetGetRealtimeAccel(bool enable)
{
	if((SensorControlChar==nullptr)||(SensorDataChar==nullptr))
	{
		SensorControlChar=Service1->getCharacteristic(SensorControlCharUUID);
		SensorDataChar=Service1->getCharacteristic(SensorDataCharUUID);
	}
	if(enable)
	{
		SensorControlChar->writeValue((uint8_t*)"\x01\x01\x19",3);
		delay(100);
		SensorControlChar->writeValue('\x02');
		SensorDataChar->registerForNotify((notify_callback)AccelNotifyCallback);
	}
	else
	{
		SensorControlChar->writeValue('\x03');
		SensorDataChar->registerForNotify(nullptr,false);
	}
}



