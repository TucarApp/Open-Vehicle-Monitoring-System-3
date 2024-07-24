/*
;    Project:       Open Vehicle Monitor System
;    Date:          21th January 2019
;
;    Changes:
;    0.0.1  Initial stub
;			- Ported from Kia Soul. Totally untested.
;
;    0.1.0  First version on par with Soul
;			- First "complete" version.
;
;		0.1.1 06-apr-2019 - Geir Øyvind Vælidalo
;			- Minor changes after proper real life testing
;			- VIN is working
;			- Removed more of the polling when car is off in order to prevent AUX battery drain
;
;		0.1.2 10-apr-2019 - Geir Øyvind Vælidalo
;			- Fixed TPMS reading
;			- Fixed xks aux
;			- Estimated range show WLTP in lack of the actual displayed range
;			- Door lock works even after ECU goes to sleep.
;
;		0.1.3 12-apr-2019 - Geir Øyvind Vælidalo
;			- Fixed OBC temp reading
;			- Removed a couple of pollings when car is off, in order to save AUX battery
;			- Added range calculator for estimated range instead of WLTP. It now uses 20 last trips as a basis.
;
;		0.1.5 18-apr-2019 - Geir Øyvind Vælidalo
;			- Changed poll frequencies to minimize the strain on the CAN-write function.
;
;		0.1.6 20-apr-2019 - Geir Øyvind Vælidalo
;			- AUX Battery monitor..
;
;    (C) 2011       Michael Stegen / Stegen Electronics
;    (C) 2011-2017  Mark Webb-Johnson
;    (C) 2011       Sonny Chen @ EPRO/DX
;    (C) 2019       Geir Øyvind Vælidalo <geir@validalo.net>
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
#define TO_PSI(n)			((float)n/4.0)

#define UDS_SID_IOCTRL_BY_ID 			0x2F	// InputOutputControlByCommonID
#define UDS_SID_IOCTRL_BY_LOC_ID	0x30 	// InputOutputControlByLocalID
#define UDS_SID_TESTER_PRESENT		0x3E 	// TesterPresent

#define UDS_DEFAULT_SESSION 									0x01
#define UDS_PROGRAMMING_SESSION 							0x02
#define UDS_EXTENDED_DIAGNOSTIC_SESSION 			0x03
#define UDS_SAFETY_SYSTEM_DIAGNOSTIC_SESSION 	0x04
    
#define POLLSTATE_OFF          PollSetState(0);
#define POLLSTATE_RUNNING      PollSetState(1);
#define POLLSTATE_CHARGING     PollSetState(2);


static const char *TAG = "v-dfe60";


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
	}

OvmsVehicleDFE60::~OvmsVehicleDFE60()
	{
	ESP_LOGI(TAG, "Shutdown Dongfeng E60 vehicle module");
	}

void OvmsVehicleDFE60::Ticker1(uint32_t ticker)
	{
	}

void OvmsVehicleDFE60::Ticker300(uint32_t ticker)
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

	const uint8_t (&data)[8] = p_frame->data.u8;

	// Check if response is from synchronous can message
	if (can_message_buffer.status == 0xff && p_frame->MsgID == (can_message_buffer.id + 0x08))
	{
		// Store message bytes so that the async method can continue
		can_message_buffer.status = 3;

		std::copy(std::begin(data), std::end(data), std::begin(can_message_buffer.byte));
	}

	// set batt temp
	switch (p_frame->MsgID)
	{
	case 0x488:
	{
		StdMetrics.ms_v_pos_odometer->SetValue((int)CAN_UINT24(0) / 10, Kilometers);
		break;
	}
	case 0x347:
	{
		StdMetrics.ms_v_env_awake->SetValue(CAN_BIT(0, 6));
		break;
	}
	case 0x23a:
	{
		StdMetrics.ms_v_door_fl->SetValue(CAN_BIT(1, 3));
		StdMetrics.ms_v_door_fr->SetValue(CAN_BIT(1, 4));
		StdMetrics.ms_v_door_rr->SetValue(CAN_BIT(1, 5));
		StdMetrics.ms_v_door_rl->SetValue(CAN_BIT(1, 6));
		break;
	}
	case 0x380:
	{
		StdMetrics.ms_v_bat_soc->SetValue(CAN_BYTE(0), Percentage);
		if (CAN_BIT(1, 2) && !CAN_BIT(1, 3))
		{
			StdMetrics.ms_v_charge_inprogress->SetValue(true);
		}
		else
		{
			StdMetrics.ms_v_charge_inprogress->SetValue(false);
		}
		break;
	}
	case 0x490:
	{
		StdMetrics.ms_v_env_locked->SetValue(CAN_BIT(0, 2));
		break;
	}
	case 0x0a0:
	{
		// (it doesnt show the same speed as in the screen but it is close, a little less when going slow and a little more when going fast)
		StdMetrics.ms_v_pos_speed->SetValue((int)CAN_UINT(6) / 93, Kph);
		break;
	}
	default:
		break;
	}	

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
