/*
;    Project:       Open Vehicle Monitor System
;    Date:          9th August 2024
;
;    Changes:
;    0.0.1  Initial stub
;
;    (C) 2018        Axel Troncoso <atroncoso@tucar.app> / Tucar
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

#include "vehicle_kia_niroevsg2.h"

#include "ovms_log.h"

#include <stdio.h>
#include <string.h>
#include "pcp.h"
#include "metrics_standard.h"
#include "ovms_metrics.h"
#include "ovms_notify.h"
#include <sys/param.h>
#include "../../vehicle_kiasoulev/src/kia_common.h"

#define VERSION "0.0.1"

static const char *TAG = "v-kianiroevsg2";

// Pollstate 0 - car is off
// Pollstate 1 - car is on
// Pollstate 2 - car is charging
static const OvmsPoller::poll_pid_t vehicle_kianiroevsg2_polls[] =
	{
		// ok2	IncomingIGMP
		{0x770, 0x778, VEHICLE_POLL_TYPE_OBDIIEXTENDED, 0xbc03, {2, 2, 2}, 1, ISOTP_STD}, // IGMP Door status and lock
		{0x770, 0x778, VEHICLE_POLL_TYPE_OBDIIEXTENDED, 0xbc04, {2, 2, 2}, 1, ISOTP_STD}, // IGMP Door lock

		// ok2 IncomingBCM
		{0x7a0, 0x7a8, VEHICLE_POLL_TYPE_OBDIIEXTENDED, 0xC00B, {0, 25, 0}, 1, ISOTP_STD}, // TMPS - Pressure
		{0x7a0, 0x7a8, VEHICLE_POLL_TYPE_OBDIIEXTENDED, 0xB010, {0, 25, 0}, 1, ISOTP_STD}, // TMPS - Pressure

		// ok2	IncomingVMCU
		{0x7e2, 0x7ea, VEHICLE_POLL_TYPE_OBDIIGROUP, 0x02, {600, 25, 25}, 1, ISOTP_STD}, // VMCU - Aux Battery voltage

		// ok2	IncomingBMC
		{0x7e4, 0x7ec, VEHICLE_POLL_TYPE_OBDIIEXTENDED, 0x0101, {5, 2, 2}, 1, ISOTP_STD}, // Voltage and current main bat
		{0x7e4, 0x7ec, VEHICLE_POLL_TYPE_OBDIIEXTENDED, 0x0105, {0, 5, 5}, 1, ISOTP_STD}, // bat soc and soh

		// ok2	IncomingCM
		{0x7c6, 0x7ce, VEHICLE_POLL_TYPE_OBDIIEXTENDED, 0xB002, {0, 25, 60}, 1, ISOTP_STD}, // ODO
		{0x7c6, 0x7ce, VEHICLE_POLL_TYPE_OBDIIEXTENDED, 0xB003, {2, 2, 2}, 1, ISOTP_STD},	// AWAKE

		// ok2	IncomingSteeringwheel
		{0x7d4, 0x7dc, VEHICLE_POLL_TYPE_OBDIIEXTENDED, 0x0101, {2, 2, 2}, 1, ISOTP_STD}, // ON SPEED
		POLL_LIST_END
	};

static const OvmsPoller::poll_pid_t vehicle_kianiroevsg2_polls_stop[] = {POLL_LIST_END};

/**
 * Constructor for Kia Niro EV OvmsVehicleKiaNiroEvSg2
 */
OvmsVehicleKiaNiroEvSg2::OvmsVehicleKiaNiroEvSg2()
  {
  ESP_LOGI(TAG, "Kia Sg2 EV vehicle module");
  }

/**
 * Destructor
 */
OvmsVehicleKiaNiroEvSg2::~OvmsVehicleKiaNiroEvSg2()
  {
	ESP_LOGI(TAG, "Shutdown Kia Sg2 EV vehicle module");
  }

/**
 * Ticker1: Called every second
 */
void OvmsVehicleKiaNiroEvSg2::Ticker1(uint32_t ticker)
	{
	}

/**
 * Ticker10: Called every ten seconds
 */
void OvmsVehicleKiaNiroEvSg2::Ticker10(uint32_t ticker)
	{
	}

/**
 * Ticker300: Called every five minutes
 */
void OvmsVehicleKiaNiroEvSg2::Ticker300(uint32_t ticker)
	{
	}

/**
 * Update metrics when charging
 */
void OvmsVehicleKiaNiroEvSg2::HandleCharging()
	{
	}

/**
 * Update metrics when charging stops
 */
void OvmsVehicleKiaNiroEvSg2::HandleChargeStop()
	{
	}

/**
 *  Sets the charge metrics
 */
void OvmsVehicleKiaNiroEvSg2::SetChargeMetrics(float voltage, float current, float climit, bool ccs)
	{
	}

/**
 * Handles incoming CAN-frames on bus 1, the C-bus
 */
void OvmsVehicleKiaNiroEvSg2::IncomingFrameCan1(CAN_frame_t* p_frame)
	{
	uint8_t *d = p_frame->data.u8;

	// Check if response is from synchronous can message
	if (kia_send_can.status == 0xff && p_frame->MsgID == (kia_send_can.id + 0x08))
		{
		//Store message bytes so that the async method can continue
		kia_send_can.status = 3;

		kia_send_can.byte[0] = d[0];
		kia_send_can.byte[1] = d[1];
		kia_send_can.byte[2] = d[2];
		kia_send_can.byte[3] = d[3];
		kia_send_can.byte[4] = d[4];
		kia_send_can.byte[5] = d[5];
		kia_send_can.byte[6] = d[6];
		kia_send_can.byte[7] = d[7];
		}
	}


/**
 * Incoming poll reply messages
 */
void OvmsVehicleKiaNiroEvSg2::IncomingPollReply(const OvmsPoller::poll_job_t &job, uint8_t *data, uint8_t length)
	{
	// ESP_LOGD(TAG, "IPR %03x TYPE:%x PID:%02x %x %02x %02x %02x %02x %02x %02x %02x %02x", job.moduleid_rec, type, pid, length, data[0], data[1], data[2], data[3],
	//	data[4], data[5], data[6], data[7]);
	switch (job.moduleid_rec)
		{
		// ****** IGMP *****
		case 0x778:
			IncomingIGMP(job.bus, job.type, job.pid, data, length, job.mlframe, job.mlremain);
			break;

		// ****** BCM ******
		case 0x7a8:
			IncomingBCM(job.bus, job.type, job.pid, data, length, job.mlframe, job.mlremain);
			break;

		// ******* VMCU ******
		case 0x7ea:
			IncomingVMCU(job.bus, job.type, job.pid, data, length, job.mlframe, job.mlremain);
			break;

		// ***** BMC ****
		case 0x7ec:
			IncomingBMC(job.bus, job.type, job.pid, data, length, job.mlframe, job.mlremain);
			break;

		// ***** CM ****
		case 0x7ce:
			IncomingCM(job.bus, job.type, job.pid, data, length, job.mlframe, job.mlremain);
			break;

		// ***** SW ****
		case 0x7dc:
			IncomingSW(job.bus, job.type, job.pid, data, length, job.mlframe, job.mlremain);
			break;

		default:
			ESP_LOGD(TAG, "Unknown module: %03" PRIx32, job.moduleid_rec);
			break;
		}
	}

/**
 * Handle incoming messages from cluster.
 */
void OvmsVehicleKiaNiroEvSg2::IncomingCM(canbus *bus, uint16_t type, uint16_t pid, const uint8_t *data, uint8_t length, uint16_t mlframe, uint16_t mlremain)
	{
	switch (pid)
		{
		case 0xb002:
			if (mlframe == 1)
			{
				StdMetrics.ms_v_pos_odometer->SetValue(CAN_UINT24(3), Kilometers);
			}
			break;
		case 0xb003:
			if (mlframe == 1)
			{
				StdMetrics.ms_v_env_awake->SetValue(CAN_BYTE(1) != 0);
				if (!StdMetrics.ms_v_env_awake->AsBool())
				{
					StdMetrics.ms_v_env_on->SetValue(false);
				}
			}
			break;
		}
	}

/**
 * Handle incoming messages from VMCU-poll
 *
 * - Aux battery SOC, Voltage and current
 */
void OvmsVehicleKiaNiroEvSg2::IncomingVMCU(canbus *bus, uint16_t type, uint16_t pid, const uint8_t *data, uint8_t length, uint16_t mlframe, uint16_t mlremain)
	{
	switch (pid)
		{
		case 0x02:
			if (type == VEHICLE_POLL_TYPE_OBDIIGROUP)
			{
				if (mlframe == 3)
				{
					StdMetrics.ms_v_bat_12v_voltage->SetValue(((CAN_BYTE(2) << 8) + CAN_BYTE(1)) / 1000.0, Volts);
				}
			}
			break;
		}
	}

/**
 * Handle incoming messages from BMC-poll
 *
 * - Pilot signal available
 * - CCS / Type2
 * - Battery current
 * - Battery voltage
 * - Battery module temp 1-8
 * - Cell voltage max / min + cell #
 * + more
 */
void OvmsVehicleKiaNiroEvSg2::IncomingBMC(canbus *bus, uint16_t type, uint16_t pid, const uint8_t *data, uint8_t length, uint16_t mlframe, uint16_t mlremain)
	{
	if (type == VEHICLE_POLL_TYPE_OBDIIEXTENDED)
		{
		switch (pid)
			{
			case 0x0101:
				// diag page 01: skip first frame (no data)
				// ESP_LOGD(TAG, "Frame number %x",mlframe);

				if (mlframe == 2)
				{
					StdMetrics.ms_v_bat_current->SetValue((float)CAN_INT(0) / 10.0, Amps); // negative regen, positive accel
					StdMetrics.ms_v_bat_voltage->SetValue((float)CAN_UINT(2) / 10.0, Volts);
				}
				break;

			case 0x0105:
				if (mlframe == 4)
				{
					StdMetrics.ms_v_bat_soh->SetValue((float)CAN_UINT(1) / 10.0, Percentage);
				}
				else if (mlframe == 5)
				{
					StdMetrics.ms_v_bat_soc->SetValue(CAN_BYTE(0) / 2.0, Percentage);
				}
				break;
			}
		}
	}

/**
 * Handle incoming messages from BCM-poll
 */
void OvmsVehicleKiaNiroEvSg2::IncomingBCM(canbus *bus, uint16_t type, uint16_t pid, const uint8_t *data, uint8_t length, uint16_t mlframe, uint16_t mlremain)
	{
	uint8_t bVal;
	if (type == VEHICLE_POLL_TYPE_OBDIIEXTENDED)
		{
		switch (pid)
			{
			case 0xC00B:
				if (mlframe == 1)
				{
					bVal = CAN_BYTE(1);
					if (bVal > 0)
						StdMetrics.ms_v_tpms_pressure->SetElemValue(MS_V_TPMS_IDX_FL, bVal / 5.0, PSI);
					bVal = CAN_BYTE(6);
					if (bVal > 0)
						StdMetrics.ms_v_tpms_pressure->SetElemValue(MS_V_TPMS_IDX_FR, bVal / 5.0, PSI);
				}
				else if (mlframe == 2)
				{
					bVal = CAN_BYTE(4);
					if (bVal > 0)
						StdMetrics.ms_v_tpms_pressure->SetElemValue(MS_V_TPMS_IDX_RL, bVal / 5.0, PSI);
				}
				else if (mlframe == 3)
				{
					bVal = CAN_BYTE(2);
					if (bVal > 0)
						StdMetrics.ms_v_tpms_pressure->SetElemValue(MS_V_TPMS_IDX_RR, bVal / 5.0, PSI);
				}
				break;

			case 0xB010:
				if (mlframe == 1)
				{
					bVal = CAN_BYTE(1);
					/* TODO: manage windows_open*/
					if (bVal > 0)
					{
						// windows_open = true;
					}
					else
					{
						// windows_open = false;
					}
				}
				break;
			}
		}
	}

/**
 * Handle incoming messages from IGMP-poll
 *
 *
 */
void OvmsVehicleKiaNiroEvSg2::IncomingIGMP(canbus *bus, uint16_t type, uint16_t pid, const uint8_t *data, uint8_t length, uint16_t mlframe, uint16_t mlremain)
	{
	if (type == VEHICLE_POLL_TYPE_OBDIIEXTENDED)
		{
		switch (pid)
			{
			case 0xbc03:
				if (mlframe == 1)
				{
					StdMetrics.ms_v_door_fl->SetValue(CAN_BIT(1, 5));
					StdMetrics.ms_v_door_fr->SetValue(CAN_BIT(1, 4));
					StdMetrics.ms_v_door_rl->SetValue(CAN_BIT(1, 0));
					StdMetrics.ms_v_door_rr->SetValue(CAN_BIT(1, 2));
					m_v_door_lock_rl->SetValue(!CAN_BIT(1, 1));
					m_v_door_lock_rr->SetValue(!CAN_BIT(1, 3));
				}
				break;

			case 0xbc04:
				if (mlframe == 1)
				{
					m_v_door_lock_fl->SetValue(!CAN_BIT(1, 3));
					m_v_door_lock_fr->SetValue(!CAN_BIT(1, 2));
				}
				break;
			}
		}
	}

void OvmsVehicleKiaNiroEvSg2::IncomingSW(canbus *bus, uint16_t type, uint16_t pid, const uint8_t *data, uint8_t length, uint16_t mlframe, uint16_t mlremain)
	{
	if (type == VEHICLE_POLL_TYPE_OBDIIEXTENDED)
		{
		switch (pid)
			{
			case 0x0101:
				if (mlframe == 2)
				{
					StdMetrics.ms_v_env_on->SetValue(CAN_BYTE(6) != 0);
					StdMetrics.ms_v_pos_speed->SetValue(CAN_BYTE(2), Kph);
				}
				break;
			}
		}
	}



class OvmsVehicleKiaNiroEvSg2Init
  {
  public:
  	OvmsVehicleKiaNiroEvSg2Init();
  } MyOvmsVehicleKiaNiroEvSg2Init __attribute__((init_priority(9000)));

OvmsVehicleKiaNiroEvSg2Init::OvmsVehicleKiaNiroEvSg2Init()
	{
	ESP_LOGI(TAG, "Registering Vehicle: Kia Niro Sg2 EV (9000)");
	MyVehicleFactory.RegisterVehicle<OvmsVehicleKiaNiroEvSg2>("KN2", "Kia Niro Sg2 EV");
	}
