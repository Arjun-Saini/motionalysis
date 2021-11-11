#pragma once
#include "constants.hpp"

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
void connectCallback(const BlePeerDevice& peer, void* context);
void disconnectCallback(const BlePeerDevice& peer, void* context);