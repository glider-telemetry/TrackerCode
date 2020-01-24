#ifndef _Sanderbuilt_SARA_h
#define _Sanderbuilt_SARA_h

#include "includes/Sanderbuilt_SARA_Config.h"
#include "includes/Sanderbuilt_SARA_ExtIncludes.h"
#include "includes/platform/Sanderbuilt_SARA_Platform.h"

#define min _min
#define max _max

#define SANDERBUILT_SARA_DEFAULT_TIMEOUT_MS 500
#define SANDERBUILT_SARA_NO_RST_PIN 99

#define SANDERBUILT_SARA_NO_STRING 0
#define SANDERBUILT_SARA_OK 1
#define SANDERBUILT_SARA_ERROR 2
#define SANDERBUILT_SARA_DOWNLOAD 3
#define SANDERBUILT_SARA_REPLY_COUNTER_TIMEOUT 96
#define SANDERBUILT_SARA_REPLY_IN_PROGRESS 97
#define SANDERBUILT_SARA_STRING_OVERRUN 98
#define SANDERBUILT_SARA_REPLY_STRING 99

#define SANDERBUILT_SARA_RADIO_GSM 0
#define SANDERBUILT_SARA_RADIO_GSM_UMTS 1
#define SANDERBUILT_SARA_RADIO_UMTS 2
#define SANDERBUILT_SARA_RADIO_LTE 3
#define SANDERBUILT_SARA_RADIO_GSM_UMTS_LTE 4
#define SANDERBUILT_SARA_RADIO_GSM_LTE 5
#define SANDERBUILT_SARA_RADIO_UMTS_LTE 6
#define SANDERBUILT_SARA_RADIO_LTE_CAT_M1 7
#define SANDERBUILT_SARA_RADIO_LTE_CAT_NB1 8
#define SANDERBUILT_SARA_RADIO_GPRS_EGPRS 9

#define SANDERBUILT_SARA_SIM_READY 0
#define SANDERBUILT_SARA_SIM_NEEDS_PIN 1

class Sanderbuilt_SARA : public Sanderbuilt_SARA_StreamType {
public:
	Sanderbuilt_SARA(int8_t);

	boolean beginSerial(Sanderbuilt_SARA_StreamType &port);

	int available(void);
	size_t write(uint8_t x);
	int read(void);
	int peek(void);
	void flush();

	uint8_t readloop(void);
	uint16_t parseError();

	bool requestOK(void);
	bool requestECHO(void);

	/*
	SANDERBUILT_SARA_RADIO_GSM 0
	SANDERBUILT_SARA_RADIO_GSM_UMTS 1
	SANDERBUILT_SARA_RADIO_UMTS 2
	SANDERBUILT_SARA_RADIO_LTE 3
	SANDERBUILT_SARA_RADIO_GSM_UMTS_LTE 4
	SANDERBUILT_SARA_RADIO_GSM_LTE 5
	SANDERBUILT_SARA_RADIO_UMTS_LTE 6
	SANDERBUILT_SARA_RADIO_LTE_CAT_M1 7
	SANDERBUILT_SARA_RADIO_LTE_CAT_NB1 8
	SANDERBUILT_SARA_RADIO_GPRS_EGPRS 9
	*/
	bool setRadio(uint8_t selectedAct);
	void setCREG(uint8_t mode);
	void setUCGED(uint8_t mode);

	void requestDeviceVersion(void);
	void requestFirmwareVersion(void);

	bool requestSimStatus(void);
	int8_t parseSimStatus(char *buffer);
	void requestNetworkStatus(void);
	void requestNetworkQuality(void);
	void requestExtendedNetworkQuality(void);
	void requestRSRPvalues(void);
	void requestRSRQvalues(void);
	void requestUCGEDvalues(void);
	void requestUPSD(uint8_t profile_id);
	void requestUPSD(uint8_t profile_id, uint8_t param_tag);
	void requestUPSD(uint8_t profile_id, uint8_t param_tag, char *param_value);
	void requestReadCOPS(void);
	void requestCGDCONT(void);
	void setMNOProfile(uint8_t mode);
	void requestMNOProfile(void); 
	void disableModem(void);
	void silentReset(void);
	void requestGPRSStatus(void);
	void requestSetGPRS(void);
	void requestShutdown(void);
	void requestCONTRDP(uint8_t profile_id);
	void requestTemperature(void);
	void requestFrequencyMap(void);
	void setFrequencyMap(uint8_t RAT, char* bitmask1);
	void requestPhoneNumber(void);

	void parseReplyBuffer(char *serverResponse);

	uint8_t getIMEI(char *imei);
	void requestSetCMEE(uint8_t parameter);

	// SOCKET
	void createSocket(uint8_t protocol);
	void setSocketOption(uint8_t socket, char* options);
	void closeSocket(uint8_t socket);
	void connectSocket(uint8_t socket, char* remoteAddress, uint16_t port);
	void writeSocketData(uint8_t socket, uint8_t length, char* data); 
	void writeSocketDataBytes(uint8_t socket, unsigned long length);
	void readSocketData(uint8_t socket, unsigned long length);
	void socketControl(uint8_t socket, uint8_t paramID);
	void getSocketError(void);
	void sendCustomCommand(char* text);
	void requestIP(char* url);

	// HTTP
	void setHTTPTimeout(uint8_t time);
	bool setIP(uint8_t profile, char* url);
	bool setURL(uint8_t profile, char *url);
	bool setSSL(uint8_t profile, uint8_t secure);
	bool requestHTTPGET(uint8_t profile, char *path, char *filename);
	bool requestHTTPPOST(uint8_t profile, char *path, char *filename, char *postFilename, uint8_t contentType);
	void requestHTTPError(uint8_t profile_id);

	// FTP

	void setFTPIP(char* ip);
	void setFTPURL(char* url);
	void setFTPUser(char* user);
	void setFTPPassword(char* password);
	void setFTPAccount(char* account);
	void setFTPMode(uint8_t mode);
	void setFTPPort(uint16_t port);

	void ftpLogout(void);
	void ftpLogin(void);
	void ftpDeleteFile(char* fileName);
	void ftpRetrieveFile(char* fileName, char* localFileName);
	void ftpRetrieveFOTA(char* fileName);
	void ftpRetrieveFileList(void);
	void ftpRetrieveFileList(char* folder);

	// FOTA

	void FOTAInstall(void);
	void FOTAInstallCheck(void);

	// File System
	void requestPushFile(char *filename, uint16_t size);
	void requestPushFile(char *filename, uint16_t size, char *tag);
	void requestPushPartialFile(char *filename, uint16_t offset, uint16_t size);
	void requestPushPartialFile(char *filename, uint16_t offset, uint16_t size, char *tag);
	void requestReadFile(char *filename);
	void requestReadPartialFile(char *filename, uint16_t offset, uint16_t size);
	void requestDeleteFile(char *filename);

protected:
	int8_t _rstpin;
	uint8_t _type;
	uint8_t _readStatus;
	uint16_t _messageRemaining;
	uint16_t _replyCounter = 0;
	unsigned long _replyTime = 0;

	char replybuffer[500];
	char unknownbuffer[500];
	char serverbuffer[500];

	uint16_t error = 0;

	Sanderbuilt_SARA_FlashStringPtr ok_reply;
	Sanderbuilt_SARA_FlashStringPtr error_reply;
	Sanderbuilt_SARA_FlashStringPtr download_reply;

	boolean sendParseReply(Sanderbuilt_SARA_FlashStringPtr tosend, Sanderbuilt_SARA_FlashStringPtr toreply, uint16_t *v, char divider = ',', uint8_t index = 0);
	Sanderbuilt_SARA_StreamType *mySerial;

private:
	static bool startsWith(const char* pre, const char* str);
	void flushInput();
	uint8_t readline(uint16_t timeout = SANDERBUILT_SARA_DEFAULT_TIMEOUT_MS, boolean multiline = false);
	uint8_t getReply(char *send, uint16_t timeout = SANDERBUILT_SARA_DEFAULT_TIMEOUT_MS);
	uint8_t getReply(Sanderbuilt_SARA_FlashStringPtr send, uint16_t timeout = SANDERBUILT_SARA_DEFAULT_TIMEOUT_MS);
	uint8_t getReply(Sanderbuilt_SARA_FlashStringPtr prefix, char *suffix, uint16_t timeout = SANDERBUILT_SARA_DEFAULT_TIMEOUT_MS);
	uint8_t getReply(Sanderbuilt_SARA_FlashStringPtr prefix, int32_t suffix, uint16_t timeout = SANDERBUILT_SARA_DEFAULT_TIMEOUT_MS);
	uint8_t getReply(Sanderbuilt_SARA_FlashStringPtr prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout); // Don't set default value or else function call is ambiguous.
	boolean parseReply(Sanderbuilt_SARA_FlashStringPtr toreply, uint16_t *v, char divider, uint8_t index);
	boolean parseReply(Sanderbuilt_SARA_FlashStringPtr toreply, char *v, char divider, uint8_t index);
	boolean parseReplyQuoted(Sanderbuilt_SARA_FlashStringPtr toreply, char *v, int maxlen, char divider, uint8_t index);
};

#endif // !_Sanderbuilt_SARA_h
