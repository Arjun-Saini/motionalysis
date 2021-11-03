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
