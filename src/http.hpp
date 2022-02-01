#pragma once

#include "HttpClient/HttpClient.h"
#include "constants.hpp"
#include "globalVariables.hpp"

//setup for http connection
HttpClient http;
http_header_t headers[] = {
  {"Accept", "application/json"},
  {"Content-Type", "application/json"},
  {NULL, NULL}
};
http_header_t timeheaders[] = {
  {NULL, NULL}
};
http_request_t request;
http_response_t response;
http_request_t timerequest;
http_response_t timeresponse;

int networkCount;
WiFiAccessPoint networks[5];
String networkBuffer;

void HTTPRequestSetup() {
  request.hostname = kHTTPHostname;
  request.port = kHTTPRequestPort;
  request.path = "/";

  timerequest.hostname = kHTTPHostname;
  timerequest.port = kHTTPRequestPort;
  timerequest.path = "/time";
}

void getTime() {
  WiFi.on();
  WiFi.connect();
  while(!WiFi.ready()){
    delay(100);
  }
  if(WiFi.ready() != true) {
    WITH_LOCK(Serial) {
      Serial.println("WiFi failed to connect, time not synced");
    }
  }
  else {
    WITH_LOCK(Serial) {
      Serial.println("WiFi connected, syncing time");
    }
    WITH_LOCK(Serial) {
      http.get(timerequest, timeresponse, timeheaders);
    }
    WITH_LOCK(Serial) {
      Serial.println("Time request returned: " + timeresponse.body);
      Time.setTime(timeresponse.body.toInt());
    }
  }
}

void reportData(String payload) {
  WiFi.on();
  WiFi.connect();
  while(!WiFi.ready()) {
    delay(100);
  }
  if(WiFi.ready() != true) {
    WITH_LOCK(Serial) {
      Serial.println("WiFi failed to connect, data not reported");
    }
    rolloverPayload += payload;
  }
  else {
    WITH_LOCK(Serial) {
      Serial.println("WiFi connected, reporting data");
    }
    if(rolloverPayload != "") {
      WITH_LOCK(Serial) {
        Serial.println("Rollover payload: " + rolloverPayload);
      }
      payload += rolloverPayload;
    }
    payload.remove(payload.length() - 1);
    request.body = "{\"data\":[" + payload + "]}";
    WITH_LOCK(Serial){
      http.post(request, response, headers); //http library is not thread safe with serial, so we need to lock it to prevent panic
    }
    WITH_LOCK(Serial) {
      Serial.println("Status: " + response.status);
    }
    WITH_LOCK(Serial) {
      Serial.println("Body: " + response.body);
    }
    WITH_LOCK(Serial) {
      Serial.println("ReqBody: " + request.body);
    }
    rolloverPayload = "";
  }
  WiFi.off();
}