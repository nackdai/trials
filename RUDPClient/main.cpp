#include <string>
#include "izSampleKit.h"
#include "izSystem.h"
#include "izThreadModel.h"

#include <enet/enet.h>

// http://enet.bespin.org/Tutorial.html

IzMain(0, 0)
{
    ::enet_initialize();

    auto client = ::enet_host_create(
        NULL,   // if NULL, create a client host
        1,      // only allow 1 outgoing connection
        2,      // allow up 2 channels to be used, 0 and 1,
        57600 / 8,  // 56K modem with 56 Kbps downstream bandwidth,
        14400 / 8); // 56K modem with 14 Kbps upstream bandwidth
    IZ_ASSERT(client != NULL);

    static const char* data = "This is test.";

    auto packet = ::enet_packet_create(
        data,
        strlen(data) + 1,
        ENET_PACKET_FLAG_RELIABLE);
    IZ_ASSERT(packet != NULL);

    ENetPeer peer;
    memset(&peer, 0, sizeof(peer));

    peer.address.host = ENET_HOST_ANY;
    peer.address.port = 8000;

    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    auto res = enet_peer_send(&peer, 0, packet);
    IZ_ASSERT(res == 0);

    ::enet_host_flush(client);

    ::enet_host_destroy(client);

    ::enet_deinitialize();

    return 0;
}
