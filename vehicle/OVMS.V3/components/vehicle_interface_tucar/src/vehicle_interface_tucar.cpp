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

#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "ovms_config.h"
#include "ovms_log.h"

static const char *TAG = "v-iface-tucar";

namespace
{

template <typename T>
using ParamMap = std::unordered_map<std::string, std::pair<std::string, T>>;

static const ParamMap<std::string> defaultStringParams = {
  {"module", {"init", "done"}},
  {"password", {"module", "abc123"}},
  {"modem", {"apn", "help.help.cl"}},
};

static const ParamMap<bool> defaultBoolParams = {
  {"auto", {"modem", true}},
};

static const ParamMap<int> defaultIntParams = {
  {"server.v3", {"updatetime.awake", 1}},
};

enum class ParamSetResult
{
  Success,
  Fail
};

std::string getImei()
{
  return "000000000000001";
}

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

} // namespace


OvmsVehicleInterfaceTucar::OvmsVehicleInterfaceTucar() :
  imei(getImei())
{
  ESP_LOGI(TAG, "Initialising Tucar Vehicle Interface");

  setParamConfigMap(defaultStringParams);
  setParamConfigMap(defaultBoolParams);
  setParamConfigMap(defaultIntParams);
}

void OvmsVehicleInterfaceTucar::Ticker1(uint32_t ticker)
{
  ESP_LOGI(TAG, "Ticker1: %d", ticker);

  setParamConfigMap(defaultStringParams);
}
