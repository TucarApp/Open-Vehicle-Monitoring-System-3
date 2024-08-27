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

#ifndef __ESP32BLUETOOTH_APP_TUCAR_H__
#define __ESP32BLUETOOTH_APP_TUCAR_H__

#include <queue>

#include "esp32bluetooth.h"
#include "esp32bluetooth_gatts.h"

class BluetoothCommandResponseBuffer
{
private:
  static const std::string::size_type MAX_CHUNK_SIZE;
  static const std::string::size_type MAX_CHUNK_COUNT;
  static const std::string::size_type MAX_RESPONSE_SIZE;
  static const std::string AUTH_CMD;

public:
  BluetoothCommandResponseBuffer(
    std::vector<std::string>&& accepted_command_set);
  void runCommand(std::string&& command);
  std::string fetchNextResponseChunk();
  bool hasMoreResponseChunks() const;
  const std::string& getCommand() const;
  void clearResponseChunks();

  void resetPasscode(const std::string& passcode);
  void closeAuthenticatedSession();
  bool isAuthenticated() const;

private:
  void executeCommand();

  void startAuthenticatedSession(const std::string& command);

  std::string m_command;
  std::queue<std::string> m_command_response_chunks;
  const std::vector<std::string> m_accepted_command_set;

  std::string m_passcode;
  bool m_authenticated;
};

class OvmsBluetoothTucarApp : public esp32bluetoothApp
{
public:
  OvmsBluetoothTucarApp();

public:
  void EventRegistered(esp_ble_gatts_cb_param_t::gatts_reg_evt_param *reg) override;
  void EventCreate(esp_ble_gatts_cb_param_t::gatts_add_attr_tab_evt_param *attrtab) override;
  void EventConnect(esp_ble_gatts_cb_param_t::gatts_connect_evt_param *connect) override;
  void EventDisconnect(esp_ble_gatts_cb_param_t::gatts_disconnect_evt_param *disconnect) override;
  void EventAddChar(esp_ble_gatts_cb_param_t::gatts_add_char_evt_param *addchar) override;
  void EventAddCharDescr(esp_ble_gatts_cb_param_t::gatts_add_char_descr_evt_param *adddescr) override;
  void EventRead(esp_ble_gatts_cb_param_t::gatts_read_evt_param *read) override;
  void EventWrite(esp_ble_gatts_cb_param_t::gatts_write_evt_param *write) override;
  void EventExecWrite(esp_ble_gatts_cb_param_t::gatts_exec_write_evt_param *execwrite) override;

private:
};

#endif //#ifndef __ESP32BLUETOOTH_APP_TUCAR_H__
