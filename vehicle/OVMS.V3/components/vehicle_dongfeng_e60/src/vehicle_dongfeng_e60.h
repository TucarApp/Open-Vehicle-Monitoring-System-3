/*
;    Project:       Open Vehicle Monitor System
;    Date:          24th July 2024
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

#ifndef __VEHICLE_DONGFENGE60_H__
#define __VEHICLE_DONGFENGE60_H__

#include "vehicle.h"


class OvmsVehicleDFE60 : public OvmsVehicle
  {
  public:
		OvmsVehicleDFE60();
    ~OvmsVehicleDFE60();

  public:

    void IncomingFrameCan1(CAN_frame_t *p_frame) override;
    void Ticker1(uint32_t ticker) override;
    void Ticker300(uint32_t ticker) override;

    vehicle_command_t CommandLock(const char *pin) override;
    vehicle_command_t CommandUnlock(const char *pin) override;
  
  private:

    struct
      {
      uint8_t byte[8];
      uint8_t status;
      uint16_t id;
      } can_message_buffer;

  };

#endif // #ifndef __VEHICLE_DONGFENGE60_H__
