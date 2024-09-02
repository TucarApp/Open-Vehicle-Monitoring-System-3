/*
;    Project:       Open Vehicle Monitor System
;    Date:          27th August 2024
;
;    Changes:
;    1.0  Initial release
;
;    (C) 2024      Jaime Middleton / Tucar
;    (C) 2024      Axel Troncoso   / Tucar
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

#include "esp32bluetooth_app_tucar.h"

#include <queue>
#include <regex>
#include <string>
#include <vector>

#include "esp_bt.h"
#include "esp_gatt_common_api.h"

#include "ovms_log.h"
#include "buffered_shell.h"

static const char *TAG = "bt-tucar-app";

#define DEFAULT_BLE_APP_PSWD "tucar987"

const std::string::size_type BluetoothCommandResponseBuffer::MAX_CHUNK_SIZE = 16;
const std::string::size_type BluetoothCommandResponseBuffer::MAX_CHUNK_COUNT = 40;
const std::string::size_type BluetoothCommandResponseBuffer::MAX_RESPONSE_SIZE =
  MAX_CHUNK_SIZE * MAX_CHUNK_COUNT;
const std::string BluetoothCommandResponseBuffer::AUTH_CMD = "auth";

#ifdef CONFIG_OVMS_TUCAR_BLE_APP_COMMAND_PSWD
static const std::string TUCAR_APP_BLE_PSWD = CONFIG_OVMS_TUCAR_BLE_APP_COMMAND_PSWD;
#ifdef CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
static_assert(strcmp(CONFIG_OVMS_TUCAR_BLE_APP_COMMAND_PSWD, DEFAULT_BLE_APP_PSWD),
  "Invalid BLE password. See Menuconfig: OVMS->Tucar Configurations");
#endif // CONFIG_OVMS_TUCAR_BUILD_TYPE_PROD
#else
static const std::string TUCAR_APP_BLE_PSWD = DEFAULT_BLE_APP_PSWD;
#endif // CONFIG_OVMS_TUCAR_BLE_APP_COMMAND_PSWD

const size_t OvmsBluetoothTucarApp::UUID_LEN = ESP_UUID_LEN_16;
const uint16_t OvmsBluetoothTucarApp::SERVICE_UUID = 0xffd0;
const uint16_t OvmsBluetoothTucarApp::CHAR_UUID = 0xffd1;
static_assert(OvmsBluetoothTucarApp::UUID_LEN == ESP_UUID_LEN_16,
  "If UUID_LEN is not 16, the UUID type must be changed to match the length.");
const size_t OvmsBluetoothTucarApp::NUM_HANDLE = 4;


namespace 
{

std::string hex_buffer_to_string(const uint8_t* buffer, uint16_t length)
{
  std::string result;
  for (size_t i = 0; i < length; ++i) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%c", buffer[i]);
    result += hex;
  }
  return result;
}

std::vector<uint8_t> string_to_hex_buffer(const std::string& str)
{
  std::vector<uint8_t> result;
  for (size_t i = 0; i < str.size(); ++i) {
    result.push_back(static_cast<uint8_t>(str[i]));
  }
  return result;
}

} // namespace

OvmsBluetoothTucarApp MyBluetoothTucarApp __attribute__ ((init_priority (8016)));


BluetoothCommandResponseBuffer::BluetoothCommandResponseBuffer(std::vector<std::string>&& accepted_command_set) :
  m_accepted_command_set(std::move(accepted_command_set))
{
}

void BluetoothCommandResponseBuffer::runCommand(std::string&& command)
{
  if (!isAuthenticated())
  {
    startAuthenticatedSession(command);
    return;
  }
  m_command = std::move(command);
  executeCommand();
}

void BluetoothCommandResponseBuffer::executeCommand()
{
  bool acceptable_command = false;
  for (const auto& pattern : m_accepted_command_set)
  {
    std::regex command_pattern(pattern);
    if (std::regex_match(m_command, command_pattern))
    {
      acceptable_command = true;
      break;
    }
  }
 
  std::string response = "Command not accepted.";
  if (acceptable_command)
    response = BufferedShell::ExecuteCommand(m_command, true);

  // Clear the response chunks.
  clearResponseChunks();

  // Split the response into chunks.
  std::string::size_type index = 0;
  while (index < response.size() && 
      m_command_response_chunks.size() < MAX_CHUNK_COUNT)
  {
    m_command_response_chunks.push(
      response.substr(index, MAX_CHUNK_SIZE));
    index += MAX_CHUNK_SIZE;
  }
}

void BluetoothCommandResponseBuffer::clearResponseChunks()
{
  m_command_response_chunks = std::queue<std::string>();
}

bool BluetoothCommandResponseBuffer::hasMoreResponseChunks() const
{
  return !m_command_response_chunks.empty();
}

std::string BluetoothCommandResponseBuffer::fetchNextResponseChunk()
{
  assert(hasMoreResponseChunks());
  auto chunk = m_command_response_chunks.front();
  m_command_response_chunks.pop();
  return chunk;
}

const std::string& BluetoothCommandResponseBuffer::getCommand() const
{
  return m_command;
}

void BluetoothCommandResponseBuffer::resetPasscode(const std::string& passcode)
{
  m_passcode = passcode;
}

bool BluetoothCommandResponseBuffer::isAuthenticated() const
{
  return m_authenticated;
}

void BluetoothCommandResponseBuffer::startAuthenticatedSession(
  const std::string& command)
{
  if (command == AUTH_CMD + " " + m_passcode)
  {
    m_authenticated = true;
    clearResponseChunks();
    m_command_response_chunks.push("ok");
  }
  else
  {
    clearResponseChunks();
    m_command_response_chunks.push("i");
  }
}

void BluetoothCommandResponseBuffer::closeAuthenticatedSession()
{
  m_authenticated = false;
}

OvmsBluetoothTucarApp::OvmsBluetoothTucarApp() :
  esp32bluetoothApp("tucar-app"),
  m_char_uuid(),
  m_descr_uuid(),
  m_descr_handle(0),
  m_char_handle(0),
  m_command_response_buffer({
    "^lock\\s.*", "^unlock\\s.*", "^module\\s.*", "^bt\\s.*"
  })
{
  ESP_LOGI(TAG, "Created OvmsBluetoothTucarApp");
#ifdef CONFIG_OVMS_TUCAR_BLE_APP
  MyBluetoothGATTS.RegisterApp(this);
#endif // CONFIG_OVMS_TUCAR_BLE_APP
}

void OvmsBluetoothTucarApp::EventRegistered(
  esp_ble_gatts_cb_param_t::gatts_reg_evt_param *reg)
{
  ESP_LOGI(TAG, "EventRegistered");

  m_service_id.is_primary = true;
  m_service_id.id.inst_id = 0x00;
  m_service_id.id.uuid.len = UUID_LEN;
  m_service_id.id.uuid.uuid.uuid16 = SERVICE_UUID;

  auto ret = esp_ble_gatts_create_service(m_gatts_if, &m_service_id, NUM_HANDLE);
  if (ret) ESP_LOGE(TAG, "esp_ble_gatts_create_service failed, error code = %x", ret);

  m_command_response_buffer.resetPasscode(TUCAR_APP_BLE_PSWD);
}

void OvmsBluetoothTucarApp::EventCreate(
  esp_ble_gatts_cb_param_t::gatts_create_evt_param *attrtab)
{
  ESP_LOGI(TAG, "EventCreate");
  m_char_uuid.len = ESP_UUID_LEN_16;
  m_char_uuid.uuid.uuid16 = CHAR_UUID;

  m_service_handle = attrtab->service_handle;

  auto ret = esp_ble_gatts_start_service(m_service_handle);
  if (ret) ESP_LOGE(TAG, "esp_ble_gatts_start_service failed, error code = %x", ret);
}

void OvmsBluetoothTucarApp::EventStart(
  esp_ble_gatts_cb_param_t::gatts_start_evt_param *connect)
{
  ESP_LOGI(TAG, "EventStart");

  auto ret = esp_ble_gatts_add_char(
    m_service_handle,
    &m_char_uuid,
    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
    nullptr,
    nullptr);
  if (ret) ESP_LOGE(TAG, "esp_ble_gatts_add_char failed, error code = %x", ret);
}

void OvmsBluetoothTucarApp::EventConnect(
  esp_ble_gatts_cb_param_t::gatts_connect_evt_param *connect)
{
  ESP_LOGI(TAG, "EventConnect");
}

void OvmsBluetoothTucarApp::EventDisconnect(
  esp_ble_gatts_cb_param_t::gatts_disconnect_evt_param *disconnect)
{
  ESP_LOGI(TAG, "EventDisconnect");

  m_command_response_buffer.clearResponseChunks();
  m_command_response_buffer.closeAuthenticatedSession();
}

void OvmsBluetoothTucarApp::EventAddChar(
  esp_ble_gatts_cb_param_t::gatts_add_char_evt_param *addchar)
{
  ESP_LOGI(TAG, "EventAddCharacteristic");

  m_char_handle = addchar->attr_handle;
  m_descr_uuid.len = UUID_LEN;
  m_descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

  uint16_t length = 0;
  const uint8_t *value;

  auto ret_status = esp_ble_gatts_get_attr_value(
    m_char_handle, &length, &value);
  
  if (ret_status == ESP_FAIL)
    ESP_LOGE(TAG, "get attr value failed, error code = %x", ret_status);
}

void OvmsBluetoothTucarApp::EventAddCharDescr(
  esp_ble_gatts_cb_param_t::gatts_add_char_descr_evt_param *adddescr)
{
  ESP_LOGI(TAG, "EventAddCharDescr");

  m_descr_handle = adddescr->attr_handle;
}

void OvmsBluetoothTucarApp::EventRead(
  esp_ble_gatts_cb_param_t::gatts_read_evt_param *read)
{
  ESP_LOGI(TAG, "EventRead");

  std::string response_chunk = "empty";
  if (m_command_response_buffer.hasMoreResponseChunks())
    response_chunk = m_command_response_buffer.fetchNextResponseChunk();
  
  auto response_in_hex = string_to_hex_buffer(response_chunk);

  esp_gatt_rsp_t rsp;
  memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
  rsp.attr_value.handle = read->handle;
  rsp.attr_value.len = response_in_hex.size();

  size_t index = 0;
  for (const auto& byte : response_in_hex)
  {
    rsp.attr_value.value[index++] = byte;
  }

  auto ret = esp_ble_gatts_send_response(
    m_gatts_if, read->conn_id, read->trans_id, ESP_GATT_OK, &rsp);
  
  if (ret) ESP_LOGE(TAG, "send response failed, error code = %x", ret);
}

void OvmsBluetoothTucarApp::EventWrite(
  esp_ble_gatts_cb_param_t::gatts_write_evt_param *write)
{
  ESP_LOGI(TAG, "EventWrite");
  if (!write->is_prep)
  {
    esp_log_buffer_hex(TAG, write->value, write->len);
    esp_log_buffer_char(TAG, write->value, write->len);

    m_command_response_buffer.runCommand(
      hex_buffer_to_string(write->value, write->len));

    ESP_LOGI(TAG, "Value written: %s",
      hex_buffer_to_string(write->value, write->len).c_str());
  }
  else ESP_LOGI(TAG, "write is_prep");

  esp_ble_gatts_send_response(
    m_gatts_if, write->conn_id, write->trans_id, ESP_GATT_OK, NULL);
}

void OvmsBluetoothTucarApp::EventExecWrite(
  esp_ble_gatts_cb_param_t::gatts_exec_write_evt_param *execwrite)
{
  ESP_LOGI(TAG, "EventExecWrite");
}
