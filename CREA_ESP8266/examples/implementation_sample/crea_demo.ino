#include "CREA_ESP8266.h";

CREA_ESP8266 modulo;
// ******** SETUP ********
void setup()
{
  String SSID = "NETGEAR";
  String PASS = "0000011111";
  const char* MODULEID = "a047b0534e65d829bf54b3561dd0c0b1";
  const char* AUTH = "ZDc2MGQ2Mzg5YzE1M2FkZDRiYzMwMjFkNzI1ZjZhNmE6OTZhZjM3MTk4YzIxMDRmOTgyYzcwOGRhNGMzMzg3MzY=";
  pinMode(9, OUTPUT);
  modulo.CREA_setup(SSID, PASS, MODULEID, AUTH);
}

// ******** LOOP ********
void loop()
{
  modulo.CREA_loop(handle_command);
}

void handle_command(String command)
{
  if (modulo.execute(command)){
    if (modulo.command == "DO"){
        modulo.set_response("OK");
    }else if( modulo.command == "AO" ){
        modulo.set_response("OK");
    }else if( modulo.command == "SR" ){
        modulo.set_response(modulo.concat_char("PROCESO: ",modulo.value.c_str()));
    }else if( modulo.command == "ST" ){
        Serial.println("STATUS REQUEST");
        modulo.set_response("RETORNO+DE+STATUS");
    }
  }
}
