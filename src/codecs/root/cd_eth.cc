/*
** Copyright (C) 2002-2013 Sourcefire, Inc.
** Copyright (C) 1998-2002 Martin Roesch <roesch@sourcefire.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pcap.h>
#include "codecs/root/cd_eth_module.h"
#include "framework/codec.h"
#include "time/profiler.h"
#include "protocols/packet.h"
#include "protocols/eth.h"
#include "codecs/codec_events.h"
#include "managers/packet_manager.h"
#include "codecs/sf_protocols.h"

namespace
{

class EthCodec : public Codec
{
public:
    EthCodec() : Codec(CD_ETH_NAME){};
    ~EthCodec(){};


    virtual PROTO_ID get_proto_id() { return PROTO_ETH; };
    virtual void get_protocol_ids(std::vector<uint16_t>&) {};
    virtual void get_data_link_type(std::vector<int>&);
    virtual bool decode(const uint8_t *raw_pkt, const uint32_t& raw_len,
        Packet *p, uint16_t &lyr_len, uint16_t &next_prot_id);
    virtual bool encode(EncState*, Buffer* out, const uint8_t* raw_in);
    virtual bool update(Packet*, Layer*, uint32_t* len);
    virtual void format(EncodeFlags, const Packet* p, Packet* c, Layer*);
};

} // namespace


void EthCodec::get_data_link_type(std::vector<int>&v)
{
    v.push_back(DLT_EN10MB);
}


//--------------------------------------------------------------------
// decode.c::Ethernet
//--------------------------------------------------------------------

/*
 * Function: DecodeEthPkt(Packet *, char *, DAQ_PktHdr_t*, uint8_t*)
 *
 * Purpose: Decode those fun loving ethernet packets, one at a time!
 *
 * Arguments: p => pointer to the decoded packet struct
 *            user => Utility pointer (unused)
 *            pkthdr => ptr to the packet header
 *            pkt => pointer to the real live packet data
 *
 * Returns: void function
 */
bool EthCodec::decode(const uint8_t *raw_pkt, const uint32_t& raw_len,
        Packet *p, uint16_t &lyr_len, uint16_t& next_prot_id)
{

    /* do a little validation */
    if(raw_len < eth::hdr_len())
    {
        DEBUG_WRAP(DebugMessage(DEBUG_DECODE,
            "WARNING: Truncated eth header (%d bytes).\n", raw_len););

        codec_events::decoder_event(p, DECODE_ETH_HDR_TRUNC);

        return false;
    }

    /* lay the ethernet structure over the packet data */
    const eth::EtherHdr *eh = reinterpret_cast<const eth::EtherHdr *>(raw_pkt);

    DEBUG_WRAP(
            DebugMessage(DEBUG_DECODE, "%X:%X:%X:%X:%X:%X -> %X:%X:%X:%X:%X:%X\n",
                eh->ether_src[0],
                eh->ether_src[1], eh->ether_src[2], eh->ether_src[3],
                eh->ether_src[4], eh->ether_src[5], eh->ether_dst[0],
                eh->ether_dst[1], eh->ether_dst[2], eh->ether_dst[3],
                eh->ether_dst[4], eh->ether_dst[5]);
            );
    DEBUG_WRAP(
            DebugMessage(DEBUG_DECODE, "type:0x%X len:0x%X\n",
                ntohs(eh->ether_type), p->pkth->pktlen)
            );

    next_prot_id = ntohs(eh->ether_type);
    if (next_prot_id > eth::min_ethertype() )
    {
        p->proto_bits |= PROTO_BIT__ETH;
        lyr_len = eth::hdr_len();
        return true;
    }


    return false;
}


//-------------------------------------------------------------------------
// ethernet
//-------------------------------------------------------------------------

bool EthCodec::encode(EncState* enc, Buffer* out, const uint8_t* raw_in)
{
    // not raw ip -> encode layer 2
    int raw = ( enc->flags & ENC_FLAG_RAW );
    const eth::EtherHdr* hi = reinterpret_cast<const eth::EtherHdr*>(raw_in);
    eth::EtherHdr* ho;

    // if not raw ip AND out buf is empty
    if ( !raw && (out->off == out->end) )
    {
        // for alignment
        out->off = out->end = SPARC_TWIDDLE;
    }

    // if not raw ip OR out buf is not empty
    if ( !raw || (out->off != out->end) )
    {
        // we get here for outer-most layer when not raw ip
        // we also get here for any encapsulated ethernet layer.
        if (!update_buffer(out, sizeof(*ho)))
            return false;

        ho = reinterpret_cast<eth::EtherHdr*>(out->base);
        ho->ether_type = hi->ether_type;
        uint8_t *dst_mac = PacketManager::encode_get_dst_mac();
        
        if ( forward(enc) )
        {
            memcpy(ho->ether_src, hi->ether_src, sizeof(ho->ether_src));
            /*If user configured remote MAC address, use it*/
            if (nullptr != dst_mac)
                memcpy(ho->ether_dst, dst_mac, sizeof(ho->ether_dst));
            else
                memcpy(ho->ether_dst, hi->ether_dst, sizeof(ho->ether_dst));
        }
        else
        {
            memcpy(ho->ether_src, hi->ether_dst, sizeof(ho->ether_src));
            /*If user configured remote MAC address, use it*/
            if (nullptr != dst_mac)
                memcpy(ho->ether_dst, dst_mac, sizeof(ho->ether_dst));
            else
                memcpy(ho->ether_dst, hi->ether_src, sizeof(ho->ether_dst));
        }
    }

    return true;
}

bool EthCodec::update (Packet*, Layer* lyr, uint32_t* len)
{
    *len += lyr->length;
    return true;
}

void EthCodec::format(EncodeFlags f, const Packet* p, Packet* c, Layer* lyr)
{
    eth::EtherHdr* ch = (eth::EtherHdr*)lyr->start;

    if ( reverse(f) )
    {
        int i = lyr - c->layers;
        eth::EtherHdr* ph = (eth::EtherHdr*)p->layers[i].start;

        memcpy(ch->ether_dst, ph->ether_src, sizeof(ch->ether_dst));
        memcpy(ch->ether_src, ph->ether_dst, sizeof(ch->ether_src));
    }
}


//-------------------------------------------------------------------------
// api
//-------------------------------------------------------------------------

static Module* mod_ctor()
{
    return new EthModule;
}

static void mod_dtor(Module* m)
{
    delete m;
}

static Codec* ctor(Module*)
{
    return new EthCodec();
}

static void dtor(Codec *cd)
{
    delete cd;
}

static const CodecApi eth_api =
{
    { 
        PT_CODEC,
        CD_ETH_NAME,
        CDAPI_PLUGIN_V0,
        0,
        mod_ctor,
        mod_dtor,
    },
    nullptr, // pinit
    nullptr, // pterm
    nullptr, // tinit
    nullptr, // tterm
    ctor, // ctor
    dtor, // dtor
};

const BaseApi* cd_eth = &eth_api.base;
