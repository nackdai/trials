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

    ENetAddress address;
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 8000;

    auto peer = ::enet_host_connect(
        client,
        &address,
        0,
        0);
    IZ_ASSERT(peer != NULL);

    ENetEvent event;

    // Wait for connecting to server.
    for (;;) {
        auto result = ::enet_host_service(client, &event, 0);

        IZ_BOOL isConnected = IZ_FALSE;

        if (result > 0) {
            switch (event.type) {
            case ENetEventType::ENET_EVENT_TYPE_NONE:
                break;
            case ENetEventType::ENET_EVENT_TYPE_CONNECT:
                IZ_PRINTF(
                    "A new client connected from %x:%u.\n",
                    event.peer->address.host,
                    event.peer->address.port);
                isConnected = IZ_TRUE;
                break;
            case ENetEventType::ENET_EVENT_TYPE_RECEIVE:
                // TODO

                // Clean up the packet now that we're done using it.
                ::enet_packet_destroy(event.packet);
                break;
            case ENetEventType::ENET_EVENT_TYPE_DISCONNECT:
                // Reset the peer's client information.
                event.peer->data = NULL;
                break;
            }
        }

        if (isConnected) {
            break;
        }
    }

    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    auto res = enet_peer_send(peer, 0, packet);
    IZ_ASSERT(res == 0);

    // Send queued packets earlier than in a call to enet_host_service().
    ::enet_host_flush(client);

    ::enet_host_destroy(client);

    ::enet_deinitialize();

    return 0;
}
