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

/* $Header:   F:\projects\c&c\vcs\code\ipxconn.cpv   1.9   16 Oct 1995 16:50:52   JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPXCONN.CPP                              *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 20, 1994                        *
 *                                                                         *
 *                  Last Update : April 9, 1995 [BRR]								*
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:    			                                                   *
 *   UDPConnClass::UDPConnClass -- class constructor           				*
 *   UDPConnClass::~UDPConnClass -- class destructor           				*
 *   UDPConnClass::Init -- hardware-specific initialization routine			*
 *   UDPConnClass::Configure -- One-time initialization routine				*
 *   UDPConnClass::Start_Listening -- commands IPX to listen               *
 *   UDPConnClass::Stop_Listening -- commands IPX to stop  listen          *
 *   UDPConnClass::Send -- sends a packet; invoked by SequencedConnection	*
 *   UDPConnClass::Open_Socket -- opens communications socket    				*
 *   UDPConnClass::Close_Socket -- closes the socket 								*
 *   UDPConnClass::Send_To -- sends the packet to the given address			*
 *   UDPConnClass::Broadcast -- broadcasts the given packet						*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "FUNCTION.H"
#include "IPX95.H"
#include "TCPIP.H"

#include <stdio.h>
#include <stdlib.h>

/*
********************************* Globals ***********************************
*/
unsigned short 	UDPConnClass::Socket;
int			 		UDPConnClass::ConnectionNum;
ECBType *			UDPConnClass::ListenECB;
IPXHeaderType *	UDPConnClass::ListenHeader;
char *				UDPConnClass::ListenBuf;
ECBType *			UDPConnClass::SendECB;
IPXHeaderType *	UDPConnClass::SendHeader;
char *				UDPConnClass::SendBuf;
uint32_t 			UDPConnClass::Handler;
int 					UDPConnClass::Configured = 0;
int 					UDPConnClass::SocketOpen = 0;
int 					UDPConnClass::Listening = 0;
int					UDPConnClass::PacketLen;


/***************************************************************************
 * UDPConnClass::UDPConnClass -- class constructor             				*
 *                                                                         *
 * INPUT:                                                                  *
 *		numsend			desired # of entries for the send queue					*
 *		numreceive		desired # of entries for the recieve queue				*
 *		maxlen			max length of an application packet							*
 *		magicnum			the packet "magic number" for this connection			*
 *		address			address of destination (NULL = no address)				*
 *		id					connection's unique numerical ID								*
 *		name				connection's name													*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
UDPConnClass::UDPConnClass (int numsend, int numreceive, int maxlen,
	unsigned short magicnum, UDPAddressClass *address, int id, char const *name) :
#ifdef SEQ_NET
	SequencedConnClass (numsend, numreceive, maxlen, magicnum,
#else
	NonSequencedConnClass (numsend, numreceive, maxlen, magicnum,
#endif
		2,			// retry delta
		-1,		// max retries
		60)		// timeout
{
	NetNumType net;
	NetNodeType node;

	/*------------------------------------------------------------------------
	Save the values passed in
	------------------------------------------------------------------------*/
	if (address)
		Address = (*address);
	ID = id;
	strcpy (Name, name);

	if (!Winsock.Get_Connected()){
		/*------------------------------------------------------------------------
		If our Address field is an actual address (ie NULL wasn't passed to the
		constructor), pre-compute the ImmediateAddress value for the SendECB.
		This allows pre-computing of the ImmediateAddress for all connections
		created after Configure() is called.
		------------------------------------------------------------------------*/
		if (!Address.Is_Broadcast() && Configured==1) {
			Address.Get_Address(net,node);
			/*.....................................................................
			If the user is logged in & has a valid Novell Connection Number, get the
			bridge address the "official" way
			.....................................................................*/
			if (ConnectionNum != 0) {
				if (IPX_Get_Local_Target (net, node, Socket, ImmediateAddress)!=0)
					memcpy(ImmediateAddress,node,6);
			} else {

				/*.....................................................................
				Otherwise, use the destination node address as the ImmediateAddress, and
				just hope there's no network bridge in the path.
				.....................................................................*/
				memcpy(ImmediateAddress,node,6);
			}

			Immed_Set = 1;
		} else {
			memset (ImmediateAddress, 0, 6);
			Immed_Set = 0;
		}
	}

}	/* end of UDPConnClass */


/***************************************************************************
 * UDPConnClass::Init -- hardware-specific initialization routine				*
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
void UDPConnClass::Init (void)
{
	/*------------------------------------------------------------------------
	Invoke the parent's Init routine
	------------------------------------------------------------------------*/
#ifdef SEQ_NET
	SequencedConnClass::Init();
#else
	NonSequencedConnClass::Init();
#endif
}


/***************************************************************************
 * UDPConnClass::Configure -- One-time initialization routine					*
 *                                                                         *
 * This routine sets up static members that are shared by all IPX				*
 * connections (ie those variables used by the Send/Listen/Broadcast			*
 * routines).																					*
 *																									*
 * INPUT:                                                                  *
 *		socket				socket ID for sending & receiving						*
 *		conn_num				local IPX Connection Number (0 = not logged in)		*
 *		listen_ecb			ptr to ECBType for listening								*
 *		send_ecb				ptr to ECBType for sending									*
 *		listen_header		ptr to IPXHeaderType for listening						*
 *		send_header			ptr to IPXHeaderType for sending							*
 *		listen_buf			ptr to buffer for listening								*
 *		send_buf				ptr to buffer for sending									*
 *		handler_rm_ptr		REAL-MODE pointer to event service routine			*
 *								(high word = segment, low word = offset)				*
 *		maxpacketlen		max packet size to listen for								*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		- All pointers must be protected-mode pointers, but must point to		*
 *		  DOS real-mode memory (except the Handler segment/offset)				*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
void UDPConnClass::Configure (unsigned short socket, int conn_num,
	ECBType *listen_ecb, ECBType *send_ecb, IPXHeaderType *listen_header,
	IPXHeaderType *send_header, char *listen_buf, char *send_buf,
	uint32_t handler_rm_ptr, int maxpacketlen)
{
	/*------------------------------------------------------------------------
	Save the values passed in
	------------------------------------------------------------------------*/
	Socket = socket;
	ConnectionNum = conn_num;
	ListenECB = listen_ecb;
	SendECB = send_ecb;
	ListenHeader = listen_header;
	SendHeader = send_header;
	ListenBuf = listen_buf;
	SendBuf = send_buf;
	Handler = handler_rm_ptr;
	PacketLen = maxpacketlen;

	Configured = 1;

}	/* end of Configure */


/***************************************************************************
 * UDPConnClass::Start_Listening -- commands IPX to listen                 *
 *                                                                         *
 * This routine may be used to start listening in polled mode (if the 		*
 * ECB's Event_Service_Routine is NULL), or in interrupt mode; it's			*
 * up to the caller to fill the ECB in.  If in polled mode, Listening		*
 * must be restarted every time a packet comes in.									*
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		- The ListenECB must have been properly filled in by the IPX Manager.*
 *		- Configure must be called before calling this routine.					*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
bool UDPConnClass::Start_Listening(void)
{

#ifndef NOT_FOR_WIN95

	if (Winsock.Get_Connected()) return (true);

	/*------------------------------------------------------------------------
	Open the Socket
	------------------------------------------------------------------------*/
	if (!Open_Socket(Socket))
		return(false);

	if (IPX_Start_Listening95()){
		Listening =1;
		return (TRUE);
	}else{
		return (FALSE);
	}

#else

	void *hdr_ptr;
	uint32_t hdr_val;
	void *buf_ptr;
	uint32_t buf_val;
	int rc;

	/*------------------------------------------------------------------------
	Don't do a thing unless we've been configured, and we're not listening.
	------------------------------------------------------------------------*/
	if (Configured==0 || Listening==1)
		return(false);

	/*------------------------------------------------------------------------
	Open the Socket
	------------------------------------------------------------------------*/
	if (!Open_Socket(Socket))
		return(false);

	/*------------------------------------------------------------------------
	Clear the ECB & header
	------------------------------------------------------------------------*/
	memset(ListenECB, 0, sizeof(ECBType));
	memset(ListenHeader, 0, sizeof(IPXHeaderType));

	/*------------------------------------------------------------------------
	Convert protected-mode ptrs to real-mode ptrs
	------------------------------------------------------------------------*/
	hdr_val = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ListenHeader));
	hdr_ptr = reinterpret_cast<void *>(((hdr_val & 0xffff0U) << 12) | (hdr_val & 0x000fU));

	buf_val = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ListenBuf));
	buf_ptr = reinterpret_cast<void *>(((buf_val & 0xffff0U) << 12) | (buf_val & 0x000fU));

	/*------------------------------------------------------------------------
	Fill in the ECB
	------------------------------------------------------------------------*/
	ListenECB->SocketNumber = Socket;
	ListenECB->PacketCount = 2;
	ListenECB->Packet[0].Address = hdr_ptr;
	ListenECB->Packet[0].Length = sizeof(IPXHeaderType);
	ListenECB->Packet[1].Address = buf_ptr;
	ListenECB->Packet[1].Length = (unsigned short)PacketLen;

	reinterpret_cast<uint32_t &>(ListenECB->Event_Service_Routine) = Handler;

	/*------------------------------------------------------------------------
	Command IPX to listen
	------------------------------------------------------------------------*/
	rc = IPX_Listen_For_Packet(ListenECB);
	if (rc!=0) {
		Close_Socket(Socket);
		return(false);
	} else {
		Listening = 1;
		return(true);
	}

#endif	//NOT_FOR_WIN95
}	/* end of Start_Listening */


/***************************************************************************
 * UDPConnClass::Stop_Listening -- commands IPX to stop  listen            *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		- This routine MUST NOT be called if IPX is not listening already!	*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
bool UDPConnClass::Stop_Listening(void)
{
	/*------------------------------------------------------------------------
	Don't do anything unless we're already Listening.
	------------------------------------------------------------------------*/
	if (Listening==0)
		return(false);

#ifndef NOT_FOR_WIN95

	if (Winsock.Get_Connected()){
		Listening = 0;
		return (true);
	}else{
		IPX_Shut_Down95();
		Close_Socket(Socket);
	}

#else	//NOT_FOR_WIN95

	/*------------------------------------------------------------------------
	Shut IPX down.
	------------------------------------------------------------------------*/
	IPX_Cancel_Event(ListenECB);
	Close_Socket(Socket);

#endif	//NOT_FOR_WIN95

	Listening = 0;

	/*------------------------------------------------------------------------
	All done.
	------------------------------------------------------------------------*/
	return(true);

}	/* end of Stop_Listening */


/***************************************************************************
 * UDPConnClass::Send -- sends a packet; invoked by SequencedConnection		*
 *                                                                         *
 * INPUT:                                                                  *
 *		socket		desired socket ID number											*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error																		*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int UDPConnClass::Send(char *buf, int buflen)
{
	/*------------------------------------------------------------------------
	Invoke our own Send_To routine, filling in our Address as the destination.
	------------------------------------------------------------------------*/
	if (Immed_Set) {
		return(Send_To (buf, buflen, &Address, ImmediateAddress));
	} else {
		return(Send_To (buf, buflen, &Address, NULL));
	}

}	/* end of Send */


/***************************************************************************
 * UDPConnClass::Open_Socket -- opens communications socket    				*
 *                                                                         *
 * INPUT:                                                                  *
 *		socket		desired socket ID number											*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error																		*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int UDPConnClass::Open_Socket(unsigned short socket)
{
	int rc;

	if (Winsock.Get_Connected()){
		SocketOpen = 1;
		return (true);
	}

	SocketOpen = 0;

	/*------------------------------------------------------------------------
	Try to open a listen socket.  The socket may have been left open by
	a previously-crashed program, so ignore the state of the SocketOpen
	flag for this call; use IPX to determine if the socket was already open.
	------------------------------------------------------------------------*/
	rc = IPX_Open_Socket(socket);
	if (rc) {
		/*
		................. If already open, close & reopen it ..................
		*/
		if (rc==IPXERR_SOCKET_ERROR) {
			IPX_Close_Socket(socket);
			rc = IPX_Open_Socket(socket);
			/*
			.................. Still can't open: return error ..................
			*/
			if (rc) {
				return(false);
			}
		}
	}

	SocketOpen = 1;

	return(true);
}


/***************************************************************************
 * UDPConnClass::Close_Socket -- closes the socket 								*
 *                                                                         *
 * INPUT:                                                                  *
 *		socket		desired socket ID number											*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		Calling this routine when the sockets aren't open may crash!			*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
void UDPConnClass::Close_Socket(unsigned short socket)
{
	if (Winsock.Get_Connected()){
		SocketOpen = 0;
		return;
	}
	/*------------------------------------------------------------------------
	Never, ever, ever, under any circumstances whatsoever, close a socket
	that isn't open.  You'll regret it forever (or until at least until you're
	through rebooting, which, if you're on a Pentium is the same thing).
	------------------------------------------------------------------------*/
	if (SocketOpen==1)
		IPX_Close_Socket(socket);

	SocketOpen = 0;

}	/* end of Close_Socket */


/***************************************************************************
 * UDPConnClass::Send_To -- sends the packet to the given address				*
 *                                                                         *
 * The "ImmediateAddress" field of the SendECB must be filled in with the	*
 * address of a bridge, or the node address of the destination if there		*
 * is no bridge.  The NETX call to find this address will always crash		*
 * if NETX isn't loaded (ConnectionNum is 0), so this case is trapped &		*
 * prevented.																					*
 * Also, if the address of this IPX connection is known when the 				*
 * constructor is called, and Configure has been called, Get_Local_Target 	*
 * is called to precompute the ImmediateAddress; this case is detected & 	*
 * if the value is already computed, it's just memcpy'd over.					*
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to send															*
 *		buflen		length of buffer														*
 *		address		Address to send to													*
 *		immed			ImmediateAddress value, NULL if none							*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error																		*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int UDPConnClass::Send_To(char *buf, int buflen, UDPAddressClass *address,
	NetNodeType immed)
{
	NetNumType net;
	NetNodeType node;
	int rc;

	unsigned char	send_address[6];

	if (Winsock.Get_Connected()){
		if (getenv("TD_TRACE_NETWORK") != NULL) {
			fprintf(stderr, "[udpconn] Send_To via Winsock buflen=%d\n", buflen);
			fflush(stderr);
		}

#ifdef VIRTUAL_SUBNET_SERVER
		if (immed){
			memcpy(send_address, immed, 6);
		}else{
			address->Get_Address(net,node);
			memcpy (send_address, node, 6);
		}
		/*
		** Use first two bytes of ipx address as target mask
		*/
		unsigned short *maskptr = (unsigned short*)&send_address[0];
		target_mask = *maskptr;

		char *tempsend = new char [buflen + sizeof (target_mask)];

		*(unsigned short*)tempsend = htons(target_mask);
		memcpy (tempsend+2, buf, buflen);

		Winsock.Write((void*)tempsend, buflen + sizeof (target_mask));
		delete [] tempsend;
#else	// VIRTUAL_SUBNET_SERVER
		Winsock.Write((void*)buf, buflen);
#endif	// VIRTUAL_SUBNET_SERVER

		return (true);
	}



	if (immed) {
		memcpy(send_address, immed, 6);
		//memcpy(node, immed, 6);
		//memset (net, 0, sizeof(net) );
		address->Get_Address(net,node);
	} else {
		address->Get_Address(net,node);
		/*.....................................................................
		If the user is logged in & has a valid Novell Connection Number, get the
		bridge address the "official" way
		.....................................................................*/
		if (ConnectionNum != 0) {
			rc = IPX_Get_Local_Target (net, node, Socket, &send_address[0]);
			if (rc!=0) {
				return(false);
			}
		} else {
			/*.....................................................................
			Otherwise, use the destination node address as the ImmediateAddress, and
			just hope there's no network bridge in the path.
			.....................................................................*/
			memcpy(send_address,node,6);
		}
	}

	return (IPX_Send_Packet95(&send_address[0], (unsigned char*)buf, buflen, (unsigned char*)net, (unsigned char*)node));


}
/***************************************************************************
 * UDPConnClass::Broadcast -- broadcasts the given packet						*
 *                                                                         *
 * INPUT:                                                                  *
 *		socket		desired socket ID number											*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error																		*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int UDPConnClass::Broadcast(char *buf, int buflen)
{

	if (Winsock.Get_Connected()){
		if (getenv("TD_TRACE_NETWORK") != NULL) {
			fprintf(stderr, "[udpconn] Broadcast via Winsock buflen=%d\n", buflen);
			fflush(stderr);
		}
#ifdef VIRTUAL_SUBNET_SERVER
		char *tempsend = new char [buflen + sizeof (unsigned short)];
		memcpy (tempsend+2, buf, buflen);
		*tempsend = 0;
		*(tempsend+1) = 0;

		Winsock.Write((void*)tempsend, buflen + sizeof (unsigned short));
		delete [] tempsend;
#else	// VIRTUAL_SUBNET_SERVER
		Winsock.Write((void*)buf, buflen);
#endif	// VIRTUAL_SUBNET_SERVER
		return(true);
	}else{
		return (IPX_Broadcast_Packet95((unsigned char*)buf, buflen));
	}

}
