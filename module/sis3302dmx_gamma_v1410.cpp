/*
Copyright 2011 Bastian Loeher, Roland Wirth

This file is part of GECKO.

GECKO is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

GECKO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sis3302dmx_gamma_v1410.h"
#include "eventbuffer.h"

#include <QVector>


Sis3302V1410Demux::Sis3302V1410Demux(const QList<EventSlot *> &evsl)
    : evslots (evsl), pageWrap(false)
{
}

void Sis3302V1410Demux::setMetaData(uint32_t _nofTraces, EventDirEntry_t* _evDir, TimestampDir_t *_tsDir)
{
    nofTraces = _nofTraces;
    pageWrap = true;
}

void Sis3302V1410Demux::setMultiEvent(bool _isMultiEvent) {
    isMultiEvent = _isMultiEvent;
}

void Sis3302V1410Demux::setNofEvents(uint32_t _nofEvents) {
    nofEvents = _nofEvents;
}

void Sis3302V1410Demux::process (Event *ev, uint32_t *_data, uint32_t _len)
{

    //printf("DemuxSis3302V1410Plugin processing...\n");
    data = (DataStruct_t*)_data;
    len = _len;

    // Recover channel information
    uint8_t curCh = (len >> 29) & 0x7;

    // Revover length
    uint32_t length = (len & 0x1ffffff); // lWords
    uint32_t length_single = 0;

    if(isMultiEvent) length_single = length / nofEvents;
    else length_single = length;

    // Header
    uint16_t header = _data[0] & 0xffff;

    // Compute data lengths
    uint16_t length_raw = header/2; // 16-bit samples
    uint16_t length_energy = length_single - length_raw/2 - 6;

    // Compute data offsets
    uint16_t rawOffset = 2;
    uint16_t energyOffset = rawOffset+(length_raw/2);
    uint16_t energyValueOffset = energyOffset + length_energy;

    //printf("sis3302dmx: Current channel: %d with %d lwords of data.\n",curCh,length);
    //printf("sis3302dmx: MultiEvent: %d, nofEvents: %d with single length: %d\n",isMultiEvent,nofEvents,length_single);
    //printf("sis3302dmx: raw: %d samples, energy: %d samples.\n",length_raw,length_energy);
    //printf("sis3302dmx: raw: %d offset, energy: %d offset.\n",rawOffset,energyOffset);

    // Event data containers
    QVector<uint32_t> outData(length_raw*nofEvents,0);
    QVector<uint32_t> outData2(length_energy*nofEvents,0);
    QVector<double> outData3(nofEvents,0);

    int rawcnt = 0;
    int nrgcnt = 0;
    for(uint32_t n = 0; n < nofEvents; ++n) {
        uint32_t event_offset = length_single*n;

        //  RAW trace
        for(uint32_t i = 0; i < length_raw/2; i++) {
            outData[rawcnt++] = data[event_offset + rawOffset + i].low;
            outData[rawcnt++] = data[event_offset + rawOffset + i].high;
        }

        //  Energy trace

        for(uint32_t i = 0; i < length_energy; i++) {
            outData2[nrgcnt++] = data[event_offset + energyOffset + i].data;
        }

        //  Energy value
        double energyValue = - (int32_t)(data[event_offset + energyValueOffset + 1].data)
                            + (int32_t)(data[event_offset + energyValueOffset].data);
        outData3[n] = energyValue;

        //printf("sis3302dmx: energy value[%d]: %d offset, %f value: \n",n,energyValueOffset,energyValue);
    }


    // Publish event data
    ev->put (evslots.at(curCh), QVariant::fromValue (outData));
    ev->put (evslots.at(curCh+8), QVariant::fromValue (outData2));
    ev->put (evslots.at(curCh+16), QVariant::fromValue (outData3));

    /*printf("Data dump from DMX:\n");
    for(uint32_t i=0; i < length*2; i++)
    {
        printf("<%d> %u  ",i,outData[i]);
    }
    printf("\n");*/

}
