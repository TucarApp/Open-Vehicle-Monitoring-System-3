/*
;    Project:       Open Vehicle Monitor System
;    Date:          20th August 2024
;
;    Changes:
;    1.0  Initial release
;
;    (C) 2021       Jaime Middleton / Tucar
;    (C) 2021       Axel Troncoso   / Tucar
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

#ifndef __VEHICLE_INTERFACE_TUCAR_H__
#define __VEHICLE_INTERFACE_TUCAR_H__

#include "vehicle.h"

template<typename T>
struct Optional
{
  Optional() : mHasValue(false) {}

  T getValue() const
  {
    assert(mHasValue);
    return mValue;
  }

  bool hasValue() const
  {
    return mHasValue;
  }

  void setValue(const T& value)
  {
    mValue = value;
    mHasValue = true;
  }

private:
  T mValue;
  bool mHasValue;
};

class OvmsVehicleInterfaceTucar : public OvmsVehicle
{
public:
  OvmsVehicleInterfaceTucar();
  virtual ~OvmsVehicleInterfaceTucar() = default;

  bool hasImei() const;
  std::string getImei() const;
  std::string getId() const;

private:
  void modemReceivedImei(std::string event, void* data);
  void setImei(const std::string& imei);

  Optional<std::string> mImei;
};

#endif //#ifndef __VEHICLE_INTERFACE_TUCAR_H__
