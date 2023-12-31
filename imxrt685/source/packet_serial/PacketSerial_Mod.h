// =============================================================================
// Copyright (c) 2013-2016 Christopher Baker <http://christopherbaker.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =============================================================================


#pragma once


//#include <Arduino.h>
//#include "Encoding/COBS.h"
//#include "Encoding/SLIP.h"
#include "COBS.h"
#include "SLIP.h"

class PacketHandlerClass{
public:
    virtual void onPacketFunction(const uint8_t* buffer, size_t size) = 0;
};

template<typename EncoderType, uint8_t PacketMarker = 0, size_t BufferSize = 256>
class PacketSerial_
{
public:
    typedef void (*PacketHandlerFunction)(const uint8_t* buffer, size_t size);

    PacketSerial_():
        _recieveBufferIndex(0),
        _serial(0),
        _onPacketFunction(0)
    {
    }

    ~PacketSerial_()
    {
    }

    void begin(Stream* serial)
    {
        _serial = serial;
    }

    void update()
    {
        if (_serial == 0) return;
        
        while (_serial->available() > 0)
        {
       
            uint8_t data = _serial->read();

            if (data == PacketMarker)
            {
                if (_onPacketFunction || _onPacketClass)
                {
                    uint8_t _decodeBuffer[_recieveBufferIndex];

                    size_t numDecoded = EncoderType::decode(_recieveBuffer,
                                                            _recieveBufferIndex,
                                                            _decodeBuffer);

                    if(_onPacketFunction){
                      _onPacketFunction(_decodeBuffer, numDecoded);
                    }
                    
                    if(_onPacketClass){
                      _onPacketClass->onPacketFunction(_decodeBuffer, numDecoded);
                    }
                    
                }

                _recieveBufferIndex = 0;
            }
            else
            {
                if ((_recieveBufferIndex + 1) < BufferSize)
                {
                    _recieveBuffer[_recieveBufferIndex++] = data;
                }
                else
                {
                    // Error, buffer overflow if we write.
                }
            }
        }
    }

    void send(const uint8_t* buffer, size_t size)
    {
        if(_serial == 0 || buffer == 0 || size == 0) return;

        uint8_t _encodeBuffer[EncoderType::getEncodedBufferSize(size)];

        size_t numEncoded = EncoderType::encode(buffer,
                                                size,
                                                _encodeBuffer);
                                                
        _serial->write(_encodeBuffer, numEncoded);
        _serial->write(PacketMarker);
    }

    void setPacketHandler(PacketHandlerFunction onPacketFunction)
    {
        _onPacketFunction = onPacketFunction;
    }

    void setPacketHandler(PacketHandlerClass *onPacketClass)
    {
        _onPacketClass = onPacketClass;
    }


private:
    PacketSerial_(const PacketSerial_&);
    PacketSerial_& operator = (const PacketSerial_&);

    uint8_t _recieveBuffer[BufferSize];
    size_t _recieveBufferIndex;

    Stream* _serial;

    PacketHandlerFunction _onPacketFunction;
    PacketHandlerClass    *_onPacketClass;

};



 typedef PacketSerial_<COBS> PacketSerial;
 typedef PacketSerial_<COBS> COBSPacketSerial;
 typedef PacketSerial_<SLIP, SLIP::END> SLIPPacketSerial;
