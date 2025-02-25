//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  CAN Monitor for PEAK PCAN Interfaces
//
//  Copyright (c) 2008-2010,2017-2021 Uwe Vogt, UV Software, Berlin (info@uv-software.com)
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "build_no.h"
#define VERSION_MAJOR    0
#define VERSION_MINOR    4
#define VERSION_PATCH    1
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
static const char APPLICATION[] = "CAN Monitor for PEAK PCAN Interfaces, Version " VERSION_STRING;
static const char COPYRIGHT[]   = "Copyright (c) 2008-2010,2017-2021 by Uwe Vogt, UV Software, Berlin";
static const char WARRANTY[]    = "This program comes with ABSOLUTELY NO WARRANTY!\n\n" \
                                  "This is free software, and you are welcome to redistribute it\n" \
                                  "under certain conditions; type `/ABOUT' for details.";
static const char LICENSE[]     = "This program is free software: you can redistribute it and/or modify\n" \
                                  "it under the terms of the GNU General Public License as published by\n" \
                                  "the Free Software Foundation, either version 3 of the License, or\n" \
                                  "(at your option) any later version.\n\n" \
                                  "This program is distributed in the hope that it will be useful,\n" \
                                  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
                                  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
                                  "GNU General Public License for more details.\n\n" \
                                  "You should have received a copy of the GNU General Public License\n" \
                                  "along with this program.  If not, see <http://www.gnu.org/licenses/>.";
#define basename(x)  "can_moni" // FIXME: Where is my `basename' function?

#include "PeakCAN_Defines.h"
#include "PeakCAN.h"
#include "Timer.h"
#include "Message.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include <inttypes.h>

#ifdef _MSC_VER
//not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#define MAX_ID  (CAN_MAX_STD_ID + 1)

extern "C" {
#include "dosopt.h"
}
#define BAUDRATE_STR    0
#define BAUDRATE_CHR    1
#define BITRATE_STR     2
#define BITRATE_CHR     3
#define VERBOSE_STR     4
#define VERBOSE_CHR     5
#define OP_MODE_STR     6
#define OP_MODE_CHR     7
#define OP_RTR_STR      8
#define OP_XTD_STR      9
#define OP_ERR_STR      10
#define OP_ERRFRMS_STR  11
#define OP_MON_STR      12
#define OP_MONITOR_STR  13
#define OP_LSTNONLY_STR 14
#define SHARED_STR      15
#define SHARED_CHR      16
#define MODE_TIME_STR   17
#define MODE_TIME_CHR   18
#define MODE_ID_STR     19
#define MODE_ID_CHR     20
#define MODE_DATA_STR   21
#define MODE_DATA_CHR   22
#define MODE_ASCII_STR  23
#define MODE_ASCII_CHR  24
#define WRAPAROUND_STR  25
#define WRAPAROUND_CHR  26
#define EXCLUDE_STR     27
#define EXCLUDE_CHR     28
#define SCRIPT_STR      29
#define SCRIPT_CHR      30
#define LISTBOARDS_STR  31
#define LISTBOARDS_CHR  32
#define TESTBOARDS_STR  33
#define TESTBOARDS_CHR  34
#define HELP            35
#define QUESTION_MARK   36
#define ABOUT           37
#define CHARACTER_MJU   38
#define MAX_OPTIONS     39

static char* option[MAX_OPTIONS] = {
    (char*)"BAUDRATE", (char*)"bd",
    (char*)"BITRATE", (char*)"br",
    (char*)"VERBOSE", (char*)"v",
    (char*)"MODE", (char*)"m",
    (char*)"RTR",
    (char*)"XTD",
    (char*)"ERR", (char*)"ERROR-FRAMES",
    (char*)"MON", (char*)"MONITOR", (char*)"LISTEN-ONLY",
    (char*)"SHARED", (char*)"shrd",
    (char*)"TIME", (char*)"t",
    (char*)"ID", (char*)"i",
    (char*)"DATA", (char*)"d",
    (char*)"ASCII", (char*)"a",
    (char*)"WARAPAROUND", (char*)"w",
    (char*)"EXCLUDE", (char*)"x",
    (char*)"SCRIPT", (char*)"s",
    (char*)"LIST-BOARDS", (char*)"list",
    (char*)"TEST-BOARDS", (char*)"test",
    (char*)"HELP", (char*)"?",
    (char*)"ABOUT", (char*)"�"
};

static int get_exclusion(const char *arg);  // TODO: make it a member function

class CCanDriver : public CPeakCAN {
public:
    uint64_t ReceptionLoop();
public:
    static int ListCanDevices(const char *vendor = NULL);
    static int TestCanDevices(CANAPI_OpMode_t opMode, const char *vendor = NULL);
    // list of CAN interface vendors
    static const struct TCanVendor {
        int32_t id;
        char *name;
    } m_CanVendors[];
    // list of CAN interface devices
    static const struct TCanDevice {
        int32_t library;
        int32_t adapter;
        char *name;
    } m_CanDevices[];
};
const CCanDriver::TCanVendor CCanDriver::m_CanVendors[] = {
    {PEAKCAN_LIBRARY_ID, (char *)"PEAK" },
    {EOF, NULL}
};
const CCanDriver::TCanDevice CCanDriver::m_CanDevices[] = {
    {PEAKCAN_LIBRARY_ID, PCAN_USB1, (char *)"PCAN-USB1" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB2, (char *)"PCAN-USB2" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB3, (char *)"PCAN-USB3" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB4, (char *)"PCAN-USB4" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB5, (char *)"PCAN-USB5" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB6, (char *)"PCAN-USB6" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB7, (char *)"PCAN-USB7" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB8, (char *)"PCAN-USB8" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB9, (char *)"PCAN-USB9" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB10, (char *)"PCAN-USB10" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB11, (char *)"PCAN-USB11" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB12, (char *)"PCAN-USB12" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB13, (char *)"PCAN-USB13" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB14, (char *)"PCAN-USB14" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB15, (char *)"PCAN-USB15" },
    {PEAKCAN_LIBRARY_ID, PCAN_USB16, (char *)"PCAN-USB16" },
    {EOF, EOF, NULL}
};

static void sigterm(int signo);
static void usage(FILE *stream, const char *program);
static void version(FILE *stream, const char *program);

static int can_id[MAX_ID];
static int can_id_xtd = 1;
static volatile int running = 1;

static CCanDriver canDriver = CCanDriver();

// TODO: this code could be made more C++ alike
int main(int argc, const char * argv[]) {
    int i;
    int optind;
    char *optarg;

    int channel = 0, hw = 0;
    int op = 0, rf = 0, xf = 0, ef = 0, lo = 0, sh = 0;
    int baudrate = CANBDR_250; int bd = 0;
    CCanMessage::EFormatTimestamp modeTime = CCanMessage::OptionZero; int mt = 0;
    CCanMessage::EFormatNumber modeId = CCanMessage::OptionHex; int mi = 0;
    CCanMessage::EFormatNumber modeData = CCanMessage::OptionHex; int md = 0;
    CCanMessage::EFormatOption modeAscii = CCanMessage::OptionOn; int ma = 0;
    CCanMessage::EFormatWraparound wraparound = CCanMessage::OptionWraparoundNo; int mw = 0;
    int exclude = 0;
//    char *script_file = NULL;
    int verbose = 0;
    int num_boards = 0;
    int show_version = 0;
    char *device, *firmware, *software;

    CANAPI_Bitrate_t bitrate = {};
    bitrate.index = CANBTR_INDEX_250K;
    CANAPI_OpMode_t opMode = {};
    opMode.byte = CANMODE_DEFAULT;
    CANAPI_Return_t retVal = 0;

    /* default bit-timing */
    CANAPI_BusSpeed_t speed = {};
    (void)CCanDriver::MapIndex2Bitrate(bitrate.index, bitrate);
    (void)CCanDriver::MapBitrate2Speed(bitrate, speed);
    (void)op;

    /* default format options */
    (void)CCanMessage::SetTimestampFormat(modeTime);
    (void)CCanMessage::SetIdentifierFormat(modeId);
    (void)CCanMessage::SetDataFormat(modeData);
    (void)CCanMessage::SetAsciiFormat(modeAscii);
    (void)CCanMessage::SetWraparound(wraparound);

    /* exclude list (11-bit IDs only) */
    for (int i = 0; i < MAX_ID; i++) {
        can_id[i] = 1;
    }
    /* signal handler */
    if ((signal(SIGINT, sigterm) == SIG_ERR) ||
#if !defined(_WIN32) && !defined(_WIN64)
       (signal(SIGHUP, sigterm) == SIG_ERR) ||
#endif
       (signal(SIGTERM, sigterm) == SIG_ERR)) {
        perror("+++ error");
        return errno;
    }
    /* scan command-line */
    while ((optind = getOption(argc, (char**)argv, MAX_OPTIONS, option)) != EOF) {
        switch (optind) {
        case BAUDRATE_STR:
        case BAUDRATE_CHR:
            if ((bd++)) {
                fprintf(stderr, "%s: duplicated option /BAUDRATE\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /BAUDRATE\n", basename(argv[0]));
                return 1;
            }
            if (sscanf_s(optarg, "%li", &baudrate) != 1) {
                fprintf(stderr, "%s: illegal argument for option /BAUDRATE\n", basename(argv[0]));
                return 1;
            }
            switch (baudrate) {
            case 1000: case 1000000: bitrate.index = CANBTR_INDEX_1M; break;
            case 800:  case 800000:  bitrate.index = CANBTR_INDEX_800K; break;
            case 500:  case 500000:  bitrate.index = CANBTR_INDEX_500K; break;
            case 250:  case 250000:  bitrate.index = CANBTR_INDEX_250K; break;
            case 125:  case 125000:  bitrate.index = CANBTR_INDEX_125K; break;
            case 100:  case 100000:  bitrate.index = CANBTR_INDEX_100K; break;
            case 50:   case 50000:   bitrate.index = CANBTR_INDEX_50K; break;
            case 20:   case 20000:   bitrate.index = CANBTR_INDEX_20K; break;
            case 10:   case 10000:   bitrate.index = CANBTR_INDEX_10K; break;
            default:                 bitrate.index = -baudrate; break;
            }
            if (CCanDriver::MapIndex2Bitrate(bitrate.index, bitrate) != CCANAPI::NoError) {
                fprintf(stderr, "%s: illegal argument for option /BAUDRATE\n", basename(argv[0]));
                return 1;
            }
            if (CCanDriver::MapBitrate2Speed(bitrate, speed) != CCANAPI::NoError) {
                fprintf(stderr, "%s: illegal argument for option /BAUDRATE\n", basename(argv[0]));
                return 1;
            }
            break;
        case BITRATE_STR:
        case BITRATE_CHR:
            if ((bd++)) {
                fprintf(stderr, "%s: duplicated option /BITRATE\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /BITRATE\n", basename(argv[0]));
                return 1;
            }
            if (CCanDriver::MapString2Bitrate(optarg, bitrate) != CCANAPI::NoError) {
                fprintf(stderr, "%s: illegal argument for option /BITRATE\n", basename(argv[0]));
                return 1;
            }
            if (CCanDriver::MapBitrate2Speed(bitrate, speed) != CCANAPI::NoError) {
                fprintf(stderr, "%s: illegal argument for option /BITRATE\n", basename(argv[0]));
                return 1;
            }
            break;
        case VERBOSE_STR:
        case VERBOSE_CHR:
            if (verbose) {
                fprintf(stderr, "%s: duplicated option /VERBOSE\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) != NULL) {
                fprintf(stderr, "%s: illegal argument for option /VERBOSE\n", basename(argv[0]));
                return 1;
            }
            verbose = 1;
            break;
        case OP_MODE_STR:
        case OP_MODE_CHR:
            if ((op++)) {
                fprintf(stderr, "%s: duplicated option /MODE\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /MODE\n", basename(argv[0]));
                return 1;
            }
            if (!_strcmpi(optarg, "DEFAULT") || !_strcmpi(optarg, "CLASSIC") ||
                !_strcmpi(optarg, "CAN20") || !_strcmpi(optarg, "CAN2.0") || !_strcmpi(optarg, "2.0"))
                opMode.byte |= CANMODE_DEFAULT;
            else if (!_strcmpi(optarg, "CANFD") || !_strcmpi(optarg, "FD") || !_strcmpi(optarg, "FDF"))
                opMode.byte |= CANMODE_FDOE;
            else if (!_strcmpi(optarg, "CANFD+BRS") || !_strcmpi(optarg, "FDF+BRS") || !_strcmpi(optarg, "FD+BRS"))
                opMode.byte |= CANMODE_FDOE | CANMODE_BRSE;
            else {
                fprintf(stderr, "%s: illegal argument for option /MODE\n", basename(argv[0]));
                return 1;
            }
            break;
        case OP_RTR_STR:
            if ((rf++)) {
                fprintf(stderr, "%s: duplicated option /RTR\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /RTR\n", basename(argv[0]));
                return 1;
            }
            if (!_strcmpi(optarg, "NO") || !_strcmpi(optarg, "N") || !_strcmpi(optarg, "OFF") || !_strcmpi(optarg, "0"))
                opMode.byte |= CANMODE_NRTR;
            else if (!_strcmpi(optarg, "YES") || !_strcmpi(optarg, "Y") || !_strcmpi(optarg, "ON") || !_strcmpi(optarg, "1"))
                opMode.byte &= ~CANMODE_NRTR;
            else {
                fprintf(stderr, "%s: illegal argument for option /RTR\n", basename(argv[0]));
                return 1;
            }
            break;
        case OP_XTD_STR:
            if ((xf++)) {
                fprintf(stderr, "%s: duplicated option /XTD\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /XTD\n", basename(argv[0]));
                return 1;
            }
            if (!_strcmpi(optarg, "NO") || !_strcmpi(optarg, "N") || !_strcmpi(optarg, "OFF") || !_strcmpi(optarg, "0"))
                opMode.byte |= CANMODE_NXTD;
            else if (!_strcmpi(optarg, "YES") || !_strcmpi(optarg, "Y") || !_strcmpi(optarg, "ON") || !_strcmpi(optarg, "1"))
                opMode.byte &= ~CANMODE_NXTD;
            else {
                fprintf(stderr, "%s: illegal argument for option /XTD\n", basename(argv[0]));
                return 1;
            }
            break;
        case OP_ERR_STR:
            if ((ef++)) {
                fprintf(stderr, "%s: duplicated option /ERR\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /ERR\n", basename(argv[0]));
                return 1;
            }
            if (!_strcmpi(optarg, "YES") || !_strcmpi(optarg, "Y") || !_strcmpi(optarg, "ON") || !_strcmpi(optarg, "1"))
                opMode.byte |= CANMODE_ERR;
            else if (!_strcmpi(optarg, "NO") || !_strcmpi(optarg, "N") || !_strcmpi(optarg, "OFF") || !_strcmpi(optarg, "0"))
                opMode.byte &= ~CANMODE_ERR;
            else {
                fprintf(stderr, "%s: illegal argument for option /ERR\n", basename(argv[0]));
                return 1;
            }
            break;
        case OP_ERRFRMS_STR:
            if ((ef++)) {
                fprintf(stderr, "%s: duplicated option /ERROR-FRAMES\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) != NULL) {
                fprintf(stderr, "%s: illegal argument for option /ERROR-FRAMES\n", basename(argv[0]));
                return 1;
            }
            opMode.byte |= CANMODE_ERR;
            break;
        case OP_MON_STR:
        case OP_MONITOR_STR:
            if ((lo++)) {
                fprintf(stderr, "%s: duplicated option /MON\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /MON\n", basename(argv[0]));
                return 1;
            }
            if (!_strcmpi(optarg, "YES") || !_strcmpi(optarg, "Y") || !_strcmpi(optarg, "ON") || !_strcmpi(optarg, "1"))
                opMode.byte |= CANMODE_MON;
            else if (!_strcmpi(optarg, "NO") || !_strcmpi(optarg, "N") || !_strcmpi(optarg, "OFF") || !_strcmpi(optarg, "0"))
                opMode.byte &= ~CANMODE_MON;
            else {
                fprintf(stderr, "%s: illegal argument for option /MON\n", basename(argv[0]));
                return 1;
            }
            break;
        case OP_LSTNONLY_STR:
            if ((lo++)) {
                fprintf(stderr, "%s: duplicated option /LISTEN-ONLY\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) != NULL) {
                fprintf(stderr, "%s: illegal argument for option /LISTEN-ONLY\n", basename(argv[0]));
                return 1;
            }
            opMode.byte |= CANMODE_MON;
            break;
        case SHARED_STR:
        case SHARED_CHR:
            if ((sh++)) {
                fprintf(stderr, "%s: duplicated option /SHARED\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) != NULL) {
                fprintf(stderr, "%s: illegal argument for option /SHARED\n", basename(argv[0]));
                return 1;
            }
            opMode.byte |= CANMODE_SHRD;
            break;
        case MODE_TIME_STR:
        case MODE_TIME_CHR:
            if ((mt++)) {
                fprintf(stderr, "%s: conflict for option /TIME\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /TIME\n", basename(argv[0]));
                return 1;
            }
            if (!strcasecmp(optarg, "ABSOLUTE") || !strcasecmp(optarg, "ABS") || !strcasecmp(optarg, "a"))
                modeTime = CCanMessage::OptionAbsolute;
            else if (!strcasecmp(optarg, "RELATIVE") || !strcasecmp(optarg, "REL") || !strcasecmp(optarg, "r"))
                modeTime = CCanMessage::OptionRelative;
            else if (!strcasecmp(optarg, "ZERO") || !strcasecmp(optarg, "0") || !strcasecmp(optarg, "z"))
                modeTime = CCanMessage::OptionZero;
            else {
                fprintf(stderr, "%s: illegal argument for option /TIME\n", basename(argv[0]));
                return 1;
            }
            if (!CCanMessage::SetTimestampFormat(modeTime)) {
                fprintf(stderr, "%s: illegal argument for option /TIME\n", basename(argv[0]));
                return 1;
            }
            break;
        case MODE_ID_STR:
        case MODE_ID_CHR:
            if ((mi++)) {
                fprintf(stderr, "%s: conflict for option /ID\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /ID\n", basename(argv[0]));
                return 1;
            }
            if (!strcasecmp(optarg, "HEXADECIMAL") || !strcasecmp(optarg, "HEX") || !strcasecmp(optarg, "h") || !strcasecmp(optarg, "16"))
                modeId = CCanMessage::OptionHex;
            else if (!strcasecmp(optarg, "DECIMAL") || !strcasecmp(optarg, "DEC") || !strcasecmp(optarg, "d") || !strcasecmp(optarg, "10"))
                modeId = CCanMessage::OptionDec;
            else if (!strcasecmp(optarg, "OCTAL") || !strcasecmp(optarg, "OCT") || !strcasecmp(optarg, "o") || !strcasecmp(optarg, "8"))
                modeId = CCanMessage::OptionOct;
            else {
                fprintf(stderr, "%s: illegal argument for option /ID\n", basename(argv[0]));
                return 1;
            }
            if (!CCanMessage::SetIdentifierFormat(modeId)) {
                fprintf(stderr, "%s: illegal argument for option /ID\n", basename(argv[0]));
                return 1;
            }
            break;
        case MODE_DATA_STR:
        case MODE_DATA_CHR:
            if ((md++)) {
                fprintf(stderr, "%s: conflict for option /DATA\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /DATA\n", basename(argv[0]));
                return 1;
            }
            if (!strcasecmp(optarg, "HEXADECIMAL") || !strcasecmp(optarg, "HEX") || !strcasecmp(optarg, "h") || !strcasecmp(optarg, "16"))
                modeData = CCanMessage::OptionHex;
            else if (!strcasecmp(optarg, "DECIMAL") || !strcasecmp(optarg, "DEC") || !strcasecmp(optarg, "d") || !strcasecmp(optarg, "10"))
                modeData = CCanMessage::OptionDec;
            else if (!strcasecmp(optarg, "OCTAL") || !strcasecmp(optarg, "OCT") || !strcasecmp(optarg, "o") || !strcasecmp(optarg, "8"))
                modeData = CCanMessage::OptionOct;
            else {
                fprintf(stderr, "%s: illegal argument for option /DATA\n", basename(argv[0]));
                return 1;
            }
            if (!CCanMessage::SetDataFormat(modeData)) {
                fprintf(stderr, "%s: illegal argument for option /DATA\n", basename(argv[0]));
                return 1;
            }
            break;
        case MODE_ASCII_STR:
        case MODE_ASCII_CHR:
            if ((ma++)) {
                fprintf(stderr, "%s: conflict for option /ASCII\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /ASCII\n", basename(argv[0]));
                return 1;
            }
            if (!strcasecmp(optarg, "OFF") || !strcasecmp(optarg, "NO") || !strcasecmp(optarg, "n") || !strcasecmp(optarg, "0"))
                modeAscii = CCanMessage::OptionOff;
            else if (!strcasecmp(optarg, "ON") || !strcasecmp(optarg, "YES") || !strcasecmp(optarg, "y") || !strcasecmp(optarg, "1"))
                modeAscii = CCanMessage::OptionOn;
            else {
                fprintf(stderr, "%s: illegal argument for option /ASCII\n", basename(argv[0]));
                return 1;
            }
            if (!CCanMessage::SetAsciiFormat(modeAscii)) {
                fprintf(stderr, "%s: illegal argument for option /ASCII\n", basename(argv[0]));
                return 1;
            }
            break;
        case WRAPAROUND_STR:
        case WRAPAROUND_CHR:
            if ((mw++)) {
                fprintf(stderr, "%s: conflict for option /WRAPAROUND\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /WRAPAROUND\n", basename(argv[0]));
                return 1;
            }
            if (!strcasecmp(optarg, "NO") || !strcasecmp(optarg, "n") || !strcasecmp(optarg, "0"))
                wraparound = CCanMessage::OptionWraparoundNo;
            else if (!strcasecmp(optarg, "8"))
                wraparound = CCanMessage::OptionWraparound8;
            else if (!strcasecmp(optarg, "10"))
                wraparound = CCanMessage::OptionWraparound10;
            else if (!strcasecmp(optarg, "16"))
                wraparound = CCanMessage::OptionWraparound16;
            else if (!strcasecmp(optarg, "32"))
                wraparound = CCanMessage::OptionWraparound32;
            else if (!strcasecmp(optarg, "64"))
                wraparound = CCanMessage::OptionWraparound64;
            else {
                fprintf(stderr, "%s: illegal argument for option /WRAPAROUND\n", basename(argv[0]));
                return 1;
            }
            if (!CCanMessage::SetWraparound(wraparound)) {
                fprintf(stderr, "%s: illegal argument for option /WRAPAROUND\n", basename(argv[0]));
                return 1;
            }
            break;
        case EXCLUDE_STR:
        case EXCLUDE_CHR:
            if ((exclude++)) {
                fprintf(stderr, "%s: conflict for option /EXCLUDE\n", basename(argv[0]));
                return 1;
            }
            if ((optarg = getOptionParameter()) == NULL) {
                fprintf(stderr, "%s: missing argument for option /EXCLUDE\n", basename(argv[0]));
                return 1;
            }
            if (!get_exclusion(optarg)) {
                fprintf(stderr, "%s: illegal argument for option /EXCLUDE\n", basename(argv[0]));
                return 1;
            }
            break;
        case LISTBOARDS_STR:
        case LISTBOARDS_CHR:
            fprintf(stdout, "%s\n%s\n\n%s\n\n", APPLICATION, COPYRIGHT, WARRANTY);
            /* list all supported interfaces */
            num_boards = CCanDriver::ListCanDevices(getOptionParameter());
            fprintf(stdout, "Number of supported CAN interfaces=%i\n", num_boards);
            return (num_boards >= 0) ? 0 : 1;
        case TESTBOARDS_STR:
        case TESTBOARDS_CHR:
            fprintf(stdout, "%s\n%s\n\n%s\n\n", APPLICATION, COPYRIGHT, WARRANTY);
            /* list all available interfaces */
            num_boards = CCanDriver::TestCanDevices(opMode, getOptionParameter());
            fprintf(stdout, "Number of present CAN interfaces=%i\n", num_boards);
            return (num_boards >= 0) ? 0 : 1;
        case HELP:
        case QUESTION_MARK:
            usage(stdout, basename(argv[0]));
            return 0;
        case ABOUT:
        case CHARACTER_MJU:
            version(stdout, basename(argv[0]));
            return 0;
        default:
            usage(stderr, basename(argv[0]));
            return 1;
        }
    }
    /* - check if one and only one <interface> is given */
    for (i = 1; i < argc; i++) {
        if (!isOption(argc, (char**)argv, MAX_OPTIONS, option, i)) {
            if ((hw++)) {
                fprintf(stderr, "%s: too many arguments\n", basename(argv[0]));
                return 1;
            }
            for (channel = 0; CCanDriver::m_CanDevices[channel].adapter != EOF; channel++) {
                if (!_stricmp(argv[i], CCanDriver::m_CanDevices[channel].name))
                    break;
            }
            if (CCanDriver::m_CanDevices[channel].adapter == EOF) {
                fprintf(stderr, "%s: illegal argument\n", basename(argv[0]));
                return 1;
            }
        }
    }
    if (!hw || ((CCanDriver::m_CanDevices[channel].adapter == EOF))) {
        fprintf(stderr, "%s: not enough arguments\n", basename(argv[0]));
        return 1;
    }
    /* - check bit-timing index (n/a for CAN FD) */
    if (opMode.fdoe && (bitrate.btr.frequency <= 0)) {
        fprintf(stderr, "%s: illegal combination of options /MODE and /BAUDRATE\n", basename(argv[0]));
        return 1;
    }
    /* CAN Monitor for PEAK PCAN interfaces */
    fprintf(stdout, "%s\n%s\n\n%s\n\n", APPLICATION, COPYRIGHT, WARRANTY);

    /* - show operation mode and bit-rate settings */
    if (verbose) {
        fprintf(stdout, "Op.-mode=%s", (opMode.byte & CANMODE_FDOE) ? "CANFD" : "CAN2.0");
        if ((opMode.byte & CANMODE_BRSE)) fprintf(stdout, "+BRS");
        if ((opMode.byte & CANMODE_NISO)) fprintf(stdout, "+NISO");
        if ((opMode.byte & CANMODE_SHRD)) fprintf(stdout, "+SHRD");
        if ((opMode.byte & CANMODE_NXTD)) fprintf(stdout, "+NXTD");
        if ((opMode.byte & CANMODE_NRTR)) fprintf(stdout, "+NRTR");
        if ((opMode.byte & CANMODE_ERR)) fprintf(stdout, "+ERR");
        if ((opMode.byte & CANMODE_MON)) fprintf(stdout, "+MON");
        fprintf(stdout, " (op_mode=%02Xh)\n", opMode.byte);
        if (bitrate.btr.frequency > 0) {
            fprintf(stdout, "Bit-rate=%.0fkbps@%.1f%%",
                speed.nominal.speed / 1000.,
                speed.nominal.samplepoint * 100.);
            if (speed.data.brse)
                fprintf(stdout, ":%.0fkbps@%.1f%%",
                    speed.data.speed / 1000.,
                    speed.data.samplepoint * 100.);
            fprintf(stdout, " (f_clock=%i,nom_brp=%u,nom_tseg1=%u,nom_tseg2=%u,nom_sjw=%u",
                bitrate.btr.frequency,
                bitrate.btr.nominal.brp,
                bitrate.btr.nominal.tseg1,
                bitrate.btr.nominal.tseg2,
                bitrate.btr.nominal.sjw);
            if (speed.data.brse)
                fprintf(stdout, ",data_brp=%u,data_tseg1=%u,data_tseg2=%u,data_sjw=%u",
                    bitrate.btr.data.brp,
                    bitrate.btr.data.tseg1,
                    bitrate.btr.data.tseg2,
                    bitrate.btr.data.sjw);
            else
                fprintf(stdout, ",nom_sam=%u", bitrate.btr.nominal.sam);
            fprintf(stdout, ")\n\n");
        }
        else {
            fprintf(stdout, "Baudrate=%.0fkbps@%.1f%% (index %i)\n\n",
                             speed.nominal.speed / 1000.,
                             speed.nominal.samplepoint * 100., -bitrate.index);
        }
    }
    /* - initialize interface */
    fprintf(stdout, "Hardware=%s...", CCanDriver::m_CanDevices[channel].name);
    fflush (stdout);
    retVal = canDriver.InitializeChannel(CCanDriver::m_CanDevices[channel].adapter, opMode);
    if (retVal != CCANAPI::NoError) {
        fprintf(stdout, "FAILED!\n");
        fprintf(stderr, "+++ error: CAN Controller could not be initialized (%i)\n", retVal);
        if (retVal == CCANAPI::NotSupported)
            fprintf(stderr, " - possibly CAN operating mode %02Xh not supported", opMode.byte);
        fputc('\n', stderr);
        goto finalize;
    }
    fprintf(stdout, "OK!\n");
    /* - start communication */
    if (bitrate.btr.frequency > 0) {
        fprintf(stdout, "Bit-rate=%.0fkbps",
            speed.nominal.speed / 1000.);
        if (speed.data.brse)
            fprintf(stdout, ":%.0fkbps",
                speed.data.speed / 1000.);
        fprintf(stdout, "...");
    }
    else {
        fprintf(stdout, "Baudrate=%skbps...",
            bitrate.index == CANBTR_INDEX_1M   ? "1000" :
            bitrate.index == CANBTR_INDEX_800K ? "800" :
            bitrate.index == CANBTR_INDEX_500K ? "500" :
            bitrate.index == CANBTR_INDEX_250K ? "250" :
            bitrate.index == CANBTR_INDEX_125K ? "125" :
            bitrate.index == CANBTR_INDEX_100K ? "100" :
            bitrate.index == CANBTR_INDEX_50K  ? "50" :
            bitrate.index == CANBTR_INDEX_20K  ? "20" :
            bitrate.index == CANBTR_INDEX_10K  ? "10" : "?");
    }
    fflush(stdout);
    retVal = canDriver.StartController(bitrate);
    if (retVal != CCANAPI::NoError) {
        fprintf(stdout, "FAILED!\n");
        fprintf(stderr, "+++ error: CAN Controller could not be started (%i)\n", retVal);
        goto teardown;
    }
    fprintf(stdout, "OK!\n");
    /* - do your job well: */
    canDriver.ReceptionLoop();
    /* - show interface information */
    if ((device = canDriver.GetHardwareVersion()) != NULL)
        fprintf(stdout, "Hardware: %s\n", device);
    if ((firmware = canDriver.GetFirmwareVersion()) != NULL)
        fprintf(stdout, "Firmware: %s\n", firmware);
    if ((software = CCanDriver::GetVersion()) != NULL)
        fprintf(stdout, "Software: %s\n", software);
teardown:
    /* - teardown the interface*/
    retVal = canDriver.TeardownChannel();
    if (retVal != CCANAPI::NoError) {
        fprintf(stderr, "+++ error: CAN Controller could not be reset (%i)\n", retVal);
        goto finalize;
    }
finalize:
    /* So long and farewell! */
    fprintf(stdout, "%s\n", COPYRIGHT);
    return retVal;
}

int CCanDriver::ListCanDevices(const char *vendor) {
    int32_t library = EOF; int n = 0;

    if (vendor != NULL) {
        /* search library ID in the vendor list */
        for (int32_t i = 0; CCanDriver::m_CanVendors[i].id != EOF; i++) {
            if (!strcmp(vendor, CCanDriver::m_CanVendors[i].name)) {
                library = CCanDriver::m_CanVendors[i].id;
                break;
            }
        }
        fprintf(stdout, "Suppored hardware from \"%s\":\n", vendor);
    }
    else
        fprintf(stdout, "Suppored hardware:\n");
    for (int32_t i = 0; CCanDriver::m_CanDevices[i].library != EOF; i++) {
        /* list all boards or from a specific vendor */
        if ((vendor == NULL) || (library == CCanDriver::m_CanDevices[i].library) ||
            !strcmp(vendor, "*")) { // TODO: pattern matching
            fprintf(stdout, "\"%s\" ", CCanDriver::m_CanDevices[i].name);
            /* search vendor name in the vendor list */
            for (int32_t j = 0; CCanDriver::m_CanVendors[j].id != EOF; j++) {
                if (CCanDriver::m_CanDevices[i].library == CCanDriver::m_CanVendors[j].id) {
                    fprintf(stdout, "(VendorName=\"%s\", LibraryId=%" PRIi32 ", AdapterId=%" PRIi32 ")",
                            CCanDriver::m_CanVendors[j].name, CCanDriver::m_CanDevices[i].library, CCanDriver::m_CanDevices[i].adapter);
                    break;
                }
            }
            fprintf(stdout, "\n");
            n++;
        }
    }
    return n;
}

int CCanDriver::TestCanDevices(CANAPI_OpMode_t opMode, const char *vendor) {
    int32_t library = EOF; int n = 0;

    if (vendor != NULL) {
        /* search library ID in the vendor list */
        for (int32_t i = 0; CCanDriver::m_CanVendors[i].id != EOF; i++) {
            if (!strcmp(vendor, CCanDriver::m_CanVendors[i].name)) {
                library = CCanDriver::m_CanVendors[i].id;
                break;
            }
        }
    }
    for (int32_t i = 0; CCanDriver::m_CanDevices[i].library != EOF; i++) {
        /* test all boards or from a specific vendor */
        if ((vendor == NULL) || (library == CCanDriver::m_CanDevices[i].library) ||
            !strcmp(vendor, "*")) { // TODO: pattern matching
            fprintf(stdout, "Hardware=%s...", CCanDriver::m_CanDevices[i].name);
            fflush(stdout);
            EChannelState state;
            CANAPI_Return_t retVal = CCanDriver::ProbeChannel(CCanDriver::m_CanDevices[i].adapter, opMode, state);
            if ((retVal == CCANAPI::NoError) || (retVal == CCANAPI::IllegalParameter)) {
                CTimer::Delay(333U * CTimer::MSEC);  // to fake probing a hardware
                switch (state) {
                    case CCANAPI::ChannelOccupied: fprintf(stdout, "occupied\n"); n++; break;
                    case CCANAPI::ChannelAvailable: fprintf(stdout, "available\n"); n++; break;
                    case CCANAPI::ChannelNotAvailable: fprintf(stdout, "not available\n"); break;
                    default: fprintf(stdout, "not testable\n"); break;
                }
                if (retVal == CCANAPI::IllegalParameter)
                    fprintf(stderr, "+++ warning: CAN operation mode not supported (%02x)\n", opMode.byte);
            } else
                fprintf(stdout, "FAILED!\n");
        }
    }
    return n;
}

uint64_t CCanDriver::ReceptionLoop() {
    CANAPI_Message_t message;
    CANAPI_Return_t retVal;
    uint64_t frames = 0U;

    char string[CANPROP_MAX_STRING_LENGTH+1];
    memset(string, 0, CANPROP_MAX_STRING_LENGTH+1);

    fprintf(stderr, "\nPress ^C to abort.\n\n");
    while(running) {
        if ((retVal = ReadMessage(message)) == CCANAPI::NoError) {
            if ((((message.id < MAX_ID) && can_id[message.id]) || ((message.id >= MAX_ID) && can_id_xtd)) &&
                !message.sts) {
                (void)CCanMessage::Format(message, ++frames, string, CANPROP_MAX_STRING_LENGTH);
                fprintf(stdout, "%s\n", string);
            }
        }
    }
    fprintf(stdout, "\n");
    return frames;
}

static int get_exclusion(const char *arg)
{
    char *val, *end;
    int i, inv = 0;
    long id, last = -1;

    if (!arg)
        return 0;

    val = (char *)arg;
    if (*val == '~') {
        inv = 1;
        val++;
    }
    for (;;) {
        errno = 0;
        id = strtol(val, &end, 0);

        if (errno == ERANGE && (id == LONG_MAX || id == LONG_MIN))
            return 0;
        if (errno != 0 && id == 0)
            return 0;
        if (val == end)
            return 0;

        if (id < MAX_ID)
            can_id[id] = 0;

        if (*end == '\0') {
            if (last != -1) {
                while (last != id) {
                    if (last < id)
                        last++;
                    else
                        last--;
                    can_id[last] = 0;
                }
                /*last = -1; <<< dead store */
            }
            break;
        }
        if (*end == ',') {
            if (last != -1) {
                while (last != id) {
                    if (last < id)
                        last++;
                    else
                        last--;
                    can_id[last] = 0;
                }
                last = -1;
            }
        }
        else if (*end == '-')
            last = id;
        else
            return 0;

        val = ++end;
    }
    if (inv) {
        for (i = 0; i < MAX_ID; i++)
            can_id[i] = !can_id[i];
    }
    can_id_xtd = !inv;
    return 1;
}

/** @brief       signal handler to catch Ctrl+C.
 *
 *  @param[in]   signo - signal number (SIGINT, SIGHUP, SIGTERM)
 */
static void sigterm(int signo)
{
    //fprintf(stderr, "%s: got signal %d\n", __FILE__, signo);
    (void)canDriver.SignalChannel();
    running = 0;
    (void)signo;
}

/** @brief       shows a help screen with all command-line options.
 *
 *  @param[in]   stream  - output stream (e.g. stdout)
 *  @param[in]   program - base name of the program
 */
static void usage(FILE *stream, const char *program)
{
    fprintf(stream, "Usage:\n");
    fprintf(stream, "  %-8s <interface>  [/Time=(ZERO|ABS|REL)]\n", program);
    fprintf(stream, "  %-8s              [/Id=(HEX|DEC|OCT)]\n", "");
    fprintf(stream, "  %-8s              [/Data=(HEX|DEC|OCT)]\n", "");
    fprintf(stream, "  %-8s              [/Ascii=(ON|OFF)]\n", "");
    fprintf(stream, "  %-8s              [/Wraparound=(No|8|10|16|32|64)]\n", "");
    fprintf(stream, "  %-8s              [/eXclude=[~]<id>[-<id>]{,<id>[-<id>]}]\n", "");
    //fprintf(stream, "  %-8s              [/Script=<filename>]\n", "");
    fprintf(stream, "  %-8s              [/RTR=(Yes|No)] [/XTD=(Yes|No)]\n", "");
    fprintf(stream, "  %-8s              [/ERR=(No|Yes) | /ERROR-FRAMES]\n", "");
    fprintf(stream, "  %-8s              [/MONitor=(No|Yes) | /LISTEN-ONLY]\n", "");
    fprintf(stream, "  %-8s              [/Mode=(2.0|FDf[+BRS])] [/SHARED] [/Verbose]\n", "");
    fprintf(stream, "  %-8s              [/BauDrate=<baudrate> | /BitRate=<bitrate>]\n", "");
#if (OPTION_CANAPI_LIBRARY != 0)
    fprintf(stream, "  %-8s (/TEST-BOARDS[=<vendor>] | /TEST[=<vendor>])\n", program);
    fprintf(stream, "  %-8s (/LIST-BOARDS[=<vendor>] | /LIST[=<vendor>])\n", program);
#else
    fprintf(stream, "  %-8s (/TEST-BOARDS | /TEST)\n", program);
    fprintf(stream, "  %-8s (/LIST-BOARDS | /LIST)\n", program);
#endif
    fprintf(stream, "  %-8s (/HELP  | /?)\n", program);
    fprintf(stream, "  %-8s (/ABOUT | /�)\n", program);
    fprintf(stream, "Options:\n");
    fprintf(stream, "  <id>        CAN identifier (11-bit)\n");
    fprintf(stream, "  <interface> CAN interface board (list all with /LIST)\n");
    fprintf(stream, "  <baudrate>  CAN baud rate index (default=3):\n");
    fprintf(stream, "              0 = 1000 kbps\n");
    fprintf(stream, "              1 = 800 kbps\n");
    fprintf(stream, "              2 = 500 kbps\n");
    fprintf(stream, "              3 = 250 kbps\n");
    fprintf(stream, "              4 = 125 kbps\n");
    fprintf(stream, "              5 = 100 kbps\n");
    fprintf(stream, "              6 = 50 kbps\n");
    fprintf(stream, "              7 = 20 kbps\n");
    fprintf(stream, "              8 = 10 kbps\n");
    fprintf(stream, "  <bitrate>   Comma-separated <key>=<value>-list:\n");
    fprintf(stream, "              f_clock=<value>      Frequency in Hz or\n");
    fprintf(stream, "              f_clock_mhz=<value>  Frequency in MHz\n");
    fprintf(stream, "              nom_brp=<value>      Bit-rate prescaler (nominal)\n");
    fprintf(stream, "              nom_tseg1=<value>    Time segment 1 (nominal)\n");
    fprintf(stream, "              nom_tseg2=<value>    Time segment 2 (nominal)\n");
    fprintf(stream, "              nom_sjw=<value>      Sync. jump width (nominal)\n");
    fprintf(stream, "              nom_sam=<value>      Sampling (only SJA1000)\n");
    fprintf(stream, "              data_brp=<value>     Bit-rate prescaler (FD data)\n");
    fprintf(stream, "              data_tseg1=<value>   Time segment 1 (FD data)\n");
    fprintf(stream, "              data_tseg2=<value>   Time segment 2 (FD data)\n");
    fprintf(stream, "              data_sjw=<value>     Sync. jump width (FD data).\n");
    fprintf(stream, "Hazard note:\n");
    fprintf(stream, "  If you connect your CAN device to a real CAN network when using this program,\n");
    fprintf(stream, "  you might damage your application.\n");
}

/** @brief       shows version information of the program.
 *
 *  @param[in]   stream  - output stream (e.g. stdout)
 *  @param[in]   program - base name of the program
 */
static void version(FILE *stream, const char *program)
{
    fprintf(stdout, "%s\n%s\n\n%s\n\n", APPLICATION, COPYRIGHT, LICENSE);
    (void)program;
    fprintf(stream, "Written by Uwe Vogt, UV Software, Berlin <http://www.uv-software.com/>\n");
}
