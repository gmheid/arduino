#include "LocalServer.h"

LocalServer::LocalServer(Stream &log, OpenMetrics &openMetrics,
                         Measurements &measure, Configuration &config,
                         WifiConnector &wifiConnector)
    : PrintLog(log, "LocalServer"), openMetrics(openMetrics), measure(measure),
      config(config), wifiConnector(wifiConnector) {}

LocalServer::~LocalServer() {}

bool LocalServer::begin(void) {
  server.on("/measures/current", HTTP_GET, [this]() { _GET_measure(); });
  server.on(openMetrics.getApi(), HTTP_GET, [this]() { _GET_metrics(); });
  server.on("/config", HTTP_GET, [this]() { _GET_config(); });
  server.on("/config", HTTP_PUT, [this]() { _PUT_config(); });
  server.on("/sleep", HTTP_GET, [this]() { _PMS_sleep(); });
  server.on("/wakeup", HTTP_GET, [this]() { _PMS_wakeup(); });
  server.on("/restart", HTTP_GET, [this]() { _GET_restart(); });
  server.on("/night", HTTP_GET, [this]() { _GET_night(); });
  server.on("/night2", HTTP_GET, [this]() { _GET_night2(); });
  server.on("/day", HTTP_GET, [this]() { _GET_day(); });
  server.begin();

  if (xTaskCreate(
          [](void *param) {
            LocalServer *localServer = (LocalServer *)param;
            for (;;) {
              localServer->_handle();
            }
          },
          "webserver", 1024 * 4, this, 5, NULL) != pdTRUE) {
    Serial.println("Create task handle webserver failed");
  }
  logInfo("Init: " + getHostname() + ".local");

  return true;
}

void LocalServer::setAirGraident(AirGradient *ag) { this->ag = ag; }

String LocalServer::getHostname(void) {
  return "airgradient_" + ag->deviceId();
}

void LocalServer::_handle(void) { server.handleClient(); }

void LocalServer::_GET_config(void) {
  if(ag->isOne()) {
    server.send(200, "application/json", config.toString());
  } else {
    server.send(200, "application/json", config.toString(fwMode));
  }
}

void LocalServer::_PUT_config(void) {
  String data = server.arg(0);
  String response = "";
  int statusCode = 400; // Status code for data invalid
  if (config.parse(data, true)) {
    statusCode = 200;
    response = "Success";
  } else {
    response = config.getFailedMesage();
  }
  server.send(statusCode, "text/plain", response);
}

void LocalServer::_GET_metrics(void) {
  server.send(200, openMetrics.getApiContentType(), openMetrics.getPayload());
}

void LocalServer::_GET_measure(void) {
  String toSend = measure.toString(true, fwMode, wifiConnector.RSSI());
  server.send(200, "application/json", toSend);
}

void LocalServer::setFwMode(AgFirmwareMode fwMode) { this->fwMode = fwMode; }

void LocalServer::_PMS_sleep(void) {
  if(ag->isOne()) {
    ag->pms5003.sleep();
    server.send(200, "text/plain", "Sleep Success\n");
    config.hasSensorPMS1 = false;
  } else {
    server.send(400, "text/plain", "Supported only for Indoor monitor\n");
  }

}

void LocalServer::_PMS_wakeup(void) {
  if(ag->isOne()) {
    ag->pms5003.wakeUp();
    config.hasSensorPMS1 = true;
    server.send(200, "text/plain", "Wakeup Success\n");
  } else {
    server.send(400, "text/plain", "Supported only for Indoor monitor\n");
  }
}

void LocalServer::_GET_restart(void) {
  server.send(200, "text/plain", "Success\n");
  // Wake PMS up, only for indoor
  if(ag->isOne()) {
    config.hasSensorPMS1 = true;
    ag->pms5003.wakeUp();
    delay(2000);
  } 
  delay(1000);
  ESP.restart();
}

// Nightmode: Display off and LED 5%
void LocalServer::_GET_night(void) {
  if(ag->isOne()) {
    ag->pms5003.sleep();
    String data = "{\"ledBarBrightness\": 5, \"displayBrightness\": 0 }";
    String response = "";
    int statusCode = 400; // Status code for data invalid
    if (config.parse(data, true)) {
      statusCode = 200;
      response = "Nightmode Success";
    } else {
      response = config.getFailedMesage();
    }
    server.send(statusCode, "text/plain", response);
  } else {
    server.send(400, "text/plain", "Supported only for Indoor monitor\n");
  }
}

//Nightmode2: Display and LED off
void LocalServer::_GET_night2(void) {
  if(ag->isOne()) {
    ag->pms5003.sleep();
    String data = "{\"ledBarBrightness\": 0, \"displayBrightness\": 0 }";
    String response = "";
    int statusCode = 400; // Status code for data invalid
    if (config.parse(data, true)) {
      statusCode = 200;
      response = "Nightmode2 Success";
    } else {
      response = config.getFailedMesage();
    }
    server.send(statusCode, "text/plain", response);
  } else {
    server.send(400, "text/plain", "Supported only for Indoor monitor\n");
  }
}

void LocalServer::_GET_day(void) {
  if(ag->isOne()) {
    ag->pms5003.wakeUp();
    String data = "{\"ledBarBrightness\": 15, \"displayBrightness\": 50 }";
    String response = "";
    int statusCode = 400; // Status code for data invalid
    if (config.parse(data, true)) {
      statusCode = 200;
      response = "Daymode Success";
    } else {
      response = config.getFailedMesage();
    }
    server.send(statusCode, "text/plain", response);
  } else {
    server.send(400, "text/plain", "Supported only for Indoor monitor\n");
  }
}

