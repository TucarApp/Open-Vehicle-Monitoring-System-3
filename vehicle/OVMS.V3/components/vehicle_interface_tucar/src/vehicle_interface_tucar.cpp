/*
;    Project:       Open Vehicle Monitor System
;    Date:          20th August 2024
;
;    Changes:
;    1.0  Initial release
;
;    (C) 2021       Jaime Middleton / Tucar
;    (C) 2021       Axel Troncoso   / Tucar
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
*/

#include "vehicle_interface_tucar.h"

#include <cstring>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "ovms_config.h"
#include "ovms_peripherals.h"
#include "ovms_log.h"
#include "ovms_metrics.h"
#include "ovms_boot.h"

#define DEFAULT_PSWD "tucar"
#define DEFAULT_STR "str"

#ifdef CONFIG_OVMS_TUCAR_WIFI_AP_PSWD
static const std::string defaultAccessPointPswd = CONFIG_OVMS_TUCAR_WIFI_AP_PSWD;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_WIFI_AP_PSWD, DEFAULT_PSWD),
              "Invalid wifi access point. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
#error "CONFIG_OVMS_TUCAR_WIFI_AP_PSWD must be defined"
#endif // CONFIG_OVMS_TUCAR_WIFI_AP_PSWD

#ifdef CONFIG_OVMS_TUCAR_SERVER_V3_USER
static const std::string defaultServerV3User = CONFIG_OVMS_TUCAR_SERVER_V3_USER;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_SERVER_V3_USER, DEFAULT_STR),
              "Invalid wifi access point. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
#error "CONFIG_OVMS_TUCAR_SERVER_V3_USER must be defined"
#endif // CONFIG_OVMS_TUCAR_SERVER_V3_USER

#ifdef CONFIG_OVMS_TUCAR_SERVER_V3_SERVER
static const std::string defaultServerV3Server = CONFIG_OVMS_TUCAR_SERVER_V3_SERVER;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_SERVER_V3_SERVER, DEFAULT_STR),
              "Invalid wifi access point. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
#error "CONFIG_OVMS_TUCAR_SERVER_V3_SERVER must be defined"
#endif // CONFIG_OVMS_TUCAR_SERVER_V3_SERVER

#ifdef CONFIG_OVMS_TUCAR_SERVER_V3_PSWD
static const std::string defaultServerV3Pswd = CONFIG_OVMS_TUCAR_SERVER_V3_PSWD;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_SERVER_V3_PSWD, DEFAULT_PSWD),
              "Invalid wifi access point. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
#error "CONFIG_OVMS_TUCAR_SERVER_V3_PSWD must be defined"
#endif // CONFIG_OVMS_TUCAR_SERVER_V3_PSWD

#ifdef CONFIG_OVMS_TUCAR_MODULE_PSWD
static const std::string defaultModulePswd = CONFIG_OVMS_TUCAR_MODULE_PSWD;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_MODULE_PSWD, DEFAULT_PSWD),
              "Invalid wifi access point. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
#error "CONFIG_OVMS_TUCAR_MODULE_PSWD must be defined"
#endif // CONFIG_OVMS_TUCAR_MODULE_PSWD

#ifdef CONFIG_OVMS_TUCAR_MODULE_PIN
static const std::string defaultModulePin = CONFIG_OVMS_TUCAR_MODULE_PIN;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_MODULE_PIN, DEFAULT_PSWD),
              "Invalid wifi access point. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
#error "CONFIG_OVMS_TUCAR_MODULE_PIN must be defined"
#endif // CONFIG_OVMS_TUCAR_MODULE_PIN

#ifdef CONFIG_OVMS_TUCAR_WIFI_1_SSID
static const std::string defaultWifi1SSID = CONFIG_OVMS_TUCAR_WIFI_1_SSID;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_WIFI_1_SSID, DEFAULT_PSWD),
              "Invalid wifi access point. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
#error "CONFIG_OVMS_TUCAR_WIFI_1_SSID must be defined"
#endif // CONFIG_OVMS_TUCAR_WIFI_1_SSID

#ifdef CONFIG_OVMS_TUCAR_WIFI_2_SSID
static const std::string defaultWifi2SSID = CONFIG_OVMS_TUCAR_WIFI_2_SSID;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_WIFI_2_SSID, DEFAULT_PSWD),
              "Invalid wifi access point. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
#error "CONFIG_OVMS_TUCAR_WIFI_2_SSID must be defined"
#endif // CONFIG_OVMS_TUCAR_WIFI_2_SSID

#ifdef CONFIG_OVMS_TUCAR_WIFI_1_PSWD
static const std::string defaultWifi1Pwsd = CONFIG_OVMS_TUCAR_WIFI_1_PSWD;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_WIFI_1_PSWD, DEFAULT_PSWD),
              "Invalid wifi access point. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
#error "CONFIG_OVMS_TUCAR_WIFI_1_PSWD must be defined"
#endif // CONFIG_OVMS_TUCAR_WIFI_1_PSWD

#ifdef CONFIG_OVMS_TUCAR_WIFI_2_PSWD
static const std::string defaultWifi2Pwsd = CONFIG_OVMS_TUCAR_WIFI_2_PSWD;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_WIFI_2_PSWD, DEFAULT_PSWD),
              "Invalid wifi access point. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
#error "CONFIG_OVMS_TUCAR_WIFI_2_PSWD must be defined"
#endif // CONFIG_OVMS_TUCAR_WIFI_2_PSWD

static const char *TAG = "v-iface-tucar";
static const std::string defaultId(15, '0');

namespace
{
  struct hasherfunc
  {
    size_t operator()(const std::pair<std::string, std::string> &p) const
    {
      return std::hash<std::string>()(p.first) ^ (std::hash<std::string>()(p.second) << 1);
    }
  };

  template <typename T>
  // using ParamMap = std::unordered_map<std::string, std::pair<std::string, T>>;
  using ParamMap = std::unordered_map<std::pair<std::string, std::string>, T, hasherfunc>;

  /*
  {"auto", {"vehicle.type", "MPL60S"}},
  {"vehicle", {"timezone", ""}}, // revisar
  {"vehicle", {"timezone_region"}}, // revisar
*/

  static const ParamMap<std::string> defaultStringParams = {
      // {"egpio", {"monitor.ports", "1,2"}} list of port numbers separated by spaces
      // {"server.v3", {"metrics.exclude", "coma-separated-value"}}, // revisar
      // {"server.v3", {"metrics.include", "coma-separated-value"}}, // revisar
      {{"auto", "wifi.mode"}, "client"},
      {{"auto", "wifi.Pwsd.client"}, ""},
      {{"log", "file.path"}, "/sd/logs/log"},
      {{"log", "level"}, "error"},
      {{"modem", "apn"}, "m2m.entel.cl"},
      {{"modem", "apn.password"}, ""},
      {{"modem", "apn.user"}, ""},
      {{"modem", "driver"}, "auto"},
      {{"modem", "pincode"}, ""},
      {{"module", "init"}, "done"},
      {{"ota", "http.mru"}, ""},
      {{"ota", "auto.hour"}, "2"},
      {{"password", "server.v2"}, ""},
      {{"password", "server.v3"}, defaultServerV3Pswd},
      {{"password", "module"}, defaultModulePswd},
      {{"password", "pin"}, defaultModulePin},
      {{"server.v3", "port"}, "8883"},
      {{"server.v3", "server"}, defaultServerV3Server},
      {{"server.v3", "user"}, defaultServerV3User},
      {{"vehicle", "units.accel"}, "kmphps"},
      {{"vehicle", "units.accelshort"}, "mpss"},
      {{"vehicle", "units.charge"}, "amphours"},
      {{"vehicle", "units.consumption"}, "kmpkwh"},
      {{"vehicle", "units.distance"}, "K"},
      {{"vehicle", "units.distanceshort"}, "meters"},
      {{"vehicle", "units.energy"}, "kwh"},
      {{"vehicle", "units.power"}, "kw"},
      {{"vehicle", "units.pressure"}, "kpa"},
      {{"vehicle", "units.ratio"}, "percent"},
      {{"vehicle", "units.signal"}, "dbm"},
      {{"vehicle", "units.speed"}, "kmph"},
      {{"vehicle", "units.temp"}, "celcius"},
      {{"wifi.ssid", defaultWifi1SSID}, defaultWifi1Pwsd},
      {{"wifi.ssid", defaultWifi2SSID}, defaultWifi2Pwsd},
  };

  static const ParamMap<bool> defaultBoolParams = {
      {{"auto", "dbc"}, false},
      {{"auto", "egpio"}, true},
      {{"auto", "ext12v"}, false},
      {{"auto", "init"}, true},
      {{"auto", "modem"}, true},
      {{"auto", "obd2ecu"}, false},
      {{"auto", "ota"}, false},
      {{"auto", "scripting"}, true},
      {{"auto", "server.v2"}, false},
      {{"auto", "server.v3"}, true},
      {{"auto", "swcan"}, false},
      {{"log", "file.enable"}, false},
      {{"log", "poller.timers"}, false},
      {{"modem", "enable.gps"}, true},
      {{"modem", "enable.gpstime"}, true},
      {{"modem", "enable.net"}, true},
      {{"modem", "enable.sms"}, true},
      {{"module", "debug.tasks"}, false},
      {{"network", "reboot.no.ip"}, true},
      {{"network", "wifi.bad.reconnect"}, false},
      {{"notify", "report.trip.enable"}, false},
      {{"obd2ecu", "autocreate"}, false},
      {{"obd2ecu", "private"}, false},
      {{"ota", "auto.allow.modem"}, false},
      {{"power", "enabled"}, false},
      {{"pushover", "enable"}, false},
      {{"sdcard", "automount"}, true},
      {{"server.v3", "tls"}, true},
      {{"vehicle", "bms.alerts.enabled"}, false},
      {{"vehicle", "brakelight.enable"}, false},
      {{"vehicle", "tpms.alerts.enabled"}, false},
  };

  static const ParamMap<int> defaultIntParams = {
      {{"log", "file.keepdays"}, 7},
      {{"log", "file.maxsize"}, 1024},
      {{"log", "file.syncperiod"}, 3},
      {{"network", "modem.sq.bad"}, -93},
      {{"network", "modem.sq.good"}, -95},
      {{"network", "wifi.ap.bw"}, 20},
      {{"network", "wifi.scan.tmax"}, 120},
      {{"network", "wifi.scan.tmin"}, 120},
      {{"network", "wifi.sq.bad"}, -89},
      {{"network", "wifi.sq.good"}, -87},
      {{"notify", "log.grid.storetime"}, 0},
      {{"notify", "log.trip.minlength"}, 0},
      {{"notify", "log.trip.storetime"}, 0},
      {{"notify", "report.trip.minlength"}, 0},
      {{"power", "12v_shutdown_delay"}, 0},
      {{"power", "modemoff_delay"}, 0},
      {{"power", "wifioff_delay"}, 0},
      {{"sdcard", "maxfreq.khz"}, 16000},
      {{"server.v3", "updatetime.awake"}, 1},
      {{"server.v3", "updatetime.charging"}, 5},
      {{"server.v3", "updatetime.connected"}, 1},
      {{"server.v3", "updatetime.idle"}, 60},
      {{"server.v3", "updatetime.on"}, 1},
      {{"server.v3", "updatetime.sendall"}, 300},
      {{"vehicle", "12v.shutdown"}, 0}, // turn of device under this voltage
      {{"vehicle", "12v.shutdown_delay"}, 2},
      {{"vehicle", "12v.wakeup"}, 0}, // turn on device over this voltage
      {{"vehicle", "12v.wakeup_interval"}, 60},
      {{"vehicle", "bms.log.temp.interval"}, 0},
      {{"vehicle", "bms.log.voltage.interval"}, 0},
      {{"vehicle", "gps.sq.bad"}, 30}, // % of signal quality (sat/hdop)
      {{"vehicle", "gps.sq.good"}, 45},
      {{"vehicle", "minsoc"}, 0},
      {{"vehicle", "stream"}, 5},
  };

  static const ParamMap<float> defaultFLoatParams = {
      {{"vehicle", "12v.alert"}, 2.1},
      {{"vehicle", "12v.ref"}, 12.6},
      {{"vehicle", "accel.smoothing"}, 2.0}, // to calculate accel call CalculateAcceleration()
      {{"vehicle", "batpwr.smoothing"}, 2.0},
  };

  enum class ParamSetResult
  {
    Success,
    Fail,
    Ready
  };

  template <typename T>
  ParamSetResult setParamConfig(
      const std::string &moduleKey, const std::string &paramKey, const T &paramValue)
  {
    return ParamSetResult::Fail;
  }

  template <>
  ParamSetResult setParamConfig(
      const std::string &moduleKey, const std::string &paramKey, const std::string &paramValue)
  {
    if (MyConfig.GetParamValue(moduleKey, paramKey) != paramValue)
    {
      ESP_LOGE(TAG, "Reconfiguring %s %s", moduleKey.c_str(), paramKey.c_str());
      MyConfig.SetParamValue(moduleKey, paramKey, paramValue);
      if (MyConfig.GetParamValue(moduleKey, paramKey) == paramValue)
        return ParamSetResult::Success;
      return ParamSetResult::Fail;
    }
    return ParamSetResult::Ready;
  }

  template <>
  ParamSetResult setParamConfig(
      const std::string &moduleKey, const std::string &paramKey, const bool &paramValue)
  {
    if (MyConfig.GetParamValueBool(moduleKey, paramKey, !paramValue) != paramValue)
    {
      ESP_LOGE(TAG, "Reconfiguring %s %s", moduleKey.c_str(), paramKey.c_str());
      MyConfig.SetParamValueBool(moduleKey, paramKey, paramValue);
      if (MyConfig.GetParamValueBool(moduleKey, paramKey, !paramValue) == paramValue)
        return ParamSetResult::Success;
      return ParamSetResult::Fail;
    }
    return ParamSetResult::Ready;
  }

  template <>
  ParamSetResult setParamConfig(
      const std::string &moduleKey, const std::string &paramKey, const int &paramValue)
  {
    if (MyConfig.GetParamValueInt(moduleKey, paramKey, paramValue + 1) != paramValue)
    {
      ESP_LOGE(TAG, "Reconfiguring %s %s", moduleKey.c_str(), paramKey.c_str());
      MyConfig.SetParamValueInt(moduleKey, paramKey, paramValue);
      if (MyConfig.GetParamValueInt(moduleKey, paramKey, paramValue + 1) == paramValue)
        return ParamSetResult::Success;
      return ParamSetResult::Fail;
    }
    return ParamSetResult::Ready;
  }

  template <>
  ParamSetResult setParamConfig(
      const std::string &moduleKey, const std::string &paramKey, const float &paramValue)
  {
    if (MyConfig.GetParamValueFloat(moduleKey, paramKey, paramValue + 1) != paramValue)
    {
      ESP_LOGE(TAG, "Reconfiguring %s %s", moduleKey.c_str(), paramKey.c_str());
      MyConfig.SetParamValueFloat(moduleKey, paramKey, paramValue);
      if (MyConfig.GetParamValueFloat(moduleKey, paramKey, paramValue + 1) == paramValue)
        return ParamSetResult::Success;
      return ParamSetResult::Fail;
    }
    return ParamSetResult::Ready;
  }

  template <typename T>
  int setParamConfigMap(const ParamMap<T> &paramMap)
  {
    int reconfigCounter = 0;
    for (const auto &configElement : paramMap)
    {
      const auto &moduleKey = configElement.first.first;
      const auto &paramKey = configElement.first.second;
      const auto &paramValue = configElement.second;

      if (uxQueueSpacesAvailable(MyEvents.m_taskqueue) > CONFIG_OVMS_HW_EVENT_QUEUE_SIZE / 4)
      {
        auto result = setParamConfig(moduleKey, paramKey, paramValue);
        assert(result != ParamSetResult::Fail);
        if (result == ParamSetResult::Success)
          reconfigCounter++;
      }
      else
      {
        reconfigCounter++;
      }
    }
    return reconfigCounter;
  }

  void setIdConfigs(const std::string &id)
  {
    std::string prefix = "ovms/car/";
    prefix.append(id);
    prefix.append("/");
    auto result = ParamSetResult::Fail;

    int counter = 0;
    result = setParamConfig("vehicle", "id", id);
    assert(result != ParamSetResult::Fail);
    if (result == ParamSetResult::Success)
    {
      counter++;
    }
    result = setParamConfig("server.v3", "topic.prefix", prefix);
    assert(result != ParamSetResult::Fail);
    if (result == ParamSetResult::Success)
    {
      counter++;
    }
    result = setParamConfig("auto", "wifi.ssid.ap", id);
    assert(result != ParamSetResult::Fail);
    if (result == ParamSetResult::Success)
    {
      counter++;
    }
    result = setParamConfig("wifi.ap", id, defaultAccessPointPswd);
    assert(result != ParamSetResult::Fail);
    if (result == ParamSetResult::Success)
    {
      counter++;
    }
    if (counter > 0)
    {
      ESP_LOGE(TAG, "ID configurations set, restarting");
      MyBoot.Restart();
    }
  }
} // namespace

OvmsVehicleInterfaceTucar::OvmsVehicleInterfaceTucar()
{

  ESP_LOGI(TAG, "Initialising Tucar Vehicle Interface");
  m_reset_by_config = false;
  m_config_ready = false;
  /* Set imei once modem has received it. */
  MyEvents.RegisterEvent(
      TAG, "system.modem.received.imei",
      std::bind(
          &OvmsVehicleInterfaceTucar::reconfigId,
          this, std::placeholders::_1, std::placeholders::_2));
  MyEvents.RegisterEvent(
      TAG, "ticker.1",
      std::bind(
          &OvmsVehicleInterfaceTucar::reconfigSystem,
          this, std::placeholders::_1, std::placeholders::_2));

  MyEvents.RegisterEvent(
      TAG, "ticker.300",
      std::bind(
          &OvmsVehicleInterfaceTucar::reconfigId,
          this, std::placeholders::_1, std::placeholders::_2));
  reconfigId("", nullptr);
}

bool OvmsVehicleInterfaceTucar::hasImei() const
{
  return m_Imei.hasValue();
}

std::string OvmsVehicleInterfaceTucar::getImei() const
{
  return m_Imei.getValue();
}

void OvmsVehicleInterfaceTucar::setImei(const std::string &imei)
{
  m_Imei.setValue(imei);
}

std::string OvmsVehicleInterfaceTucar::getId() const
{
  if (hasImei())
    return getImei();
  else
    return defaultId;
}

void OvmsVehicleInterfaceTucar::reconfigId(std::string event, void *data)
{
  ESP_LOGI(TAG, "Attempting to reconfig ids");
  auto imeiValue = MyMetrics.Find("m.net.mdm.imei")->AsString();
  if (!imeiValue.empty())
    setImei(imeiValue);

  if (hasImei())
  {
    auto id = getId();
    assert(id != defaultId);
    setIdConfigs(id);
  }
}
void OvmsVehicleInterfaceTucar::reconfigSystem(std::string event, void *data)
{
  if (m_config_ready)
    return;
  int reconfig_counter = 0;
  reconfig_counter += setParamConfigMap(defaultStringParams);
  reconfig_counter += setParamConfigMap(defaultBoolParams);
  reconfig_counter += setParamConfigMap(defaultIntParams);
  reconfig_counter += setParamConfigMap(defaultFLoatParams);
  if (reconfig_counter == 0)
  {
    if (m_reset_by_config)
    {
      ESP_LOGE(TAG, "Reconfigured parameters, restarting.");
      MyBoot.Restart();
    }
    else
    {
      m_config_ready = true;
    }
  }
  else if (reconfig_counter > 0)
  {
    m_reset_by_config = true;
  }
}