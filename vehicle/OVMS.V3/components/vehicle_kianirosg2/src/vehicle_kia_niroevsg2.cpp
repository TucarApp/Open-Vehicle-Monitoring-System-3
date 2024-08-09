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
static const OvmsPoller::poll_pid_t vehicle_kianiroev_polls[] =
  {
  		{ 0x7e2, 0x7ea, VEHICLE_POLL_TYPE_OBDII_1A, 				0x80, 			{       0,  120,	 120 }, 0, ISOTP_STD },  // VMCU - VIN

		{ 0x7e4, 0x7ec, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0x0101, 		{      9,    9,   9 }, 0, ISOTP_STD }, 	// BMC Diag page 01 - Must be called when off to detect when charging
		{ 0x7e4, 0x7ec, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0x0102, 		{       0,   59,   9 }, 0, ISOTP_STD }, 	// BMC Diag page 02
		{ 0x7e4, 0x7ec, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0x0103, 		{       0,   59,   9 }, 0, ISOTP_STD }, 	// BMC Diag page 03
		{ 0x7e4, 0x7ec, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0x0104, 		{       0,   59,   9 }, 0, ISOTP_STD }, 	// BMC Diag page 04
		{ 0x7e4, 0x7ec, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0x0105, 		{       0,   59,   9 }, 0, ISOTP_STD },		// BMC Diag page 05
		{ 0x7e4, 0x7ec, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0x0106, 		{       0,    9,   9 }, 0, ISOTP_STD },		// BMC Diag page 06

		{ 0x7a0, 0x7a8, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0xB00C, 		{       0,   29,  29 }, 0, ISOTP_STD },   // BCM Heated handle
		{ 0x7a0, 0x7a8, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0xB00E, 		{       0,   10,  10 }, 0, ISOTP_STD },   // BCM Chargeport ++

		{ 0x7a0, 0x7a8, VEHICLE_POLL_TYPE_OBDIIEXTENDED,   0xC002, 		{       0,   60,   0 }, 0, ISOTP_STD }, 	// TMPS - ID's
		{ 0x7a0, 0x7a8, VEHICLE_POLL_TYPE_OBDIIEXTENDED,   0xC00B, 		{       0,   13,   0 }, 0, ISOTP_STD }, 	// TMPS - Pressure and Temp

		{ 0x770, 0x778, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0xbc03, 		{      7,    7,   7 }, 0, ISOTP_STD },  // IGMP Door status + IGN1 & IGN2 - Detects when car is turned on
		{ 0x770, 0x778, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0xbc04, 		{       0,   11,  11 }, 0, ISOTP_STD },  // IGMP Door status
		{ 0x770, 0x778, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0xbc07, 		{       0,   13,  13 }, 0, ISOTP_STD },  // IGMP Rear/mirror defogger

		{ 0x7b3, 0x7bb, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0x0100, 		{       0,   10,  10 }, 0, ISOTP_STD },  // AirCon
		//{ 0x7b3, 0x7bb, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0x0102, 		{       0,   10,  10 } },  // AirCon - No usable values found yet

		{ 0x7c6, 0x7ce, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0xB002, 		{       0,   19, 120 }, 0, ISOTP_STD },  // Cluster. ODO

		{ 0x7d1, 0x7d9, VEHICLE_POLL_TYPE_OBDIIEXTENDED,  	0xc101, 		{       0,   27,  27 }, 0, ISOTP_STD },  // ABS/ESP - Emergency lights

		{ 0x7e5, 0x7ed, VEHICLE_POLL_TYPE_OBDIIGROUP,  		0x01, 			{       0,   58,  11 }, 0, ISOTP_STD }, 	// OBC - On board charger
    //{ 0x7e5, 0x7ed, VEHICLE_POLL_TYPE_OBDIIGROUP,  		0x02, 			{       0,   58,  11 } }, 	// OBC
		{ 0x7e5, 0x7ed, VEHICLE_POLL_TYPE_OBDIIGROUP,  		0x03, 			{       0,   58,  11 }, 0, ISOTP_STD }, 	// OBC

		{ 0x7e2, 0x7ea, VEHICLE_POLL_TYPE_OBDIIGROUP,  		0x01, 			{       0,    7,  19 }, 0, ISOTP_STD },  // VMCU - Shift position
		{ 0x7e2, 0x7ea, VEHICLE_POLL_TYPE_OBDIIGROUP,  		0x02, 			{     	  0,    7,  7 }, 0, ISOTP_STD },  // VMCU - Aux Battery data

		{ 0x7e3, 0x7eb, VEHICLE_POLL_TYPE_OBDIIGROUP,  		0x02, 			{       0,   11,  11 }, 0, ISOTP_STD },  // MCU

    POLL_LIST_END
  };

// Charging profile
// Based mostly on this graph: https://support.fastned.nl/hc/en-gb/articles/360007699174-Charging-with-a-Kia-e-Niro
charging_profile niro_charge_steps[] = {
			 //from%, to%, Chargespeed in Wh
       {  0, 10,	34000 },
       { 10, 40, 78000 },
       { 40, 55, 70000 },
       { 55, 73, 58000 },
       { 73, 78, 38000 },
       { 78, 87, 25000 },
       { 87, 95, 10000 },
       { 95, 100, 7200 },
       { 0,0,0 },
};

/**
 * Constructor for Kia Niro EV OvmsVehicleKiaNiroEvSg2
 */
OvmsVehicleKiaNiroEvSg2::OvmsVehicleKiaNiroEvSg2()
  {
  ESP_LOGI(TAG, "Kia Sg2 EV vehicle module");
#ifdef CONFIG_OVMS_COMP_WEBSERVER
  WebInit();
#endif
  }

/**
 * Destructor
 */
OvmsVehicleKiaNiroEvSg2::~OvmsVehicleKiaNiroEvSg2()
  {
	ESP_LOGI(TAG, "Shutdown Kia Sg2 EV vehicle module");
#ifdef CONFIG_OVMS_COMP_WEBSERVER
  MyWebServer.DeregisterPage("/bms/cellmon");
#endif
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

void OvmsVehicleKiaNiroEvSg2::EventListener(std::string event, void* data)
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
 * Updates the maximum real world range at current temperature.
 * Also updates the State of Health
 */
void OvmsVehicleKiaNiroEvSg2::UpdateMaxRangeAndSOH(void)
	{
	}


/**
 * Open or lock the doors
 */
bool OvmsVehicleKiaNiroEvSg2::SetDoorLock(bool open, const char* password)
	{
	bool result=false;

	if( kn_shift_bits.Park )
  		{
    if( PinCheck((char*)password) )
    		{
    	  ACCRelay(true,password	);
    		LeftIndicator(true);
    		result = Send_IGMP_Command(0xbc, open?0x11:0x10, 0x03);
    		ACCRelay(false,password	);
    		}
  		}
		return result;
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
