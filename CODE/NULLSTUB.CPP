/*
**	Command & Conquer(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FUNCTION.H"
#include "NULLMGR.H"

void (*NullModemClass::OrigAbortModemFunc)(int) = nullptr;
KeyNumType NullModemClass::Input = KN_NONE;
GadgetClass* NullModemClass::Commands = nullptr;

NullModemClass::NullModemClass(int numsend, int numreceive, int maxlen, unsigned short magicnum)
	: BuildBuf(nullptr)
	, MaxLen(maxlen)
	, EchoBuf(nullptr)
	, EchoSize(0)
	, EchoCount(0)
	, OldIRQPri(0)
	, ModemVerboseOn(false)
	, ModemEchoOn(false)
	, ModemWaitCarrier(0)
	, ModemCarrierDetect(0)
	, ModemCarrierLoss(0)
	, ModemHangupDelay(0)
	, ModemGuardTime(0)
	, ModemEscapeCode('+')
	, Connection(nullptr)
	, NumConnections(0)
	, Port(nullptr)
	, PortHandle(nullptr)
	, NumSend(numsend)
	, NumReceive(numreceive)
	, MagicNum(magicnum)
	, RXBuf(nullptr)
	, RXSize(0)
	, RXCount(0)
	, RetryDelta(0)
	, MaxRetries(0)
	, Timeout(0)
	, SendOverflows(0)
	, ReceiveOverflows(0)
	, CRCErrors(0)
{
}

NullModemClass::~NullModemClass() = default;

int NullModemClass::Init(int, int, char*, int, char, int, int, int) { return 0; }
int NullModemClass::Delete_Connection(void) { return 0; }
int NullModemClass::Num_Connections(void) { return 0; }
int NullModemClass::Init_Send_Queue(void) { return 0; }
void NullModemClass::Shutdown(void) {}
void NullModemClass::Set_Timing(uint32_t retrydelta, uint32_t maxretries, uint32_t timeout)
{
	RetryDelta = retrydelta;
	MaxRetries = maxretries;
	Timeout = timeout;
}
int NullModemClass::Send_Message(void*, int, int) { return 0; }
int NullModemClass::Get_Message(void*, int*) { return 0; }
int NullModemClass::Service(void) { return 0; }
int NullModemClass::Num_Send(void) { return 0; }
int NullModemClass::Num_Receive(void) { return 0; }
uint32_t NullModemClass::Response_Time(void) { return 0; }
void NullModemClass::Reset_Response_Time(void) {}
void* NullModemClass::Oldest_Send(void) { return nullptr; }
void NullModemClass::Configure_Debug(int, int, int, char**, int) {}
void NullModemClass::Mono_Debug_Print(int, int) {}
DetectPortType NullModemClass::Detect_Port(SerialSettingsType*) { return PORT_INVALID; }
int NullModemClass::Detect_Modem(SerialSettingsType*, bool) { return 0; }
DialStatusType NullModemClass::Dial_Modem(char*, DialMethodType, bool) { return DIAL_ERROR; }
DialStatusType NullModemClass::Answer_Modem(bool) { return DIAL_ERROR; }
bool NullModemClass::Hangup_Modem(void) { return false; }
void NullModemClass::Setup_Modem_Echo(void (*)(char)) {}
void NullModemClass::Remove_Modem_Echo(void) {}
void NullModemClass::Print_EchoBuf(void) {}
void NullModemClass::Reset_EchoBuf(void) {}
int NullModemClass::Abort_Modem(void) { return 0; }
void NullModemClass::Setup_Abort_Modem(void) {}
void NullModemClass::Remove_Abort_Modem(void) {}
int NullModemClass::Change_IRQ_Priority(int) { return 0; }
int NullModemClass::Get_Modem_Status(void) { return 0; }
int NullModemClass::Send_Modem_Command(char*, char, char*, int, int, int) { return MODEM_CMD_ERROR; }
int NullModemClass::Verify_And_Convert_To_Int(char*) { return 0; }

void Shutdown_Modem(void) {}
void Modem_Signoff(void) {}
int Reconnect_Modem(void) { return 0; }
void Destroy_Null_Connection(int, int) {}

GameType Select_Serial_Dialog(void)
{
	return GAME_NORMAL;
}

int Com_Scenario_Dialog(void)
{
	return 0;
}

int Com_Show_Scenario_Dialog(void)
{
	return 0;
}
