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

#include "ovms_log.h"
#include "buffered_shell.h"

static const char *TAG = "bt-tucar-app";

const std::string::size_type BluetoothCommandResponseBuffer::MAX_CHUNK_SIZE = 16;
const std::string::size_type BluetoothCommandResponseBuffer::MAX_CHUNK_COUNT = 40;
const std::string::size_type BluetoothCommandResponseBuffer::MAX_RESPONSE_SIZE =
  MAX_CHUNK_SIZE * MAX_CHUNK_COUNT;
const std::string BluetoothCommandResponseBuffer::AUTH_CMD = "auth";

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
}

void OvmsBluetoothTucarApp::EventCreate(
  esp_ble_gatts_cb_param_t::gatts_add_attr_tab_evt_param *attrtab)
{
  ESP_LOGI(TAG, "EventCreate");
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
}

void OvmsBluetoothTucarApp::EventAddChar(
  esp_ble_gatts_cb_param_t::gatts_add_char_evt_param *addchar)
{
  ESP_LOGI(TAG, "EventAddChar");
}

void OvmsBluetoothTucarApp::EventAddCharDescr(
  esp_ble_gatts_cb_param_t::gatts_add_char_descr_evt_param *adddescr)
{
  ESP_LOGI(TAG, "EventAddCharDescr");
}

void OvmsBluetoothTucarApp::EventRead(
  esp_ble_gatts_cb_param_t::gatts_read_evt_param *read)
{
  ESP_LOGI(TAG, "EventRead");
}

void OvmsBluetoothTucarApp::EventWrite(
  esp_ble_gatts_cb_param_t::gatts_write_evt_param *write)
{
  ESP_LOGI(TAG, "EventWrite");
}

void OvmsBluetoothTucarApp::EventExecWrite(
  esp_ble_gatts_cb_param_t::gatts_exec_write_evt_param *execwrite)
{
  ESP_LOGI(TAG, "EventExecWrite");
}

