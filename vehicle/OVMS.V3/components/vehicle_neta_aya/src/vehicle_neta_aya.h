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

#ifndef __VEHICLE_NETAAYA_H__
#define __VEHICLE_NETAAYA_H__

#include "vehicle.h"

using namespace std;

class OvmsVehicleNetaAya : public OvmsVehicle
  {
  public:
    enum class PollState : uint8_t
    {
      OFF = 0,
      RUNNING = 1,
      CHARGING = 2
    };

		OvmsVehicleNetaAya();
    ~OvmsVehicleNetaAya();

  public:
    void IncomingFrameCan1(CAN_frame_t* p_frame) override;
    void IncomingFrameCan2(CAN_frame_t* p_frame) override;
    void Ticker1(uint32_t ticker) override;
    void IncomingPollReply(const OvmsPoller::poll_job_t &job, uint8_t *data, uint8_t length) override;

    void SendCanMessage(uint16_t id, uint8_t count,
						uint8_t serviceId, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
						uint8_t b5, uint8_t b6);
    void SendCanMessageSecondary(uint16_t id, uint8_t count,
                        uint8_t serviceId, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
                        uint8_t b5, uint8_t b6);
    void SendCanMessageTriple(uint16_t id, uint8_t count,
						uint8_t serviceId, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
						uint8_t b5, uint8_t b6);

  private:
    void HandleCharging();
    void HandleChargeStop();
    void HandleCarOn();
    void HandleCarOff();

    void ReadChargeType() const;
    void ResetChargeType() const;

    PollState GetPollState() const;

    struct {
      uint8_t byte[8];
      uint8_t status;
      uint16_t id;
    } send_can_buffer;
  };

#endif //#ifndef __VEHICLE_NETAAYA_H__
