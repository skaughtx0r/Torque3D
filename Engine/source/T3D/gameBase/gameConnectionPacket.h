//-----------------------------------------------------------------------------
// gameConnectionPacket.h
//
// Copyright (c) 2014 The Beyond Reality Team
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//------------------------------------------------------------------------------

// TODO: Handle when control object changes to not confuse values?
// maybe unnecessary?

///
/// THE FORCE CLIENT SYSTEM AND NETWORKING EXPLAINED:
/// 
/// The force system used  for the client to server architecture that is implemented.
/// The control object in this engine sends its data to the server instead of sending moves.
/// Original torque3D sends moves instead of position/velocity/ect. but we modified it in this
/// engine to avoid having to try to come up with rediculous client prediction and to prevent
/// "warping" or "snapping".
///
/// All data that is sent and received for each control object is sent in the GameConnectionPacket
/// which contains fields that must be filled on the client side through getClientPacket
/// and received and handled in receiveClientPacket on the server side.
///
/// When a packet is received on the client from the server then, it will check to see if that
/// object is your control object on teh client.  If it is, it just reads the streams and then
/// disgards the use of it.  But what happens if you wanted the server TO have authority over
/// the client object, say for example you called setVelocity() in a server script (1 example
/// of many).
///
/// In order to acomplish this, we use the force system, which sends a boolean check from a typemask
/// check to see if we should call it EVEN IF the object is the control object.  In essence, it
/// overrides the lack of updating on the client side in order TO update.
///

#ifndef _GAME_CONNECTION_PACKET_H_
#define _GAME_CONNECTION_PACKET_H_

#include "T3D/gameBase/gameConnection.h"
#include "core/stream/bitStream.h"
#include "math/mathIO.h"

struct GameConnectionPacket;

struct GameConnectionPacket {
private:
	///<summary>
	/// Uses default settings
	///</summary>
	void getDefaults();

public:
	///<summary>
	/// The object's position, rotation, and scale
	///</summary>
	MatrixF transformation;

	///<summary>
	/// The object's linear velocity
	///</summary>
	Point3F linVelocity;

	///<summary>
	/// The object's angular velocity
	///</summary>
	Point3F angVelocity;

	GameConnectionPacket();

	///<summary>
	/// Called whenever the client is sending a packet to the server.
	///</summary>
	/// @param stream the bitstream of the connection
	/// @param obj the control object
	void send(BitStream *stream, GameBase *obj);

	///<summary>
	/// Called whenever the server is receivng a packet on the client.
	///</summary>
	/// @param stream the bitstream of the connection
	/// @param obj the control object
	void receive(BitStream *stream, GameBase *obj);
};

#endif // _GAME_CONNECTION_PACKET_H_