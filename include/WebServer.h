#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "Scale.h"
#include "FlowRate.h"
#include "BluetoothScale.h"
#include "Display.h"
#include "BatteryMonitor.h"

extern float calibrationFactor;

void setupWebServer(Scale &scale, FlowRate &flowRate, BluetoothScale &bluetoothScale, OledDisplay &display, BatteryMonitor &battery);
void startWebServer();
void stopWebServer();

#endif
