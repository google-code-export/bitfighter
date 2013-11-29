//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _TNL_NETCONNECTION_H_
#define _TNL_NETCONNECTION_H_

#ifndef _TNL_NETBASE_H_
#include "tnlNetBase.h"
#endif

#ifndef _TNL_LOG_H_
#include "tnlLog.h"
#endif

#ifndef _TNL_NONCE_H_
#include "tnlNonce.h"
#endif

#ifndef _TNL_SYMMETRICCIPHER_H_
#include "tnlSymmetricCipher.h"
#endif

#ifndef _TNL_CONNECTIONSTRINGTABLE_H_
#include "tnlConnectionStringTable.h"
#endif

namespace TNL {

class NetConnection;
class NetObject;
class BitStream;
class NetInterface;
class AsymmetricKey;
class Certificate;

/// NetConnectionRep maintians a linked list of valid connection classes.
struct NetConnectionRep
{
   static NetConnectionRep *mLinkedList;

   NetConnectionRep *mNext;
   NetClassRep *mClassRep;
   bool mCanRemoteCreate;

   NetConnectionRep(NetClassRep *classRep, bool canRemoteCreate)
   {
      mNext = mLinkedList;
      mLinkedList = this;
      mClassRep = classRep;
      mCanRemoteCreate = canRemoteCreate;
   }
   static NetConnection *create(const char *name);
};

#define TNL_DECLARE_NETCONNECTION(className) \
   TNL_DECLARE_CLASS(className); \
   TNL::NetClassGroup getNetClassGroup() const

#define TNL_IMPLEMENT_NETCONNECTION(className, classGroup, canRemoteCreate) \
   TNL::NetClassRep* className::getClassRep() const { return &className::dynClassRep; } \
   TNL::NetClassRepInstance<className> className::dynClassRep(#className, 0, TNL::NetClassTypeNone, 0); \
   TNL::NetClassGroup className::getNetClassGroup() const { return classGroup; } \
   static TNL::NetConnectionRep g##className##Rep(&className::dynClassRep, canRemoteCreate)


/// All data associated with the negotiation of the connection
struct ConnectionParameters
{
   bool mIsArranged;                 ///< True if this is an arranged connection
   bool mUsingCrypto;                ///< Set to true if this connection is using crypto (public key and symmetric)
   bool mPuzzleRetried;              ///< True if a puzzle solution was already rejected by the server once.
   Nonce mNonce;                     ///< Unique nonce generated for this connection to send to the server.
   Nonce mServerNonce;               ///< Unique nonce generated by the server for the connection.
   U32 mPuzzleDifficulty;            ///< Difficulty of the client puzzle solved by this client.
   U32 mPuzzleSolution;              ///< Solution to the client puzzle the server sends to the client.
   U32 mClientIdentity;              ///< The client identity as computed by the remote host.
   RefPtr<AsymmetricKey> mPublicKey; ///< The public key of the remote host.
   RefPtr<AsymmetricKey> mPrivateKey;///< The private key for this connection.  May be generated on the connection attempt.
   RefPtr<Certificate> mCertificate; ///< The certificate of the remote host.
   ByteBufferPtr mSharedSecret;      ///< The shared secret key 
   bool mRequestKeyExchange;         ///< The initiator of the connection wants a secure key exchange
   bool mRequestCertificate;         ///< The client is requesting a certificate
   U8 mSymmetricKey[SymmetricCipher::KeySize]; ///< The symmetric key for the connection, generated by the client
   U8 mInitVector[SymmetricCipher::KeySize]; ///< The init vector, generated by the server
   Vector<Address> mPossibleAddresses; ///< List of possible addresses for the remote host in an arranged connection.
   bool mIsInitiator;                ///< True if this host initiated the arranged connection.
   bool mIsLocal;                    ///< True if this is a connectLocal connection.
   ByteBufferPtr mArrangedSecret;    ///< The shared secret as arranged by the connection intermediary.
   bool mDebugObjectSizes;           ///< This connection's initiator requested debugging size information during packet writes.

   ConnectionParameters()
   {
      mIsInitiator = false;
      mPuzzleRetried = false;
      mUsingCrypto = false;
      mIsArranged = false;
#ifdef TNL_DEBUG
      mDebugObjectSizes = true;
#else
      mDebugObjectSizes = false;     
#endif
      mIsLocal = false;
   }
};

//----------------------------------------------------------------------------
/// TNL network connection base class.
///
/// NetConnection is the base class for the connection classes in TNL. It implements a
/// notification protocol on the unreliable packet transport of UDP (via the TNL::Net layer).
/// NetConnection manages the flow of packets over the network, and calls its subclasses
/// to read and write packet data, as well as handle packet delivery notification.
///
/// Because string data can easily soak up network bandwidth, for
/// efficiency NetConnection implements an optional networked string table.
/// Users can then notify the connection of strings it references often, such as player names,
/// and transmit only a tag, instead of the whole string.
///
class NetConnection : public Object
{
   friend class NetInterface;
   friend class ConnectionStringTable;

   typedef Object Parent;

   /// Constants controlling the data representation of each packet header
   enum HeaderConstants {
      // NOTE - IMPORTANT!
      // The first bytes of each packet are made up of:
      // 1 bit - game data packet flag
      // 2 bits - packet type
      // SequenceNumberBitSize bits - sequence number
      // AckSequenceNumberBitSize bits - high ack sequence received
      // these values should be set to align to a byte boundary, otherwise
      // bits will just be wasted.

      MaxPacketWindowSizeShift = 5,                            ///< Packet window size is 2^MaxPacketWindowSizeShift.
      MaxPacketWindowSize = (1 << MaxPacketWindowSizeShift),   ///< Maximum number of packets in the packet window.
      PacketWindowMask = MaxPacketWindowSize - 1,              ///< Mask for accessing the packet window.
      MaxAckMaskSize = 1 << (MaxPacketWindowSizeShift - 5),    ///< Each ack word can ack 32 packets.
      MaxAckByteCount = MaxAckMaskSize << 2,                   ///< The maximum number of ack bytes sent in each packet.
      SequenceNumberBitSize = 11,                              ///< Bit size of the send and sequence number.
      SequenceNumberWindowSize = (1 << SequenceNumberBitSize), ///< Size of the send sequence number window.
      SequenceNumberMask = -SequenceNumberWindowSize,          ///< Mask used to reconstruct the full send sequence number of the packet from the partial sequence number sent.
      AckSequenceNumberBitSize = 10,                           ///< Bit size of the ack receive sequence number.
      AckSequenceNumberWindowSize = (1 << AckSequenceNumberBitSize), ///< Size of the ack receive sequence number window.
      AckSequenceNumberMask = -AckSequenceNumberWindowSize,          ///< Mask used to reconstruct the full ack receive sequence number of the packet from the partial sequence number sent.

      PacketHeaderBitSize = 3 + AckSequenceNumberBitSize + SequenceNumberBitSize, ///< Size, in bits, of the packet header sequence number section
      PacketHeaderByteSize = (PacketHeaderBitSize + 7) >> 3, ///< Size, in bytes, of the packet header sequence number information
      PacketHeaderPadBits = (PacketHeaderByteSize << 3) - PacketHeaderBitSize, ///< Padding bits to get header bytes to align on a byte boundary, for encryption purposes.

      MessageSignatureBytes = 5, ///< Special data bytes written into the end of the packet to guarantee data consistency
   };

   U32 mLastSeqRecvdAtSend[MaxPacketWindowSize]; ///< The sequence number of the last packet received from the remote host when we sent the packet with sequence X & PacketWindowMask.
   U32 mLastSeqRecvd;                            ///< The sequence number of the most recently received packet from the remote host.
   U32 mHighestAckedSeq;                         ///< The highest sequence number the remote side has acknowledged.
   U32 mLastSendSeq;                             ///< The sequence number of the last packet sent.
   U32 mAckMask[MaxAckMaskSize];                 ///< long string of bits, each acking a packet sent by the remote host.
                                                 ///< The bit associated with mLastSeqRecvd is the low bit of the 0'th word of mAckMask.
   U32 mLastRecvAckAck; ///< The highest sequence this side knows the other side has received an ACK or NACK for.

   U32 mInitialSendSeq; ///< The first mLastSendSeq for this side of the connection.
   U32 mInitialRecvSeq; ///< The first mLastSeqRecvd (the first mLastSendSeq for the remote host).
   U32 mHighestAckedSendTime; ///< The send time of the highest packet sequence acked by the remote host.  Used in the computation of round trip time.
   /// Two-bit identifier for each connected packet.
   enum NetPacketType
   {
      DataPacket, ///< Standard data packet.  Each data packet sent increments the current packet sequence number (mLastSendSeq).
      PingPacket, ///< Ping packet, sent if this instance hasn't heard from the remote host for a while.  Sending a
                  ///  ping packet does not increment the packet sequence number.
      AckPacket,  ///< Packet sent in response to a ping packet.  Sending an ack packet does not increment the sequence number.
      InvalidPacketType,
   };
   /// Constants controlling the behavior of pings and timeouts
   enum DefaultPingConstants {
      AdaptiveInitialPingTimeout = 25000,
      AdaptivePingRetryCount = 4,
      DefaultPingTimeout = 5000,  ///< Default milliseconds to wait before sending a ping packet.
      DefaultPingRetryCount = 10, ///< Default number of unacknowledged pings to send before timing out.
      AdaptiveUnackedSentPingTimeout = 3000,
   };
   U32 mPingTimeout; ///< Milliseconds to wait before sending a ping packet.
   U32 mPingRetryCount; ///< Number of unacknowledged pings to send before timing out.

   /// Returns true if this connection has sent packets that have not yet been acked by the remote host.
   bool hasUnackedSentPackets() { return mLastSendSeq != mHighestAckedSeq; }

protected:
      U32 mLastPacketRecvTime; ///< Time of the receipt of the last data packet.

public:
   struct PacketNotify;

   NetConnection();
   virtual ~NetConnection();

   enum TerminationReason {
      // Reasons the server might terminate an existing connection
      ReasonTimedOut,
      ReasonIdle,                   // Client is asleep at the wheel
      ReasonSelfDisconnect,
      ReasonKickedByAdmin,
      ReasonFloodControl,
      ReasonPuzzle,
      ReasonError,
      ReasonShutdown,

      // Reasons the master might reject a client at connect time
      ReasonBadLogin,               // User provided an invalid password for their username
      ReasonDuplicateId,            // User provided duplicate ID to that of another client; should never happen
      ReasonInvalidUsername,        // Username contains illegal characters
      ReasonBadConnection,          // Something went wrong in the connection

      // Reasons a server might reject a client at connect time
      ReasonInvalidCRC,
      ReasonIncompatibleRPCCounts,  // Incompatible version of the software, should never happen due to our version management
      ReasonServerFull,             // Too many players
      ReasonNeedServerPassword,     // Server requires a password -- either provided none, or an incorrect one

      ReasonConnectionsForbidden,   // Used when rejecting data connections when data connection capability is disabled
      ReasonBanned,                 // You made the admin mad...
      ReasonAnonymous,              // Anonymous connections are terminated quickly, only for simple data retrieval
      TerminationReasons,           // Must be last of enumerated reasons!
      ReasonNone
   };

protected:
   /// Called when a pending connection is terminated
   virtual void onConnectTerminated(NetConnection::TerminationReason reason, const char *rejectionString);   

   /// Called when this established connection is terminated for any reason
   virtual void onConnectionTerminated(NetConnection::TerminationReason, const char *errorDisconnectString);   

   /// Called when the connection is successfully established with the remote host
   virtual void onConnectionEstablished();  

   /// Calidates that the given certificate is a valid certificate for this connection
   virtual bool validateCertficate(Certificate *theCertificate, bool isInitiator) { return true; }

   /// Validates that the given public key is valid for this connection.  If this
   /// host requires a valid certificate for the communication, this function
   /// should always return false.  It will only be called if the remote side
   /// of the connection did not provide a certificate.
   virtual bool validatePublicKey(AsymmetricKey *theKey, bool isInitiator) { return true; }

   /// Fills the connect request packet with additional custom data (from a subclass
   virtual void writeConnectRequest(BitStream *stream);

   /// Called after this connection instance is created on a non-initiating host (server)
   ///
   /// Reads data sent by the writeConnectRequest method and returns true if the connection is accepted
   /// or false if it's not.  The errorString pointer should be filled if the connection is rejected.
   virtual bool readConnectRequest(BitStream *stream, NetConnection::TerminationReason &reason);

   /// Writes any data needed to start the connection on the accept packet
   virtual void writeConnectAccept(BitStream *stream);

   /// Reads out the extra data read by writeConnectAccept and returns true if it is processed properly
   virtual bool readConnectAccept(BitStream *stream, TerminationReason &reason);

   virtual void readPacket(BitStream *bstream);                      ///< Called to read a subclass's packet data from the packet.

   virtual void prepareWritePacket();  ///< Called to prepare the connection for packet writing.
                                       ///
                                       ///  Any setup work to determine if there isDataToTransmit() should happen in
                                       ///  this function.  prepareWritePacket should _always_ call the Parent:: function.

   virtual void writePacket(BitStream *bstream, PacketNotify *note); ///< Called to write a subclass's packet data into the packet.
                                                                     ///
                                                                     ///  Information about what the instance wrote into the packet can be attached
                                                                     ///  to the notify object.

   virtual void packetReceived(PacketNotify *note);                  ///< Called when the packet associated with the specified notify is known to have been received by the remote host.
                                                                     ///
                                                                     ///  Packets are guaranteed to be notified in the order in which they were sent.
   virtual void packetDropped(PacketNotify *note);                   ///< Called when the packet associated with the specified notify is known to have been not received by the remote host.
                                                                     ///
                                                                     ///  Packets are guaranteed to be notified in the order in which they were sent.

   /// Allocates a data record to track data sent on an individual packet.
   ///
   /// If you need to track additional notification information, you'll have to
   /// override this so you allocate a subclass of PacketNotify with extra fields.
   virtual PacketNotify *allocNotify() { return new PacketNotify; }

public:
   /// Returns the next send sequence that will be sent by this side.
   U32 getNextSendSequence() { return mLastSendSeq + 1; }

   /// Returns the sequence of the last packet sent by this connection, or
   /// the current packet's send sequence if called from within writePacket().
   U32 getLastSendSequence() { return mLastSendSeq; }

protected:
   /// Reads a raw packet from a BitStream, as dispatched from NetInterface.
   void readRawPacket(BitStream *bstream);
   /// Writes a full packet of the specified type into the BitStream
   void writeRawPacket(BitStream *bstream, NetPacketType packetType);

   /// Writes the notify protocol's packet header into the BitStream.
   void writePacketHeader(BitStream *bstream, NetPacketType packetType);
   /// Reads a notify protocol packet header from the BitStream and
   /// returns true if it was a data packet that needs more processing.
   bool readPacketHeader(BitStream *bstream);

   void writePacketRateInfo(BitStream *bstream, PacketNotify *note); ///< Writes any packet send rate change information into the packet.
   void readPacketRateInfo(BitStream *bstream);                      ///< Reads any packet send rate information requests from the packet.

   void sendPingPacket(); ///< Sends a ping packet to the remote host, to determine if it is still alive and what its packet window status is.
   void sendAckPacket();  ///< Sends an ack packet to the remote host, in response to receiving a ping packet.

   /// Dispatches a notify when a packet is ACK'd or NACK'd.
   void handleNotify(U32 sequence, bool recvd);

   /// Called when a packet is received to stop any timeout action in progress.
   void keepAlive();

   void clearAllPacketNotifies(); ///< Clears out the pending notify list.

public:
   /// Sets the initial sequence number of packets read from the remote host.
   void setInitialRecvSequence(U32 sequence);

   /// Returns the initial sequence number of packets sent from the remote host.
   U32 getInitialRecvSequence() { return mInitialRecvSeq; }

   /// Returns the initial sequence number of packets sent to the remote host.
   U32 getInitialSendSequence() { return mInitialSendSeq; }

   /// Connect to a server through a given network interface.
   /// The connection request can require that the connection use encryption, 
   /// or that the remote host's certificate be validated by a known Certificate Authority
   void connect(NetInterface *connectionInterface, const Address &address, bool requestKeyExchange = false, bool requestCertificate = false);

   /// Connects to a server interface within the same process.
   bool connectLocal(NetInterface *connectionInterface, NetInterface *localServerInterface);

   /// Connects to a remote host that is also connecting to this connection (negotiated by a third party)
   void connectArranged(NetInterface *connectionInterface, const Vector<Address> &possibleAddresses, Nonce &myNonce, Nonce &remoteNonce, ByteBufferPtr sharedSecret, bool isInitiator, bool requestsKeyExchange = false, bool requestsCertificate = false);

   /// Sends a disconnect packet to notify the remote host that this side is terminating the connection for the specified reason.
   /// This will remove the connection from its NetInterface, and may have the side
   /// effect that the connection is deleted, if there are no other objects with RefPtrs
   /// to the connection.
   void disconnect(TerminationReason tr, const char *reason);

   /// Returns true if the packet send window is full and no more data packets can be sent.
   bool windowFull();

   /// Structure used to track what was sent in an individual packet for processing
   /// upon notification of delivery success or failure.
   struct PacketNotify
   {
      // packet stream notify stuff:
      bool rateChanged;  ///< True if this packet requested a change of rate.
      U32  sendTime;     ///< Platform::getRealMilliseconds() when packet was sent.
      ConnectionStringTable::PacketList stringList; ///< List of string table entries sent in this packet

      PacketNotify *nextPacket; ///< Pointer to the next packet sent on this connection
      PacketNotify();
   };

//----------------------------------------------------------------
// Connection functions
//----------------------------------------------------------------

   /// Flags specifying the type of the connection instance.
   enum NetConnectionTypeFlags {
      ConnectionToServer = BIT(0), ///< A connection to a "server", used for directing NetEvents
      ConnectionToClient = BIT(1), ///< A connection to a "client"
      ConnectionAdaptive = BIT(2), ///< Indicates that this connection uses the adaptive protocol.
      ConnectionRemoteAdaptive = BIT(3), ///< Indicates that the remote side of this connection requested the adaptive protocol.
   };

private:
   BitSet32 mTypeFlags;  ///< Flags describing the type of connection this is, OR'd from NetConnectionTypeFlags.
   U32 mLastUpdateTime;  ///< The last time a packet was sent from this instance.
   F32 mRoundTripTime;   ///< Running average round trip time.
   U32 mSendDelayCredit; ///< Metric to help compensate for irregularities on fixed rate packet sends.

   U32 mSimulatedSendLatency;    ///< Amount of additional time this connection delays its packet sends to simulate latency in the connection
   U32 mSimulatedReceiveLatency;
   F32 mSimulatedSendPacketLoss; ///< Function to simulate packet loss on a network
   F32 mSimulatedReceivePacketLoss;

   bool mUseZeroLatencyForTesting;  ///< Override packet SendPeriod for testing purposes ONLY 

   enum RateDefaults {
      DefaultFixedBandwidth  = 2500,  ///< The default send/receive bandwidth - 2.5 Kb per second.
      DefaultFixedSendPeriod = 96,    ///< The default delay between each packet send - approx 10 packets per second.
      MaxFixedBandwidth      = 65535, ///< The maximum bandwidth for a connection using the fixed rate transmission method.
      MaxFixedSendPeriod     = 2047,  ///< The maximum period between packets in the fixed rate send transmission method.
   };

   /// Rate management structure used specify the rate at which packets are sent and the maximum size of each packet.
   struct NetRate
   {
      U32 minPacketSendPeriod; ///< Minimum millisecond delay (maximum rate) between packet sends.
      U32 minPacketRecvPeriod; ///< Minimum millisecond delay the remote host should allow between sends.
      U32 maxSendBandwidth;    ///< Number of bytes per second we can send over the connection.
      U32 maxRecvBandwidth;    ///< Number of bytes per second max that the remote instance should send.
   };

   void computeNegotiatedRate(); ///< Called internally when the local or remote rate changes.
   NetRate mLocalRate;           ///< Current communications rate negotiated for this connection.
   NetRate mRemoteRate;          ///< Maximum allowable communications rate for this connection.

   bool mLocalRateChanged;       ///< Set to true when the local connection's rate has changed.
   U32 mCurrentPacketSendSize;   ///< Current size of each packet sent to the remote host.
   U32 mCurrentPacketSendPeriod; ///< Millisecond delay between sent packets.

   Address mNetAddress;       ///< The network address of the host this instance is connected to.

   // timeout management stuff:
   U32 mPingSendCount;    ///< Number of unacknowledged ping packets sent to the remote host
   U32 mLastPingSendTime; ///< Last time a ping packet was sent from this connection

protected:
   PacketNotify *mNotifyQueueHead;  ///< Linked list of structures representing the data in sent packets
   PacketNotify *mNotifyQueueTail;  ///< Tail of the notify queue linked list.  New packets are added to the end of the tail.

   /// Returns the notify structure for the current packet write, or last written packet.
   PacketNotify *getCurrentWritePacketNotify() { return mNotifyQueueTail; }


   SafePtr<NetConnection> mRemoteConnection;  ///< Safe pointer to a short-circuit remote connection on the same host.
                                              ///
                                              ///  This currently isn't enabled - see the end of netConnection.cpp for an example
                                              ///  of how to use this. If it's set, the code will use short circuited networking.
   ConnectionParameters mConnectionParameters;
public:
   ConnectionParameters &getConnectionParameters() { return mConnectionParameters; }

   /// returns true if this object initiated the connection with the remote host
   bool isInitiator() { return mConnectionParameters.mIsInitiator; }
   void setRemoteConnectionObject(NetConnection *connection) { mRemoteConnection = connection; };
   NetConnection *getRemoteConnectionObject() { return mRemoteConnection; }

   U32 mConnectSendCount;    ///< Number of challenge or connect requests sent to the remote host.
   U32 mConnectLastSendTime; ///< The send time of the last challenge or connect request.

protected:
   static char mErrorBuffer[256]; ///< String buffer that errors are written into
public:
   static char *getErrorBuffer() { return mErrorBuffer; } ///< returns the current error buffer
   static void setLastError(const char *fmt,...);         ///< Sets an error string and notifies the currently processing connection that it should terminate.

protected:
   SafePtr<NetInterface> mInterface;             ///< The NetInterface of which this NetConnection is a member.
public:
   void setInterface(NetInterface *myInterface); ///< Sets the NetInterface this NetConnection will communicate through.
   NetInterface *getInterface();                 ///< Returns the NetInterface this connection communicates through.

   /// Return time since we received our last packet
   U32 getTimeSinceLastPacketReceived();

protected:
   RefPtr<SymmetricCipher> mSymmetricCipher;    ///< The helper object that performs symmetric encryption on packets
public:
   void setSymmetricCipher(SymmetricCipher *theCipher); ///< Sets the SymmetricCipher this NetConnection will use for encryption

public:
   /// Returns the class group of objects that can be transmitted over this NetConnection.
   virtual NetClassGroup getNetClassGroup() const { return NetClassGroupInvalid; }

   /// Sets the ping/timeout characteristics for a fixed-rate connection.  Total timeout is msPerPing * pingRetryCount.
   void setPingTimeouts(U32 msPerPing, U32 pingRetryCount)
      { mPingRetryCount = pingRetryCount; mPingTimeout = msPerPing; }
   
   /// Simulates a network situation with a percentage random packet loss and a connection one way latency as specified.
   void setSimulatedNetParams(F32 sendLoss, U32 sendLatency, F32 receiveLoss, U32 receiveLatency) { 
      mSimulatedSendPacketLoss = sendLoss; 
      mSimulatedSendLatency = sendLatency; 
      mSimulatedReceivePacketLoss = receiveLoss; 
      mSimulatedReceiveLatency = receiveLatency;
   }

   void setSimulatedNetParams(F32 packetLoss, U32 latency) { 
      mSimulatedReceivePacketLoss = mSimulatedSendPacketLoss = packetLoss; 
      mSimulatedReceiveLatency = latency / 2; 
      mSimulatedSendLatency = (latency + 1) / 2;
   }

   U32 getSimulatedSendLatency()       { return mSimulatedSendLatency;       }
   U32 getSimulatedReceiveLatency()    { return mSimulatedReceiveLatency;    }
   F32 getSimulatedSendPacketLoss()    { return mSimulatedSendPacketLoss;    }
   F32 getSimulatedReceivePacketLoss() { return mSimulatedReceivePacketLoss; }


   /// Specifies that this NetConnection instance is a connection to a "server."
   void setIsConnectionToServer() { mTypeFlags.set(ConnectionToServer); }

   /// Returns true if this is a connection to a "server."
   bool isConnectionToServer()  { return mTypeFlags.test(ConnectionToServer); }

   /// Specifies that this NetConnection instance is a connection to a "client."
   void setIsConnectionToClient() { mTypeFlags.set(ConnectionToClient); }

   /// Returns true if this is a connection to a "client."
   bool isConnectionToClient()  { return mTypeFlags.test(ConnectionToClient); }

   /// Returns true if the remote side of this connection is a NetConnection instance in on the same host.
   bool isLocalConnection() { return !mRemoteConnection.isNull(); }

   /// Returns true if the remote side if this connection is on a remote host.
   bool isNetworkConnection() { return mRemoteConnection.isNull(); }

   /// Returns the running average packet round trip time.
   F32 getRoundTripTime()
      { return mRoundTripTime; }

   /// Returns have of the average of the round trip packet time.
   F32 getOneWayTime()
      { return mRoundTripTime * 0.5f; }

   /// Returns the remote address of the host we're connected or trying to connect to.
   const Address &getNetAddress();

   /// Returns the remote address in string form.
   const char *getNetAddressString() const { return mNetAddress.toString(); }

   /// Sets the address of the remote host we want to connect to.
   void setNetAddress(const Address &address);

   /// Sends a packet that was written into a BitStream to the remote host, or the mRemoteConnection on this host.
   NetError sendPacket(BitStream *stream);

   /// Checks to see if the connection has timed out, possibly sending a ping packet to the remote host.  Returns true if the connection timed out.
   bool checkTimeout(U32 time);

   /// Checks to see if a packet should be sent at the currentTime to the remote host.
   ///
   /// If force is true and there is space in the window, it will always send a packet.
   void checkPacketSend(bool force, U32 currentTime);

   /// Connection state flags for a NetConnection instance.  If this list is modifed, please check if netInterface.cpp needs updates as well
   enum NetConnectionState {
      NotConnected=0,            ///< Initial state of a NetConnection instance - not connected
      AwaitingChallengeResponse, ///< We've sent a challenge request, awaiting the response
      SendingPunchPackets,       ///< The state of a pending arranged connection when both sides haven't heard from the other yet
      ComputingPuzzleSolution,   ///< We've received a challenge response, and are in the process of computing a solution to its puzzle
      AwaitingConnectResponse,   ///< We've received a challenge response and sent a connect request
      ConnectTimedOut,           ///< The connection timed out during the connection process
      ConnectRejected,           ///< The connection was rejected
      Connected,                 ///< We've accepted a connect request, or we've received a connect response accept
      Disconnected,              ///< The connection has been disconnected
      TimedOut,                  ///< The connection timed out
      StateCount,
   };

   NetConnectionState mConnectionState; ///< Current state of this NetConnection.

   /// Sets the current connection state of this NetConnection.
   void setConnectionState(NetConnectionState state) { mConnectionState = state; }

   /// Gets the current connection state of this NetConnection.
   NetConnectionState getConnectionState() { return mConnectionState; }

   /// Returns true if the connection handshaking has completed successfully.
   bool isEstablished() 
   { 
      return mConnectionState == Connected; 
   }

   /// @name Adaptive Protocol
   ///
   /// Functions and state for the adaptive rate protocol.
   ///
   /// TNL's adaptive rate uses rate control algorithms similar to
   /// TCP/IP's.
   ///
   /// There are a few state variables here that aren't documented.
   ///
   /// @{

public:

   /// Enables the adaptive protocol.
   ///
   /// By default NetConnection operates with a fixed rate protocol - that is, it sends a
   /// packet every few milliseconds, based on some configuration parameters. However,
   /// it is possible to use an adaptive rate protocol that attempts to maximize thoroughput
   /// over the connection.
   ///
   /// Calling this function enables this behavior.
   void setIsAdaptive();

   /// Sets the fixed rate send and receive data sizes, and sets the connection to not behave as an adaptive rate connection
   void setFixedRateParameters( U32 minPacketSendPeriod, U32 minPacketRecvPeriod, U32 maxSendBandwidth, U32 maxRecvBandwidth );

   /// Flag to override computed packet size limitations, used for testing to allow tests to run faster than they otherwise would
   void useZeroLatencyForTesting();    ///< Only for testing purposes!!!

   /// Query the adaptive status of the connection.
   bool isAdaptive()    { return mTypeFlags.test(ConnectionAdaptive | ConnectionRemoteAdaptive); }

   /// Returns true if this connection has data to transmit.
   ///
   /// The adaptive rate protocol needs to be able to tell if there is data
   /// ready to be sent, so that it can avoid sending unnecessary packets.
   /// Each subclass of NetConnection may need to send different data - events,
   /// ghost updates, or other things. Therefore, this hook is provided so
   /// that child classes can overload it and let the adaptive protocol
   /// function properly.
   ///
   /// @note Make sure this calls to its parents - the accepted idiom is:
   ///       @code
   ///       return Parent::isDataToTransmit() || localConditions();
   ///       @endcode
   virtual bool isDataToTransmit() { return false; }


private:
   F32 cwnd;
   F32 ssthresh;
   U32 mLastSeqRecvdAck;
   U32 mLastAckTime;

   /// @}
protected:
   ConnectionStringTable *mStringTable; ///< Helper for managing translation between global NetStringTable ids to local ids for this connection.
public:
   /// Enables string tag translation on this connection.
   void setTranslatesStrings();
};

static const U32 MinimumPaddingBits = 128;       ///< Padding space that is required at the end of each packet for bit flag writes and such.

};

#endif
