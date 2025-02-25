//  SPDX-License-Identifier: BSD-2-Clause OR GPL-3.0-or-later
//
//  CAN Interface API, Version 3 (for PEAK PCAN Interfaces)
//
//  Copyright (c) 2010-2021 Uwe Vogt, UV Software, Berlin (info@uv-software.com)
//  All rights reserved.
//
//  This file is part of PCANBasic-Wrapper.
//
//  PCANBasic-Wrapper is dual-licensed under the BSD 2-Clause "Simplified" License
//  and under the GNU General Public License v3.0 (or any later version). You can
//  choose between one of them if you use PCANBasic-Wrapper in whole or in part.
//
//  BSD 2-Clause "Simplified" License:
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//  1. Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  PCANBasic-Wrapper IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF PCANBasic-Wrapper, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//  GNU General Public License v3.0 or later:
//  PCANBasic-Wrapper is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  PCANBasic-Wrapper is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with PCANBasic-Wrapper.  If not, see <https://www.gnu.org/licenses/>.
//
#include "build_no.h"
#ifdef _MSC_VER
#define VERSION_MAJOR    0
#define VERSION_MINOR    4
#define VERSION_PATCH    1
#else
#define VERSION_MAJOR    0
#define VERSION_MINOR    2
#define VERSION_PATCH    1
#endif
#define VERSION_BUILD    BUILD_NO
#define VERSION_STRING   TOSTRING(VERSION_MAJOR) "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH) " (" TOSTRING(BUILD_NO) ")"
#if defined(_WIN64)
#define PLATFORM        "x64"
#elif defined(_WIN32)
#define PLATFORM        "x86"
#elif defined(__linux__)
#define PLATFORM        "Linux"
#elif defined(__APPLE__)
#define PLATFORM        "macOS"
#else
#error Unsupported architecture
#endif
static const char version[] = "CAN API V3 for PEAK PCAN Interfaces, Version " VERSION_STRING;

#ifdef _MSC_VER
//no Microsoft extensions please!
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#endif
#include "PeakCAN.h"
#include "can_defs.h"
#include "can_api.h"
#include "can_btr.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#if (OPTION_PCAN_DYLIB != 0)
__attribute__((constructor))
static void _initializer() {
    // default initializer
}
__attribute__((destructor))
static void _finalizer() {
    // default finalizer
}
#define EXPORT  __attribute__((visibility("default")))
#else
#define EXPORT
#endif

#ifndef _MSC_VER
#define STRCPY_S(dest,size,src)         strcpy(dest,src)
#define STRNCPY_S(dest,size,src,len)    strncpy(dest,src,len)
#define SSCANF_S(buf,format,...)        sscanf(buf,format,__VA_ARGS__)
#define SPRINTF_S(buf,size,format,...)  sprintf(buf,format,__VA_ARGS__)
#else
#define STRCPY_S(dest,size,src)         strcpy_s(dest,size,src)
#define STRNCPY_S(dest,size,src,len)    strncpy_s(dest,size,src,len)
#define SSCANF_S(buf,format,...)        sscanf_s(buf,format,__VA_ARGS__)
#define SPRINTF_S(buf,size,format,...)  sprintf_s(buf,size,format,__VA_ARGS__)
#endif

struct CPeakCAN::SCAN {
    can_handle_t m_Handle;
    // constructor/destructor
    SCAN() {
        m_Handle = -1;
    }
    ~SCAN() {}
};

EXPORT
CPeakCAN::CPeakCAN() {
    m_OpMode.byte = CANMODE_DEFAULT;
    m_Bitrate.index = CANBTR_INDEX_250K;
    m_Bitrate.btr.nominal.brp = 0;
    m_Bitrate.btr.nominal.tseg1 = 0;
    m_Bitrate.btr.nominal.tseg2 = 0;
    m_Bitrate.btr.nominal.sjw = 0;
    m_Bitrate.btr.nominal.sam = 0;
    m_Bitrate.btr.data.brp = m_Bitrate.btr.nominal.brp;
    m_Bitrate.btr.data.tseg1 = m_Bitrate.btr.nominal.tseg1;
    m_Bitrate.btr.data.tseg2 = m_Bitrate.btr.nominal.tseg2;
    m_Bitrate.btr.data.sjw = m_Bitrate.btr.nominal.sjw;
    m_Counter.u64TxMessages = 0U;
    m_Counter.u64RxMessages = 0U;
    m_Counter.u64ErrorFrames = 0U;
    // the PCANBasic interface
    m_pCAN = new SCAN();
}

EXPORT
CPeakCAN::~CPeakCAN() {
    delete m_pCAN;
}

EXPORT
CANAPI_Return_t CPeakCAN::ProbeChannel(int32_t channel, CANAPI_OpMode_t opMode, const void *param, EChannelState &state) {
    // test the CAN interface (hardware and driver)
    int result = CANBRD_NOT_TESTABLE;
    CANAPI_Return_t rc = can_test(channel, opMode.byte, param, &result);
    state = (EChannelState)result;
    return rc;
}

EXPORT
CANAPI_Return_t CPeakCAN::ProbeChannel(int32_t channel, CANAPI_OpMode_t opMode, EChannelState &state) {
    // delegate this function call
    return ProbeChannel(channel, opMode, NULL, state);
}

EXPORT
CANAPI_Return_t CPeakCAN::InitializeChannel(int32_t channel, can_mode_t opMode, const void *param) {
    // initialize the CAN interface
    CANAPI_Return_t rc = CANERR_FATAL;
    CANAPI_Handle_t hnd = can_init(channel, opMode.byte, param);
    if (0 <= hnd) {
        m_pCAN->m_Handle = hnd;  // we got a handle
        m_OpMode = opMode;
        rc = CANERR_NOERROR;
    } else {
        rc = (CANAPI_Return_t)hnd;
    }
    return rc;
}

EXPORT
CANAPI_Return_t CPeakCAN::TeardownChannel() {
    // shutdown the CAN interface
    CANAPI_Return_t rc = can_exit(m_pCAN->m_Handle);
    if (CANERR_NOERROR == rc) {
        m_pCAN->m_Handle = -1;  // invalidate the handle
    }
    return rc;
}

EXPORT
CANAPI_Return_t CPeakCAN::SignalChannel() {
    // signal waiting event objects of the CAN interface
    return can_kill(m_pCAN->m_Handle);
}

EXPORT
CANAPI_Return_t CPeakCAN::StartController(CANAPI_Bitrate_t bitrate) {
    // start the CAN controller with the given bit-rate settings
    CANAPI_Return_t rc = can_start(m_pCAN->m_Handle, &bitrate);
    if (CANERR_NOERROR == rc) {
        m_Bitrate = bitrate;
        memset(&m_Counter, 0, sizeof(m_Counter));
    }
    return rc;
}

EXPORT
CANAPI_Return_t CPeakCAN::ResetController() {
    // stop any operation of the CAN controller
    return can_reset(m_pCAN->m_Handle);
}

EXPORT
CANAPI_Return_t CPeakCAN::WriteMessage(CANAPI_Message_t message, uint16_t timeout) {
    // transmit a message over the CAN bus
    CANAPI_Return_t rc = can_write(m_pCAN->m_Handle, &message, timeout);
    if (CANERR_NOERROR == rc) {
        m_Counter.u64TxMessages++;
    }
    return rc;
}

EXPORT
CANAPI_Return_t CPeakCAN::ReadMessage(CANAPI_Message_t &message, uint16_t timeout) {
    // read one message from the message queue of the CAN interface, if any
    CANAPI_Return_t rc = can_read(m_pCAN->m_Handle, &message, timeout);
    if (CANERR_NOERROR == rc) {
        m_Counter.u64RxMessages += !message.sts ? 1U : 0U;
        m_Counter.u64ErrorFrames += message.sts ? 1U : 0U;
    }
    return rc;
}

EXPORT
CANAPI_Return_t CPeakCAN::GetStatus(CANAPI_Status_t &status) {
    // retrieve the status register of the CAN interface
    return can_status(m_pCAN->m_Handle, &status.byte);
}

EXPORT
CANAPI_Return_t CPeakCAN::GetBusLoad(uint8_t &load) {
    // retrieve the bus-load (in percent) of the CAN interface
    return can_busload(m_pCAN->m_Handle, &load, NULL);
}

EXPORT
CANAPI_Return_t CPeakCAN::GetBitrate(CANAPI_Bitrate_t &bitrate) {
    // retrieve the bit-rate setting of the CAN interface
    CANAPI_Return_t rc = can_bitrate(m_pCAN->m_Handle, &bitrate, NULL);
    if (CANERR_NOERROR == rc) {
        m_Bitrate = bitrate;
    }
    return rc;
}

EXPORT
CANAPI_Return_t CPeakCAN::GetBusSpeed(CANAPI_BusSpeed_t &speed) {
    // retrieve the transmission rate of the CAN interface
    return can_bitrate(m_pCAN->m_Handle, &m_Bitrate, &speed);
}

EXPORT
CANAPI_Return_t CPeakCAN::GetProperty(uint16_t param, void *value, uint32_t nbyte) {
    // backdoor access to the CAN handle (Careful with That Axe, Eugene)
    if (CANPROP_GET_CPP_BACKDOOR == param) {
        CANAPI_Return_t rc = CANERR_ILLPARA;
        if (NULL == value) {
            rc = CANERR_NULLPTR;
        }
        else if ((size_t)nbyte >= sizeof(int32_t)) {
            *(int32_t*)value = (int32_t)m_pCAN->m_Handle;
            rc = CANERR_NOERROR;
        }
        return rc;
    }
    // retrieve a property value of the CAN interface
    return can_property(m_pCAN->m_Handle, param, value, nbyte);
}

EXPORT
CANAPI_Return_t CPeakCAN::SetProperty(uint16_t param, const void *value, uint32_t nbyte) {
    // modify a property value of the CAN interface
    return can_property(m_pCAN->m_Handle, param, (void*)value, nbyte);
}

EXPORT
char *CPeakCAN::GetHardwareVersion() {
    // retrieve the hardware version of the CAN controller
    return can_hardware(m_pCAN->m_Handle);
}

EXPORT
char *CPeakCAN::GetFirmwareVersion() {
    // retrieve the firmware version of the CAN controller
    return can_software(m_pCAN->m_Handle);
}

EXPORT
char *CPeakCAN::GetVersion() {
    // get driver version
    return (char *)&version[0];
}

//  Methods for bit-rate conversion
//
EXPORT
CANAPI_Return_t CPeakCAN::MapIndex2Bitrate(int32_t index, CANAPI_Bitrate_t &bitrate) {
    return (CANAPI_Return_t)btr_index2bitrate(index, &bitrate);
}

EXPORT
CANAPI_Return_t CPeakCAN::MapString2Bitrate(const char *string, CANAPI_Bitrate_t &bitrate) {
    bool brse = false;
    // TODO: rework function 'btr_string2bitrate'
    return (CANAPI_Return_t)btr_string2bitrate((btr_string_t)string, &bitrate, &brse);
}

EXPORT
CANAPI_Return_t CPeakCAN::MapBitrate2String(CANAPI_Bitrate_t bitrate, char *string, size_t length) {
    (void) length;
    // TODO: rework function 'btr_bitrate2string'
    return (CANAPI_Return_t)btr_bitrate2string(&bitrate, false, (btr_string_t)string);
}

EXPORT
CANAPI_Return_t CPeakCAN::MapBitrate2Speed(CANAPI_Bitrate_t bitrate, CANAPI_BusSpeed_t &speed) {
    // TODO: rework function 'btr_bitrate2speed'
    return (CANAPI_Return_t)btr_bitrate2speed(&bitrate, false, false, &speed);
}

//  Private methodes
//
CANAPI_Return_t CPeakCAN::MapBitrate2Sja1000(CANAPI_Bitrate_t bitrate, uint16_t &btr0btr1) {
    return (CANAPI_Return_t)btr_bitrate2sja1000(&bitrate, &btr0btr1);
}

CANAPI_Return_t CPeakCAN::MapSja10002Bitrate(uint16_t btr0btr1, CANAPI_Bitrate_t &bitrate) {
    return (CANAPI_Return_t)btr_sja10002bitrate(btr0btr1, &bitrate);
}
