/*
**	Command & Conquer(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	Small native WebSocket client for the modern WOL backend.
**	It uses CODE/SOCKETS.H for TCP I/O and exposes only binary messages.
*/
#ifndef WSCLIENT_H
#define WSCLIENT_H

#include <cstddef>

class WSClientClass
{
	public:
		enum StateType {
			STATE_DISCONNECTED = 0,
			STATE_CONNECTING,
			STATE_OPEN,
			STATE_CLOSING,
			STATE_FAILED
		};

		WSClientClass();
		~WSClientClass();

		WSClientClass(WSClientClass const &) = delete;
		WSClientClass & operator = (WSClientClass const &) = delete;

		bool Connect(char const *url);
		void Close(void);
		int Poll(void);

		StateType Get_State(void) const
		{
			return(State);
		}

		bool Is_Open(void) const
		{
			return(State == STATE_OPEN);
		}

		bool Send_Binary(void const *data, std::size_t length);
		bool Receive_Binary(void **data, std::size_t *length);
		int Receive_Queue_Size(void) const;
		bool Has_Pending_Output(void) const;

	private:
		StateType State;
		void *Impl;
};

#endif
