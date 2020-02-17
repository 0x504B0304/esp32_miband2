#ifndef MI_BAND_2_H__
#define MI_BAND_2_H__

#include "mbedtls/aes.h"
#include "BLEDevice.h"

class MiBand2
{
public:
    MiBand2();
    MiBand2(BLEAddress Mac);
    MiBand2(BLEAddress Mac,const uint8_t AuthKey[18]);
    void SetMac(BLEAddress Mac);
    bool Connect();
    void SendTextNotification(std::string content);
    void UpdateActiveData();
    void SetGetButtonCount(bool enable);
    void SetGetRealtimeAccel(bool enable);
    uint32_t Steps=0;
    uint32_t Distance=0;
    uint32_t Calories=0;
    static uint8_t ButtonCount;
    volatile static float XAccel,YAccel,ZAccel;
private:
    bool Authenticate();
    //The callback function for authentication
    static void AuthNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData,size_t length,bool isNotify);
    static void ButtonNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData,size_t length,bool isNotify);
    static void AccelNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData,size_t length,bool isNotify);
    //Encryption context for authentication
    static mbedtls_aes_context Aes;
    static uint8_t EncryptedNum[18];
    static uint8_t SendRandNumCmd[2];
    static enum authentication_flags
    {
        send_key=0,
        require_random_number=1,
        send_encrypted_number=2,
        auth_failed,auth_success=3,
        waiting=4
    }auth_flag;
    //BLEClient of Miband2
    static BLEClient*  pClient;
    //mac address
    BLEAddress* MacAddress;
    //flag of if connected
    static bool Connected;
    //services uuid
    static const std::__cxx11::string Service1UUID,Service2UUID,
    ImmediateAlertServiceUUID,TextNotificationServiceUUID;
    //characteristic uuid
    static const std::__cxx11::string AuthCharUUID,ButtonCharUUID,
    TextNotificationCharUUID,StepsCharUUID,SensorControlCharUUID,
    SensorDataCharUUID;

    BLERemoteService *Service1=nullptr,*Service2=nullptr,
    *ImmediateAlertService=nullptr,*TextNotificationService=nullptr;
    BLERemoteCharacteristic *AuthChar=nullptr,*TextNotifyChar=nullptr,
    *ButtonChar=nullptr,*SensorControlChar=nullptr,*SensorDataChar=nullptr;
    //auth_key
    static uint8_t _KEY[18];
};                           

#endif