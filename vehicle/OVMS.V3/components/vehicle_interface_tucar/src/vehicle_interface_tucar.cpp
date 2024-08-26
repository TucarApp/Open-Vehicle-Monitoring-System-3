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

#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "ovms_config.h"
#include "ovms_peripherals.h"
#include "ovms_log.h"
#include "ovms_metrics.h"

static const char *TAG = "v-iface-tucar";
static const std::string defaultId(15, '0');

namespace
{

template <typename T>
using ParamMap = std::unordered_map<std::string, std::pair<std::string, T>>;

static const ParamMap<std::string> defaultStringParams = {
  {"module", {"init", "done"}},
  {"auto", {"vehicle.type", "MPL60S"}},
  {"auto", {"wifi.mode", "client"}},
  {"log", {"level", "error"}},
  {"vehicle", {"units.accel", "kmphps"}},
  {"vehicle", {"units.accelshort", "mpss"}},
  {"vehicle", {"units.charge", "amphours"}},
  {"vehicle", {"units.consumption", "kmpkwh"}},
  {"vehicle", {"units.distance", "K"}},
  {"vehicle", {"units.distanceshort", "meters"}},
  {"vehicle", {"units.energy", "kwh"}},
  {"vehicle", {"units.power", "kw"}},
  {"vehicle", {"units.pressure", "kpa"}},
  {"vehicle", {"units.ratio", "percent"}},
  {"vehicle", {"units.signal", "dbm"}},
  {"vehicle", {"units.speed", "kmph"}},
  {"vehicle", {"units.temp", "celcius"}}
};

static const ParamMap<bool> defaultBoolParams = {
  {"auto", {"modem", true}},
  {"auto", {"dbc", false}},
  {"auto", {"egpio", true}},
  {"auto", {"ext12v", false}},
  {"auto", {"init", true}},
  {"auto", {"modem", true}},
  {"auto", {"ota", false}},
  {"auto", {"scripting", true}},
  {"auto", {"server.v2", false}},
  {"auto", {"server.v3", true}},
  {"log", {"file.enable", false}},
  {"modem", {"enable.gps", true}},
  {"modem", {"enable.gpstime", true}},
  {"modem", {"enable.net", true}},
  {"modem", {"enable.sms", true}},
  {"vehicle", {"bms.alerts.enabled", false}}
};

static const ParamMap<int> defaultIntParams = {
  {"server.v3", {"updatetime.awake", 1}},
  {"server.v3", {"updatetime.charging", 5}},
  {"server.v3", {"updatetime.connected", 1}},
  {"server.v3", {"updatetime.idle", 60}},
  {"server.v3", {"updatetime.on", 1}},
  {"server.v3", {"updatetime.sendall", 300}}
};

enum class ParamSetResult
{
  Success,
  Fail
};

template <typename T>
ParamSetResult setParamConfig(
  const std::string& moduleKey, const std::string& paramKey, const T& paramValue)
{
  return ParamSetResult::Fail;
}

template <>
ParamSetResult setParamConfig(
  const std::string& moduleKey, const std::string& paramKey, const std::string& paramValue)
{
  MyConfig.SetParamValue(moduleKey, paramKey, paramValue);
  if (MyConfig.GetParamValue(moduleKey, paramKey) == paramValue)
    return ParamSetResult::Success;
  return ParamSetResult::Fail;
}

template <>
ParamSetResult setParamConfig(
  const std::string& moduleKey, const std::string& paramKey, const bool& paramValue)
{
  MyConfig.SetParamValueBool(moduleKey, paramKey, paramValue);
  if (MyConfig.GetParamValueBool(moduleKey, paramKey, !paramValue) == paramValue)
    return ParamSetResult::Success;
  return ParamSetResult::Fail;
}

template <>
ParamSetResult setParamConfig(
  const std::string& moduleKey, const std::string& paramKey, const int& paramValue)
{
  MyConfig.SetParamValueInt(moduleKey, paramKey, paramValue);
  if (MyConfig.GetParamValueInt(moduleKey, paramKey, paramValue+1) == paramValue)
    return ParamSetResult::Success;
  return ParamSetResult::Fail;
}

template <typename T>
void setParamConfigMap(const ParamMap<T>& paramMap)
{
  for (const auto& configElement : paramMap)
  {
    const auto& moduleKey = configElement.first;
    const auto& paramKey = configElement.second.first;
    const auto& paramValue = configElement.second.second;
    
    auto result = setParamConfig(moduleKey, paramKey, paramValue);

    assert(result == ParamSetResult::Success);
  }
}

void setIdConfigs(const std::string& id)
{
  std::string prefix = "ovms/car/";
  prefix.append(id);
  prefix.append("/");

  auto result = ParamSetResult::Fail;
  result = setParamConfig("vehicle", "id", id);
  assert(result == ParamSetResult::Success);
  result = setParamConfig("server.v3", "topic.prefix", prefix);
  assert(result == ParamSetResult::Success);

  // TODO: set param config for wifi.ap, id, password
}

void verifyAndSetIdConfigs(const std::string& id)
{
  if (MyConfig.GetParamValue("vehicle", "id") == id) return;
  setIdConfigs(id);
}

} // namespace


OvmsVehicleInterfaceTucar::OvmsVehicleInterfaceTucar()
{
  ESP_LOGI(TAG, "Initialising Tucar Vehicle Interface");

  setParamConfigMap(defaultStringParams);
  setParamConfigMap(defaultBoolParams);
  setParamConfigMap(defaultIntParams);

  /* Set imei once modem has received it. */
  MyEvents.RegisterEvent(
    TAG, "system.modem.received.imei",
    std::bind(
      &OvmsVehicleInterfaceTucar::modemReceivedImei,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
  
  /* Set imei if it has already been registered. */
  auto imeiValue = MyMetrics.Find("m.net.mdm.imei")->AsString();
  if (!imeiValue.empty()) setImei(imeiValue);

  if (hasImei())
  {
    auto id = getId();
    assert(id != defaultId);
    verifyAndSetIdConfigs(id);
  }
}

bool OvmsVehicleInterfaceTucar::hasImei() const
{
  return mImei.hasValue();
}

std::string OvmsVehicleInterfaceTucar::getImei() const
{
  return mImei.getValue();
}

void OvmsVehicleInterfaceTucar::setImei(const std::string& imei)
{
  mImei.setValue(imei);
}

std::string OvmsVehicleInterfaceTucar::getId() const
{
  if (hasImei()) return getImei();
  else return defaultId;
}

void OvmsVehicleInterfaceTucar::modemReceivedImei(std::string event, void* data)
{
  ESP_LOGI(TAG, "Received IMEI from modem");
  auto imeiValue = MyMetrics.Find("m.net.mdm.imei")->AsString();
  if (!imeiValue.empty()) setImei(imeiValue);

  assert(hasImei());

  auto id = getId();
  assert(id != defaultId);
  verifyAndSetIdConfigs(id);
}
