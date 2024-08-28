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

#ifndef __ESP32BLUETOOTH_SVC_DEVICE_H__
#define __ESP32BLUETOOTH_SVC_DEVICE_H__

#include "esp32bluetooth.h"
#include "esp32bluetooth_gatts.h"

#define GATTS_APP_UUID_OVMS_DEVICE       0x10
#define GATTS_SERVICE_UUID_OVMS_DEVICE   0x1041
#define GATTS_CHAR_UUID_OVMS_DEVICE      0x1042
#define GATTS_DESCR_UUID_OVMS_DEVICE     0x1043
#define GATTS_NUM_HANDLE_OVMS_DEVICE     4

class OvmsBluetoothAppDevice : public esp32bluetoothApp
  {
  public:
    OvmsBluetoothAppDevice();
    ~OvmsBluetoothAppDevice();

  public:
    void EventRegistered(esp_ble_gatts_cb_param_t::gatts_reg_evt_param *reg);
    void EventRead(esp_ble_gatts_cb_param_t::gatts_read_evt_param *read);
    void EventCreate(esp_ble_gatts_cb_param_t::gatts_create_evt_param *attrtab);
    void EventAddChar(esp_ble_gatts_cb_param_t::gatts_add_char_evt_param *addchar);

    void EventWrite(esp_ble_gatts_cb_param_t::gatts_write_evt_param *write) {}
    void EventExecWrite(esp_ble_gatts_cb_param_t::gatts_exec_write_evt_param *execwrite) {}
    void EventMTU(esp_ble_gatts_cb_param_t::gatts_mtu_evt_param *mtu) {}
    void EventConf(esp_ble_gatts_cb_param_t::gatts_conf_evt_param *conf) {}
    void EventUnregistered() {}
    void EventDelete(esp_ble_gatts_cb_param_t::gatts_delete_evt_param *del) {}
    void EventStart(esp_ble_gatts_cb_param_t::gatts_start_evt_param *start) {}
    void EventStop(esp_ble_gatts_cb_param_t::gatts_stop_evt_param *stop) {}
    void EventConnect(esp_ble_gatts_cb_param_t::gatts_connect_evt_param *connect) {}
    void EventDisconnect(esp_ble_gatts_cb_param_t::gatts_disconnect_evt_param *disconnect) {}
    void EventOpen(esp_ble_gatts_cb_param_t::gatts_open_evt_param *open) {}
    void EventCancelOpen(esp_ble_gatts_cb_param_t::gatts_cancel_open_evt_param *cancelopen) {}
    void EventClose(esp_ble_gatts_cb_param_t::gatts_close_evt_param *close) {}
    void EventListen() {}
    void EventCongest(esp_ble_gatts_cb_param_t::gatts_congest_evt_param *congest) {}
    void EventAddCharDescr(esp_ble_gatts_cb_param_t::gatts_add_char_descr_evt_param *adddescr) {}

  private:
    uint16_t m_char_handle;
    esp_bt_uuid_t m_char_uuid;
    esp_gatt_perm_t m_perm;
    esp_gatt_char_prop_t m_property;
    uint16_t m_descr_handle;
    esp_bt_uuid_t m_descr_uuid;
  };

#endif //#ifndef __ESP32BLUETOOTH_SVC_DEVICE_H__
