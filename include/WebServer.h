
#include "Scale.h"
#include "FlowRate.h"
#include "BluetoothScale.h"
#include "Display.h"
#include "BatteryMonitor.h"

extern float calibrationFactor;

void setupWebServer(Scale &scale, FlowRate &flowRate, BluetoothScale &bluetoothScale, Display &display, BatteryMonitor &battery);
void startWebServer();
void stopWebServer();