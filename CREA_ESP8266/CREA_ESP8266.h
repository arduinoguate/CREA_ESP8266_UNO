#ifndef CREA_ESP8266_h
#define CREA_ESP8266_h

#if ARDUINO < 100
#include <WProgram.h>
#include <pins_arduino.h>  // fix for broken pre 1.0 version - TODO TEST
#else
#include <Arduino.h>
#endif
#include "SoftwareSerial.h"

#define DEST_HOST   "crea.arduinogt.com"
#define PORT		"9000"
#define DEST_IP     "45.55.134.101"
#define TIMEOUT     5000 // mS
#define CONTINUE    false
#define HALT        true
#define INPUT_SIZE  30

#define WS_FIN            0x80
#define WS_OPCODE_TEXT    0x1
#define WS_OPCODE_BINARY  0x2
#define WS_OPCODE_CLOSE   0x08
#define WS_OPCODE_PING    0x09
#define WS_OPCODE_PONG    0x0a
// Second byte
#define WS_MASK           0x80
#define WS_SIZE16         126
#define WS_SIZE64         127

//CREA STAGES
#define AUTHENTICATING	  0
#define SUBSCRIBING		  1
#define OPERATION		  2
#define ERR_1			  3
#define ERR_2			  4
#define ERR_3			  5
#define ERR_4			  6

class CREA_ESP8266
{
	typedef void (*GeneralMessageFunction) (String value);
public:
	void CREA_setup(String _SSID, String _PASS, const char* _MODULEID, const char* _AUTH);
	boolean execute(String order);
	void CREA_loop(GeneralMessageFunction callback);
	void setResponse(char* message);
	char* concatChar(const char* a, const char* b);
	int digitalData;
	String value;
	String command;
	int ref;

private:
	void errorHalt(String msg);
	boolean echoFind(String keyword);
	void echoFlush();
	void echoSkip();
	boolean echoMessage(char *cmd, String ack, boolean halt_on_fail);
	boolean echoCommand(String cmd, String ack, boolean halt_on_fail);
	boolean connectWiFi();

	String response;
	String SSID;
	String PASS;
	const char* MODULEID;
	const char* AUTH;
	char* CALL_RESP;
	boolean executed;
	SoftwareSerial* Serial1;
};

#endif
