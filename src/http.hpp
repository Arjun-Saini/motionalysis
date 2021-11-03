#pragma once

#include "HttpClient/HttpClient.h"
#include "constants.hpp"

//setup for http connection
HttpClient http;
http_header_t headers[] = {
  {"Accept", "application/json"},
  {"Content-Type", "application/json"},
  {NULL, NULL}
};
http_request_t request;
http_response_t response;


int networkCount;
WiFiAccessPoint networks[5];
String networkBuffer;

void HTTPRequestSetup() {
  request.hostname = kHTTPHostname;
  request.port = kHTTPRequestPort;
  request.path = "/";
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
  }
  else {
    WITH_LOCK(Serial) {
      Serial.println("WiFi connected, reporting data");
    }
    payload.remove(payload.length() - 1);
    request.body = "{\"data\":[" + payload + "]}";
    http.post(request, response, headers);
    WITH_LOCK(Serial) {
      Serial.println("Status: " + response.status);
      Serial.println("Body: " + response.body);
      Serial.println("ReqBody: " + request.body);
    }
  }
  WiFi.off();
}