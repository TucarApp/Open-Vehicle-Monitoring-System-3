/*
;    Project:       Open Vehicle Monitor System
;    Date:          14th March 2017
;
;    Changes:
;    1.0  Initial release
;
;    (C) 2011       Michael Stegen / Stegen Electronics
;    (C) 2011-2017  Mark Webb-Johnson
;    (C) 2011        Sonny Chen @ EPRO/DX
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

const std::string::size_type BluetoothCommandResponseBuffer::MAX_CHUNK_SIZE = 16;
const std::string::size_type BluetoothCommandResponseBuffer::MAX_CHUNK_COUNT = 40;
const std::string::size_type BluetoothCommandResponseBuffer::MAX_RESPONSE_SIZE =
  MAX_CHUNK_SIZE * MAX_CHUNK_COUNT;
const std::string BluetoothCommandResponseBuffer::AUTH_CMD = "auth";

const size_t OvmsBluetoothTucarApp::UUID_LEN = ESP_UUID_LEN_16;
const uint16_t OvmsBluetoothTucarApp::SERVICE_UUID = 0xffd0;
const uint16_t OvmsBluetoothTucarApp::CHAR_UUID = 0xffd1;
static_assert(OvmsBluetoothTucarApp::UUID_LEN == ESP_UUID_LEN_16,
  "If UUID_LEN is not 16, the UUID type must be changed to match the length.");
const size_t OvmsBluetoothTucarApp::NUM_HANDLE = 4;

static uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// adv params
static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr          = {0, 0, 0, 0, 0, 0},
    .peer_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static uint8_t adv_config_done = 0;
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

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
}

void BluetoothCommandResponseBuffer::closeAuthenticatedSession()
{
  m_authenticated = false;
}

OvmsBluetoothTucarApp::OvmsBluetoothTucarApp() :
  esp32bluetoothApp("tucar-app")
{
  ESP_LOGI(TAG, "Created OvmsBluetoothTucarApp");
  MyBluetoothGATTS.RegisterApp(this);
}

void OvmsBluetoothTucarApp::EventRegistered(
  esp_ble_gatts_cb_param_t::gatts_reg_evt_param *reg)
{
  ESP_LOGI(TAG, "EventRegistered");
  m_service_id.is_primary = true;
  m_service_id.id.inst_id = 0x00;
  m_service_id.id.uuid.len = UUID_LEN;
  m_service_id.id.uuid.uuid.uuid16 = SERVICE_UUID;

  auto ret = esp_ble_gap_config_adv_data(&adv_data);
  if (ret) ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);

  adv_config_done |= adv_config_flag;
  ret = esp_ble_gap_config_adv_data(&scan_rsp_data);

  adv_config_done |= scan_rsp_config_flag;

  ret = esp_ble_gatts_create_service(m_gatts_if, &m_service_id, NUM_HANDLE);
  if (ret) ESP_LOGE(TAG, "esp_ble_gatts_create_service failed, error code = %x", ret);
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

  // esp_ble_conn_update_params_t conn_params = {0};
  // memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
  // /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
  // conn_params.latency = 0;
  // conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
  // conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
  // conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
  // ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
  //   param->connect.conn_id,
  //   param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
  //   param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
  
  // if (gatts_profile_handle_table.find(gatts_profile_0_id) == gatts_profile_handle_table.end()) {
  //   ESP_LOGE(TAG, "profile 0 not found");
  //   break;
  // }
  // gatts_profile_handle_table.at(gatts_profile_0_id).conn_id = param->connect.conn_id;
  // //start sent the update connection parameters to the peer device.
  // auto ret = esp_ble_gap_update_conn_params(&conn_params);
  // if (ret) ESP_LOGE(TAG, "start send update connection parameters failed, error code = %x", ret);

  // // /* start security connect with peer device when receive the connect event sent by the master. */
  // // auto ret = esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
  // // if (ret) ESP_LOGE(TAG, "set encryption failed, error code = %x", ret);
}

void OvmsBluetoothTucarApp::EventDisconnect(
  esp_ble_gatts_cb_param_t::gatts_disconnect_evt_param *disconnect)
{
  ESP_LOGI(TAG, "EventDisconnect");

  auto ret = esp_ble_gap_start_advertising(&adv_params);
  if (ret) ESP_LOGE(TAG, "start adv failed, error code = %x", ret);
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
  
  // auto ret = esp_ble_gatts_add_char_descr(
  //   m_service_handle,
  //   &m_descr_uuid,
  //   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
  //   NULL,
  //   NULL);
  
  // if (ret)
  //   ESP_LOGE(TAG, "add char descr failed, error code = %x", ret);
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

  auto rsp_hex = string_to_hex_buffer("Hello, World!");

  esp_gatt_rsp_t rsp;
  memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
  rsp.attr_value.handle = read->handle;
  rsp.attr_value.len = rsp_hex.size();

  size_t index = 0;
  for (const auto& byte : rsp_hex)
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
