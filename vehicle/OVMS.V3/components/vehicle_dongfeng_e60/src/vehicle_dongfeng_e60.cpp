/*
;    Project:       Open Vehicle Monitor System
;    Date:          7th August 2024
;
;    Changes:
;    0.0.1  Initial submit. 
;			- DongFeng E60. Totally untested.
;
;    (C) 2011       Axel Troncoso / Tucar
;;
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

#include "vehicle_dongfeng_e60.h"

#include "metrics_standard.h"

#include "ovms_log.h"
#include "ovms_events.h"

#define VERSION "0.1.6"

// CAN buffer access macros: b=byte# 0..7 / n=nibble# 0..15
#define CAN_BYTE(b)     data[b]
#define CAN_INT(b)      ((int16_t)CAN_UINT(b))
#define CAN_UINT(b)     (((UINT)CAN_BYTE(b) << 8) | CAN_BYTE(b+1))
#define CAN_UINT24(b)   (((uint32_t)CAN_BYTE(b) << 16) | ((UINT)CAN_BYTE(b+1) << 8) | CAN_BYTE(b+2))
#define CAN_UINT32(b)   (((uint32_t)CAN_BYTE(b) << 24) | ((uint32_t)CAN_BYTE(b+1) << 16)  | ((UINT)CAN_BYTE(b+2) << 8) | CAN_BYTE(b+3))
#define CAN_NIBL(b)     (can_databuffer[b] & 0x0f)
#define CAN_NIBH(b)     (can_databuffer[b] >> 4)
#define CAN_NIB(n)      (((n)&1) ? CAN_NIBL((n)>>1) : CAN_NIBH((n)>>1))
#define CAN_BIT(b,pos) 	!!(data[b] & (1<<(pos)))

#define TO_CELCIUS(n)	((float)n-40)
#define TO_PSI(n)		((float)n/4.0)

#define UDS_SID_IOCTRL_BY_ID 		0x2F	// InputOutputControlByCommonID
#define UDS_SID_IOCTRL_BY_LOC_ID	0x30 	// InputOutputControlByLocalID
#define UDS_SID_TESTER_PRESENT		0x3E 	// TesterPresent

#define UDS_DEFAULT_SESSION 					0x01
#define UDS_PROGRAMMING_SESSION 				0x02
#define UDS_EXTENDED_DIAGNOSTIC_SESSION 		0x03
#define UDS_SAFETY_SYSTEM_DIAGNOSTIC_SESSION 	0x04
    
#define POLLSTATE_OFF          PollSetState(0);
#define POLLSTATE_RUNNING      PollSetState(1);
#define POLLSTATE_CHARGING     PollSetState(2);


static const char *TAG = "v-dongfengE60";


OvmsVehicleDFE60::OvmsVehicleDFE60()
  {
  ESP_LOGI(TAG, "Dongfeng E60 vehicle module");

	StandardMetrics.ms_v_type->SetValue("DFE60");

	StandardMetrics.ms_v_bat_12v_voltage->SetValue(12.5, Volts);
	StandardMetrics.ms_v_charge_inprogress->SetValue(false);
	StandardMetrics.ms_v_env_on->SetValue(false);
	StandardMetrics.ms_v_bat_temp->SetValue(20, Celcius);

	can_message_buffer.id = 0;
	can_message_buffer.status = 0;

	RegisterCanBus(1, CAN_MODE_ACTIVE, CAN_SPEED_500KBPS);
	auto result = setParamConfig("auto", "vehicle.type", "DFE60");
	if (result != ParamSetResult::Fail)
		{
		ESP_LOGE(TAG, "Error setting vehicle.type");
		}
	}

OvmsVehicleDFE60::~OvmsVehicleDFE60()
	{
	ESP_LOGI(TAG, "Shutdown Dongfeng E60 vehicle module");
	}

void OvmsVehicleDFE60::Ticker1(uint32_t ticker)
	{
	}

OvmsVehicle::vehicle_command_t OvmsVehicleDFE60::CommandLock(const char *pin)
	{
	return NotImplemented;
	}

OvmsVehicle::vehicle_command_t OvmsVehicleDFE60::CommandUnlock(const char *pin)
	{
	return NotImplemented;
	}

void OvmsVehicleDFE60::IncomingFrameCan1(CAN_frame_t *p_frame)
	{
	}


class OvmsVehicleDFE60Init
	{
	public: OvmsVehicleDFE60Init();
} MyOvmsVehicleDFE60Init __attribute__((init_priority(9000)));

OvmsVehicleDFE60Init::OvmsVehicleDFE60Init()
	{
	ESP_LOGI(TAG, "Registering Vehicle: DongFeng E60");
	MyVehicleFactory.RegisterVehicle<OvmsVehicleDFE60>("DFE60", "DongFeng E60");
	}
