// Simple Base64 code
// (c) Copyright 2010 MCQN Ltd.
// Released under Apache License, version 2.0

#include "CREA_ESP8266.h"

void CREA_ESP8266::errorHalt(String msg)
{
  Serial.println(msg);
  Serial.println("HALT");
  while(true){};
}

// Read characters from WiFi module and echo to serial until keyword occurs or timeout.
boolean CREA_ESP8266::echoFind(String keyword)
{
  byte current_char   = 0;
  byte keyword_length = keyword.length();

  // Fail if the target string has not been sent by deadline.
  long deadline = millis() + TIMEOUT;
  while(millis() < deadline)
  {
    if (Serial1->available())
    {
      char ch = Serial1->read();
      Serial.write(ch);
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          Serial.println();
          return true;
        }
    }
  }
  return false;  // Timed out
}

// Read and echo all available module output.
// (Used when we're indifferent to "OK" vs. "no change" responses or to get around firmware bugs.)
void CREA_ESP8266::echoFlush()
  {while(Serial1->available()) Serial.write(Serial1->read());}

// Echo module output until 3 newlines encountered.
// (Used when we're indifferent to "OK" vs. "no change" responses.)
void CREA_ESP8266::echoSkip()
{
  echoFind("\n");        // Search for nl at end of command echo
  echoFind("\n");        // Search for 2nd nl at end of response.
  echoFind("\n");        // Search for 3rd nl at end of blank line.
}

// Send a command to the module and wait for acknowledgement string
// (or flush module output if no ack specified).
// Echoes all data received to the serial monitor.
boolean CREA_ESP8266::echoCommand(String cmd, String ack, boolean halt_on_fail)
{
  Serial1->println(cmd);
  #ifdef ECHO_COMMANDS
    Serial.print("--"); Serial.println(cmd);
  #endif

  // If no ack response specified, skip all available module output.
  if (ack == "")
    echoSkip();
  else
    // Otherwise wait for ack.
    if (!echoFind(ack))          // timed out waiting for ack string
      if (halt_on_fail)
        errorHalt(cmd+" failed");// Critical failure halt.
      else
        return false;            // Let the caller handle it.
  return true;                   // ack blank or ack found
}

// Send a command to the module and wait for acknowledgement string
// (or flush module output if no ack specified).
// Echoes all data received to the serial monitor.
boolean CREA_ESP8266::echoMessage(char *cmd, String ack, boolean halt_on_fail)
{
  int size = strlen(cmd);
  String com = "";
  uint8_t mask[4] = { 0x0, 0x0, 0x0, 0x0 };
  char init = 128;//WS_OPCODE_TEXT | WS_FIN;
  char thesize = size;

  com.concat(init);

  if (size <= 125){
    com.concat((char)(thesize | 0));
  }else if (size >= 126 && size <= 65535){
    com.concat((WS_SIZE16 | WS_MASK));
    com.concat((uint8_t) (size >> 8));
    com.concat((uint8_t) (size) & 0xFF);
  }else{
    com.concat((WS_SIZE64 | WS_MASK));
    com.concat((uint8_t) (size >> 56));
    com.concat((uint8_t) (size >> 48));
    com.concat((uint8_t) (size >> 40));
    com.concat((uint8_t) (size >> 32));
    com.concat((uint8_t) (size >> 24));
    com.concat((uint8_t) (size >> 16));
    com.concat((uint8_t) (size >> 8));
    com.concat((uint8_t) (size) & 255);
  }

  com.concat((char)mask[0]);
  com.concat((char)mask[1]);
  com.concat((char)mask[2]);
  com.concat((char)mask[3]);

  for (int i=0; i<thesize; ++i) {
    //com.concat((char)(cmd[i] ^ mask[i % 4]));
    com.concat(cmd[i]);
  }

  if (!echoCommand("AT+CIPSEND=0,"+String(size+6), ">", CONTINUE))
  {
    echoCommand("AT+CIPCLOSE", "", CONTINUE);
    Serial.println("Connection timeout.");
    return false;
  }

  Serial1->print(com);

  //Serial1.write((byte)(*cmd>>8));
  #ifdef ECHO_COMMANDS
    Serial.print("--"); Serial.println(cmd);
  #endif

  // If no ack response specified, skip all available module output.
  if (ack == "")
    echoSkip();
  else
    // Otherwise wait for ack.
    if (!echoFind(ack))          // timed out waiting for ack string
      if (halt_on_fail)
        errorHalt(" failed");// Critical failure halt.
      else
        return false;            // Let the caller handle it.
  return true;                   // ack blank or ack found
}

// Connect to the specified wireless network.
boolean CREA_ESP8266::connectWiFi()
{
  String cmd = "AT+CWJAP=\""; cmd += SSID; cmd += "\",\""; cmd += PASS; cmd += "\"";
  if (echoCommand(cmd, "OK", CONTINUE)) // Join Access Point
  {
    Serial.println("Connected to WiFi.");
    return true;
  }
  else
  {
    Serial.println("Connection to WiFi failed.");
    return false;
  }
}


void CREA_ESP8266::CREA_setup(String _SSID, String _PASS, const char* _MODULEID, const char* _AUTH){
  SSID = _SSID;
  PASS = _PASS;
  MODULEID = _MODULEID;
  AUTH = _AUTH;
  Serial1 = new SoftwareSerial(10, 11);

  Serial.begin(115200);         // Communication with PC monitor via USB
  Serial1->begin(115200);        // Communication with ESP8266 via 5V/3.3V level shifter

  Serial1->setTimeout(TIMEOUT);
  Serial.println("CREA ESP8266");

  delay(2000);

  echoCommand("AT+RST", "ready", HALT);    // Reset & test if the module is ready
  Serial.println("Module is ready.");
  echoCommand("AT+GMR", "OK", CONTINUE);   // Retrieves the firmware ID (version number) of the module.
  echoCommand("AT+CWMODE?","OK", CONTINUE);// Get module access mode.

  echoCommand("AT+CWMODE=1", "", HALT);    // Station mode
  echoCommand("AT+CIPMUX=1", "", HALT);    // Allow multiple connections (we'll only use the first).

  //connect to the wifi
  boolean connection_established = false;
  for(int i=0;i<5;i++)
  {
    if(connectWiFi())
    {
      connection_established = true;
      break;
    }
  }
  if (!connection_established) errorHalt("Connection failed");

  delay(5000);

  echoCommand("AT+CIPSERVER=1,9000", "OK", HALT);
  //echoCommand("AT+CWSAP=?", "OK", CONTINUE); // Test connection
  echoCommand("AT+CIFSR", "", HALT);         // Echo IP address. (Firmware bug - should return "OK".)
  //echoCommand("AT+CIPMUX=0", "", HALT);      // Set single connection mode
}

boolean CREA_ESP8266::execute(String order){
  order.trim();
  if (order != "NA"){
    command = order.substring(0,2);
    ref = order.substring(3,order.indexOf("|")).toInt();
    value = order.substring(order.indexOf("|")+1,order.length());
    if (command == "DO"){
        int v = value.toInt();
        if (v <= 0){
            digitalWrite(ref, LOW);
        }
        digitalWrite(ref, v);
        return true;
    }else if( command == "AO" ){
        analogWrite(ref, value.toInt());
        return true;
    }else if( command == "DI" ){
        digitalData = digitalRead(ref);
        return true;
    }else if( command == "AI" ){
        digitalData = analogRead(ref);
        return true;
    }else if( command == "SR"){
        return true;
    }else if( command == "ST"){
        return true;
    }
    return false;
  }else{
    return true;
  }
}

void CREA_ESP8266::setResponse(char* message){
  CALL_RESP = message;
  executed = true;
}

char* CREA_ESP8266::concatChar(const char* a, const char* b){
  char* to_return;
  int len = strlen(a) + strlen(b) + 1;
  to_return = (char*)malloc( len );
  strcpy(to_return, a); /* copy name into the new var */
  strcat(to_return, b); /* add the extension */

  return to_return;
}

void CREA_ESP8266::CREA_loop(GeneralMessageFunction callback){
  int stage = AUTHENTICATING; //THE STAGE OF THE OPERATION:
  //1: authenticating (with token)
  //2: subscribing the module
  //3: operation
  //4: error
  int next_stage;

  // Establish TCP connection
  String ack = "NEL";
  String cmd = "AT+CIPSTART=0,\"TCP\",\""; cmd += DEST_IP; cmd += "\",9000";
  if (!echoCommand(cmd, "OK", CONTINUE)) { echoCommand("AT+CIPCLOSE=0", "", CONTINUE); return;}
  delay(2000);

  // Get connection status
  if (!echoCommand("AT+CIPSTATUS", "OK", CONTINUE)) return;

  // Build WEBSOCKET request.
  cmd = "GET /"; cmd += " HTTP/1.1\r\n";  cmd += "Upgrade: websocket\r\n"; cmd += "Connection: Upgrade\r\nSec-WebSocket-Key: "; cmd += "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="; cmd += "\r\n"; cmd += "Host: "; cmd += DEST_HOST; cmd += ":"; cmd += PORT; cmd += "\r\nSec-WebSocket-Protocol: chat\r\nSec-WebSocket-Version: 13\r\n\r\n";

  // Ready the module to receive raw data
  if (!echoCommand("AT+CIPSEND=0,"+String(cmd.length()), ">", CONTINUE))
  {
    echoCommand("AT+CIPCLOSE", "", CONTINUE);
    Serial.println("Connection timeout.");
    return;
  }

  Serial.println("SUBSCRIBING");
  // Send the raw HTTP request
  echoCommand(cmd, "OK", CONTINUE);  // GET

  // Loop forever echoing data received from destination server.
  boolean completed = false;
  executed = false;

  while(!completed){
    while (Serial1->available()){
      char c = Serial1->read();
      Serial.write(c);
      response += c;
      if (response.indexOf("OK") > 0){
        completed = true;
      }
      if (c == '\r') { response = ""; Serial.print('\n'); };
    }
  }

  delay(1000);

  while (completed){

    // Send the CREA commands
    //PSEUDO State Machine
    switch (stage){
      case AUTHENTICATING:
        echoMessage(concatChar("CRAUTH|", AUTH), "OK", CONTINUE);
        next_stage = SUBSCRIBING;
        ack = "ACK";
        CALL_RESP = "";
        break;
      case SUBSCRIBING:
        echoMessage(concatChar("SUBSCR|", MODULEID), "OK", CONTINUE);
        next_stage = OPERATION;
        ack = "ACK";
        break;
      case OPERATION:
        if (!executed){
          echoMessage("GET", "OK", CONTINUE);
        }else{
          echoMessage(concatChar("SEND|", CALL_RESP), "OK", CONTINUE);
          executed = false;
        }
        ack = "OK";
        break;
      default:
        next_stage = AUTHENTICATING;
        break;
    }

    delay(TIMEOUT);

    while (Serial1->available()){
      char c = Serial1->read();
      //Serial.write(c);
      response += c;

      //MOVES THE OPERATION STAGE
      if (response.indexOf(ack) > 0){// && executed){
        stage = next_stage;
      }

      if (c == '\r') {
        if (stage == OPERATION && !executed){
          if (response.indexOf("IPD") > 0){
            response = response.substring(response.indexOf(":")+3,response.length());
            callback(response);
          }
        }
        response = ""; Serial.print('\n');
      };
    }

    //echoCommand("AT+CIPCLOSE=0", "", CONTINUE);
  }
  delay(TIMEOUT);

}
