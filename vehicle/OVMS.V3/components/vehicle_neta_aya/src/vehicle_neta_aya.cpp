/*
;    Project:       Open Vehicle Monitor System
;    Date:          13th August 2024
;
;    Changes:
;    1.0  Initial stub
;
;    (C) 2024        Jaime Middleton / Tucar
;    (C) 2024        Axel Troncoso / Tucar
;
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

#include "vehicle_neta_aya.h"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "metrics_standard.h"
#include "ovms_events.h"
#include "ovms_log.h"
#include "ovms_metrics.h"
#include "ovms_peripherals.h"
#include "pcp.h"

#define VERSION "0.0.1"

static const char *TAG = "v-netaAya";


#define POLLSTATE_OFF           PollSetState(0);
#define POLLSTATE_RUNNING       PollSetState(1);
#define POLLSTATE_CHARGING      PollSetState(2);

// CAN buffer access macros: b=byte# 0..7 / n=nibble# 0..15
#define CAN_BYTE(b)     data[b]
#define CAN_INT(b)      ((int16_t)CAN_UINT(b))
#define CAN_UINT(b)     (((UINT)CAN_BYTE(b) << 8) | CAN_BYTE(b+1))
#define CAN_UINT24(b)   (((uint32_t)CAN_BYTE(b) << 16) | ((UINT)CAN_BYTE(b+1) << 8) | CAN_BYTE(b+2))
#define CAN_UINT32(b)   (((uint32_t)CAN_BYTE(b) << 24) | ((uint32_t)CAN_BYTE(b+1) << 16)  | ((UINT)CAN_BYTE(b+2) << 8) | CAN_BYTE(b+3))
#define CAN_NIBL(b)     (can_databuffer[b] & 0x0f)
#define CAN_NIBH(b)     (can_databuffer[b] >> 4)
#define CAN_NIB(n)      (((n)&1) ? CAN_NIBL((n)>>1) : CAN_NIBH((n)>>1))
#define CAN_BIT(b,pos) !!(data[b] & (1<<(pos)))

#define TO_CELCIUS(n)  ((float)n-40)
#define TO_PSI(n)    ((float)n/4.0)

// Pollstate 0 - car is off
// Pollstate 1 - car is on
// Pollstate 2 - car is charging
static const OvmsPoller::poll_pid_t vehicle_neta_polls[] =
  {
    // speed
    {0x7e2, 0x7ea, VEHICLE_POLL_TYPE_READDATA, 0xb100, {1, 1, 1}, 2, ISOTP_STD},
    // soc
    {0x7e2, 0x7ea, VEHICLE_POLL_TYPE_READDATA, 0xf015, {1, 1, 1}, 2, ISOTP_STD},
    // odometer
    {0x7e2, 0x7ea, VEHICLE_POLL_TYPE_READDATA, 0xe101, {1, 1, 1}, 2, ISOTP_STD},
    // current
    {0x7e2, 0x7ea, VEHICLE_POLL_TYPE_READDATA, 0xf013, {1, 1, 1}, 2, ISOTP_STD},
    // voltage
    {0x7e2, 0x7ea, VEHICLE_POLL_TYPE_READDATA, 0xf012, {1, 1, 1}, 2, ISOTP_STD},
    // on
    {0x7e2, 0x7ea, VEHICLE_POLL_TYPE_READDATA, 0xd001, {1, 1, 1}, 2, ISOTP_STD},
    // evse code cahrging ??
    {0x708, 0x718, VEHICLE_POLL_TYPE_READDATA, 0xf012, {1, 1, 1}, 2, ISOTP_STD},
    POLL_LIST_END};
/**
 * Constructor for Kia Niro EV OvmsVehicleNetaAya
 */
OvmsVehicleNetaAya::OvmsVehicleNetaAya()
{
  ESP_LOGI(TAG, "Neta Aya vehicle module");

  StdMetrics.ms_v_charge_inprogress->SetValue(false);
  StdMetrics.ms_v_env_on->SetValue(false);

  // Require GPS.
  MyEvents.SignalEvent("vehicle.require.gps", NULL);
  MyEvents.SignalEvent("vehicle.require.gpstime", NULL);

  PollSetThrottling(10);
  RegisterCanBus(1, CAN_MODE_LISTEN, CAN_SPEED_500KBPS);
  RegisterCanBus(2, CAN_MODE_ACTIVE, CAN_SPEED_500KBPS);

  POLLSTATE_OFF;
  PollSetPidList(m_can2, vehicle_neta_polls);
  auto result = setParamConfig("auto", "vehicle.type", "NTA");
  if (result != ParamSetResult::Fail)
  {
    ESP_LOGE(TAG, "Error setting vehicle.type");
  }
}

/**
 * Destructor
 */
OvmsVehicleNetaAya::~OvmsVehicleNetaAya()
{
  ESP_LOGI(TAG, "Shutdown Neta Aya vehicle module");
}

/**
 * Ticker1: Called every second
 */
void OvmsVehicleNetaAya::Ticker1(uint32_t ticker)
{

  StdMetrics.ms_v_bat_power->SetValue(
    StdMetrics.ms_v_bat_voltage->AsFloat(400, Volts) *
      StdMetrics.ms_v_bat_current->AsFloat(1, Amps) / 1000,
    kW);

  auto vehicle_on = static_cast<bool>(
    StdMetrics.ms_v_env_on->AsBool());
  auto vehicle_charging = static_cast<bool>(
    StdMetrics.ms_v_charge_inprogress->AsBool());
  
  auto to_run  = vehicle_on && !vehicle_charging;
  auto to_charge = vehicle_charging;
  auto to_off = !vehicle_on && !vehicle_charging;

  /* One and only one of these transitions must be true. */
  assert(to_run + to_charge + to_off == 1);

  auto poll_state = GetPollState();

  switch (poll_state)
  {
    case PollState::OFF:
      if (to_run) HandleCarOn();
      else if (to_charge) HandleCharging();
      break;
    case PollState::RUNNING:
      if (to_off) HandleCarOff();
      else if (to_charge) HandleCharging();
      break;
    case PollState::CHARGING:
      if (to_off)
        {
        HandleChargeStop();
        HandleCarOff();
        }
      else if (to_run)
        {
        HandleChargeStop();
        HandleCarOn();
        }
  }
}

void OvmsVehicleNetaAya::HandleCarOn()
{
  POLLSTATE_RUNNING;
}
void OvmsVehicleNetaAya::HandleCarOff()
{
  POLLSTATE_OFF;
}
void OvmsVehicleNetaAya::HandleCharging()
{
  POLLSTATE_CHARGING;
  ReadChargeType();
}

void OvmsVehicleNetaAya::HandleChargeStop()
{
  ResetChargeType();
}

void OvmsVehicleNetaAya::ReadChargeType() const
{
  bool using_ccs = StdMetrics.ms_v_bat_power->AsFloat(0, kW) < -15;
  StdMetrics.ms_v_charge_type->SetValue(using_ccs ? "ccs" : "type2");
}

void OvmsVehicleNetaAya::ResetChargeType() const
{
  StdMetrics.ms_v_charge_type->SetValue("");
}

OvmsVehicleNetaAya::PollState OvmsVehicleNetaAya::GetPollState() const
{
  switch (m_poll_state)
  {
    case 0:
      return PollState::OFF;
    case 1:
      return PollState::RUNNING;
    case 2:
      return PollState::CHARGING;
    default:
      assert(false);
      return PollState::OFF;
  }
}

void OvmsVehicleNetaAya::IncomingFrameCan1(CAN_frame_t *p_frame)
{
  /*
  BASIC METRICS
  StdMetrics.ms_v_pos_speed         ok
  StdMetrics.ms_v_bat_soc           ok
  StdMetrics.ms_v_pos_odometer      ok

  StdMetrics.ms_v_door_fl           ok
  StdMetrics.ms_v_door_fr           ok
  StdMetrics.ms_v_door_rl           ok
  StdMetrics.ms_v_door_rr           ok
  StdMetrics.ms_v_env_locked        ok

  StdMetrics.ms_v_bat_current       ok
  StdMetrics.ms_v_bat_voltage       ok
  StdMetrics.ms_v_bat_power         ok

  StdMetrics.ms_v_charge_inprogress ok 

  StdMetrics.ms_v_env_on            ok
  StdMetrics.ms_v_env_awake         NA
  */

  uint8_t *data = p_frame->data.u8;

  switch (p_frame->MsgID)
  {
  case 0x339:
    StdMetrics.ms_v_door_fl->SetValue(CAN_BIT(1, 1));
    StdMetrics.ms_v_door_fr->SetValue(CAN_BIT(1, 3));
    StdMetrics.ms_v_door_rl->SetValue(CAN_BIT(1, 5));
    StdMetrics.ms_v_door_rr->SetValue(CAN_BIT(1, 7));
    StdMetrics.ms_v_env_locked->SetValue(CAN_BIT(4, 6));
    break;
  default:
    return;
  }
}

void OvmsVehicleNetaAya::IncomingPollReply(const OvmsPoller::poll_job_t &job, uint8_t *data, uint8_t length)
{
  switch (job.moduleid_rec)
  {
    case 0x7ea:
      switch (job.pid)
      {
      // speed
      case 0xb100:
        StdMetrics.ms_v_pos_speed->SetValue(CAN_UINT(0) / 100, Kph);
        ;
        break;
      // soc
      case 0xf015:
        StdMetrics.ms_v_bat_soc->SetValue(CAN_BYTE(0), Percentage);
        break;
      // odometer
      case 0xe101:
        StdMetrics.ms_v_pos_odometer->SetValue(CAN_UINT24(0), Kilometers);
        break;
      // current
      case 0xf013:
        float curr;
        uid_t value;
        value = CAN_UINT(0);
        if (value < 32000)
        {
          curr = (32000 - value) * -1;
        }
        else
        {
          curr = value - 32000;
        }
        StdMetrics.ms_v_bat_current->SetValue(curr / 20, Amps);
        break;
      // voltage
      case 0xf012:
        StdMetrics.ms_v_bat_voltage->SetValue(CAN_UINT(0) / 20, Volts);
        break;
      // on
      case 0xd001:
        bool on = CAN_BYTE(0) != 0x01;
        StdMetrics.ms_v_env_awake->SetValue(on);
        StdMetrics.ms_v_env_on->SetValue(on);
        break;
      }
    case 0x718:
      switch (job.pid)
      {
      case 0xf012:
        if (job.mlframe == 1)
        {
          bool charging;
          charging = data[1] != 0x00 && data[1] != 0x27;
          charging = charging && StdMetrics.ms_v_bat_current->AsFloat(0, Amps) > 1;
          StdMetrics.ms_v_charge_inprogress->SetValue(charging);
        }
        if (job.mlframe == 3) {
          ESP_LOGE(TAG, "\n");
        }
        break;
      }
    default:
      ESP_LOGD(TAG, "Unknown module: %03" PRIx32, job.moduleid_rec);
      break;
  }
}

class OvmsVehicleNetaAyaInit
{
public:
  OvmsVehicleNetaAyaInit();
} MyOvmsVehicleNetaAyaInit __attribute__((init_priority(9000)));

OvmsVehicleNetaAyaInit::OvmsVehicleNetaAyaInit()
{
  ESP_LOGI(TAG, "Registering Vehicle: Neta Aya (9000)");
  MyVehicleFactory.RegisterVehicle<OvmsVehicleNetaAya>("NTA", "Neta Aya"); // model tag
}
