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

#ifndef __VEHICLE_KIANIROEVSG2_H__
#define __VEHICLE_KIANIROEVSG2_H__

#include "../../vehicle_kiasoulev/src/kia_common.h"
#include "vehicle.h"


using namespace std;

class OvmsVehicleKiaNiroEvSg2 : public KiaVehicle
  {
  public:
		OvmsVehicleKiaNiroEvSg2();
    ~OvmsVehicleKiaNiroEvSg2();

  public:
    void IncomingFrameCan1(CAN_frame_t *p_frame) override;
    void Ticker1(uint32_t ticker) override;
    void IncomingPollReply(const OvmsPoller::poll_job_t &job, uint8_t *data, uint8_t length) override;
    void SendTesterPresent(uint16_t id, uint8_t length);
    bool SetSessionMode(uint16_t id, uint8_t mode);
    void SendCanMessage(uint16_t id, uint8_t count,
						uint8_t serviceId, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
						uint8_t b5, uint8_t b6);
    void SendCanMessageTriple(uint16_t id, uint8_t count,
						uint8_t serviceId, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
						uint8_t b5, uint8_t b6);
    bool SendCanMessage_sync(uint16_t id, uint8_t count,
    					uint8_t serviceId, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
						uint8_t b5, uint8_t b6);
    bool SendCommandInSessionMode(uint16_t id, uint8_t count,
    					uint8_t serviceId, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
						  uint8_t b5, uint8_t b6, uint8_t mode);

  protected:
    void IncomingVMCU(canbus *bus, uint16_t type, uint16_t pid, const uint8_t *data, uint8_t length, uint16_t mlframe, uint16_t mlremain);
    void IncomingBMC(canbus* bus, uint16_t type, uint16_t pid, const uint8_t* data, uint8_t length, uint16_t mlframe, uint16_t mlremain);
    void IncomingBCM(canbus* bus, uint16_t type, uint16_t pid, const uint8_t* data, uint8_t length, uint16_t mlframe, uint16_t mlremain);
    void IncomingIGMP(canbus* bus, uint16_t type, uint16_t pid, const uint8_t* data, uint8_t length, uint16_t mlframe, uint16_t mlremain);
    void IncomingCM(canbus* bus, uint16_t type, uint16_t pid, const uint8_t* data, uint8_t length, uint16_t mlframe, uint16_t mlremain);
    void IncomingSW(canbus* bus, uint16_t type, uint16_t pid, const uint8_t* data, uint8_t length, uint16_t mlframe, uint16_t mlremain);
    void SendTesterPresentMessages();
    void StopTesterPresentMessages();
  
  private:

    enum class PollState
      {
        OFF,
        RUNNING,
        CHARGING
      };

    void HandleCharging();
    void HandleChargeStop();
    void HandleCarOn();
    void HandleCarOff();

    void SetChargeType();
    void ResetChargeType();

    PollState GetPollState();

  };

#endif //#ifndef __VEHICLE_KIANIROEVSG2_H__
