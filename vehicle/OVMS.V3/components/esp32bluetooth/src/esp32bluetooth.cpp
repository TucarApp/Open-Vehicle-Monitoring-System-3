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

#include <string.h>
#include "esp_system.h"
#include "esp32bluetooth.h"
#include "esp32bluetooth_gap.h"
#include "esp32bluetooth_gatts.h"
#include "esp_bt.h"
#include "ovms_peripherals.h"
#include "ovms_config.h"

#include "ovms_log.h"
static const char *TAG = "bt";

esp32bluetooth::esp32bluetooth(const char* name)
  : pcp(name)
  {
  m_service_running = false;
  m_powermode = Off;
  }

esp32bluetooth::~esp32bluetooth()
  {
  }

void esp32bluetooth::StartService()
  {
  esp_err_t ret;

  if (m_service_running)
    {
    ESP_LOGE(TAG,"Bluetooth service cannot start (already running)");
    return;
    }

  ESP_LOGI(TAG,"Powering bluetooth on...");

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret)
    {
    ESP_LOGE(TAG, "init controller failed: %s", esp_err_to_name(ret));
    return;
    }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret)
    {
    ESP_LOGE(TAG, "enable controller failed: %s", esp_err_to_name(ret));
    return;
    }

  ret = esp_bluedroid_init();
  if (ret)
    {
    ESP_LOGE(TAG, "init bluetooth failed: %s", esp_err_to_name(ret));
    return;
    }

  ret = esp_bluedroid_enable();
  if (ret)
    {
    ESP_LOGE(TAG, "enable bluetooth failed: %s", esp_err_to_name(ret));
    return;
    }

  MyBluetoothGATTS.RegisterForEvents();
  MyBluetoothGAP.RegisterForEvents();
  MyBluetoothGATTS.RegisterAllApps();

  m_service_running = true;
  }
 

void esp32bluetooth::StopService()
  {
  esp_err_t ret;

  if (!m_service_running)
    {
    ESP_LOGE(TAG,"Bluetooth service cannot stop (not running)");
    return;
    }

  ESP_LOGI(TAG,"Powering bluetooth off...");

  MyBluetoothGATTS.UnregisterAllApps();

  ret = esp_bluedroid_disable();
  if (ret)
    {
    ESP_LOGE(TAG, "disable bluetooth failed: %s", esp_err_to_name(ret));
    return;
    }

  ret = esp_bluedroid_deinit();
  if (ret)
    {
    ESP_LOGE(TAG, "deinit bluetooth failed: %s", esp_err_to_name(ret));
    return;
    }

  ret = esp_bt_controller_disable();
  if (ret)
    {
    ESP_LOGE(TAG, "disable controller failed: %s", esp_err_to_name(ret));
    return;
    }

  ret = esp_bt_controller_deinit();
  if (ret)
    {
    ESP_LOGE(TAG, "deinit controller failed: %s", esp_err_to_name(ret));
    return;
    }

  m_service_running = false;
  }

bool esp32bluetooth::IsServiceRunning()
  {
  return m_service_running;
  }

void esp32bluetooth::SetPowerMode(PowerMode powermode)
  {
  m_powermode = powermode;
  switch (powermode)
    {
    case On:
      if (!m_service_running) StartService();
      break;
    case Sleep:
    case DeepSleep:
    case Off:
      if (m_service_running) StopService();
      break;
    default:
      break;
    };
  }

void bluetooth_status(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  esp32bluetooth *me = MyPeripherals->m_esp32bluetooth;

  if (!me->IsServiceRunning())
    {
    writer->puts("Bluetooth service is not running");
    return;
    }

  int nclients = esp_ble_get_bond_device_num();
  esp_ble_bond_dev_t *clist = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * nclients);
  esp_ble_get_bond_device_list(&nclients, clist);
  writer->printf("Bluetooth service is running (%d client(s) registered)\n",nclients);
  for (int k = 0; k < nclients; k++)
    {
    writer->printf("  %02x:%02x:%02x:%02x:%02x:%02x\n",
      clist[k].bd_addr[0],
      clist[k].bd_addr[1],
      clist[k].bd_addr[2],
      clist[k].bd_addr[3],
      clist[k].bd_addr[4],
      clist[k].bd_addr[5]);
    }

  free(clist);
  }

void bluetooth_clear(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  esp32bluetooth *me = MyPeripherals->m_esp32bluetooth;

  if (!me->IsServiceRunning())
    {
    writer->puts("Error: Bluetooth service is not running");
    return;
    }

  int nclients = esp_ble_get_bond_device_num();
  esp_ble_bond_dev_t *clist = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * nclients);
  esp_ble_get_bond_device_list(&nclients, clist);
  for (int k = 0; k < nclients; k++)
    {
    char bta[19];
    sprintf(bta,"%02x:%02x:%02x:%02x:%02x:%02x",
      clist[k].bd_addr[0],
      clist[k].bd_addr[1],
      clist[k].bd_addr[2],
      clist[k].bd_addr[3],
      clist[k].bd_addr[4],
      clist[k].bd_addr[5]);
    if ((argc==0)||(strcasecmp(bta,argv[0])==0))
      {
      writer->printf("Removing Registration: %s\n",bta);
      esp_ble_remove_bond_device(clist[k].bd_addr);
      }
    }

  free(clist);
  }

class esp32bluetoothInit
  {
  public: esp32bluetoothInit();
  } esp32bluetoothInit  __attribute__ ((init_priority (8010)));

esp32bluetoothInit::esp32bluetoothInit()
  {
  ESP_LOGI(TAG, "Initialising ESP32 BLUETOOTH (8010)");

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  OvmsCommand* cmd_bt = MyCommandApp.RegisterCommand("bt","BLUETOOTH framework", bluetooth_status);
  cmd_bt->RegisterCommand("status","Show bluetooth status",bluetooth_status);
  cmd_bt->RegisterCommand("clear","Clear bluetooth registrations",bluetooth_clear, "(<mac>)", 0, 1);
  }
