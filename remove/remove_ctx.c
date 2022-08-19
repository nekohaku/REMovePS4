#include "remove_ctx.h"

#include <orbis/libkernel.h>
#include <orbis/Net.h>
#include <orbis/UserService.h>
#include <string.h>

#ifndef orbis_net_errno
#define orbis_net_errno ( *( ( (int *(*)()) &sceNetErrnoLoc )() ) )
#endif /* orbis_net_errno */

typedef struct OrbisNetSockaddrIn {
	uint8_t sin_len;
	OrbisNetSaFamily_t sin_family;
	OrbisNetInPort_t sin_port;
	OrbisNetInAddr_t sin_addr;
	OrbisNetInPort_t sin_vport;
	char sin_zero[6];
} OrbisNetSockaddrIn;

typedef union OrbisNetEpollData {
	void *ptr;
	uint32_t u32;
    int fd;
    uint64_t u64;
} OrbisNetEpollData;

typedef struct OrbisNetEpollEvent {
	uint32_t events; /* specified by us? */
	uint32_t unk;
	uint64_t ident;	/* actually a socket fd! wtf sony? */
	OrbisNetEpollData data;
} OrbisNetEpollEvent;

/* 'RM' in little endian ASCII */
#define REMOVECTX_PORT 19794
/* 4 controllers... */
#define REMOVECTX_MAX_CLIENTS 4
/* 1 server socket + 4 controllers */
#define REMOVECTX_MAX_SOCKETS (1 + REMOVECTX_MAX_CLIENTS)
/* our current protocol version */
#define REMOVECTX_SERVER_VERSION 1
/* packet struct attributes, packed is needed */
#define REMOVECTX_PKT_ATTR __attribute__((packed))

typedef struct REMovePortData REMovePortData;

typedef struct REMoveContextClientData {
    int32_t isConnected; // ==1 live connection to the device, ==0 otherwise
    int32_t userHandle;
    int32_t counter; // latest counter value for move samples
    int32_t howmany; // how many samples are actually present in buffer[]?
    sceMoveDeviceInfo currentDeviceInfo; // sphere radius etc
    sceMoveData buffer[32]; // ring buffer-ish array of move samples
    int32_t setColorQueued;
    uint8_t red, green, blue; // only read if setColorQueued==1, reset in Client Thread
    int32_t setVibrationQueued;
    uint8_t motor; // only read if setVibrationQueued==1, reset in Client Thread
    REMovePortData *portLink;
} REMoveContextClientData;

struct REMovePortData {
    int isOpen;
    int isSphereResetQueued;
    sceUserId portUserId;
    sceInt32 deviceType;
    sceInt32 deviceIndex;
    REMoveContextClientData *clientDataLink;
};

// already typedef'd for us: -- private
struct IREMoveContextData {
    OrbisPthread serverThread;
    OrbisPthreadMutex serverMutex;
    OrbisNetId serverEpoll; // soni cumputr entrtnmt :)
    // [0] - server socket, [1..REMOVECTX_MAX_SOCKETS-1] - clients
    OrbisNetId epollSockets[REMOVECTX_MAX_SOCKETS];
    // only client data
    REMoveContextClientData clientData[REMOVECTX_MAX_CLIENTS];
    // port stuff
    REMovePortData portData[REMOVECTX_MAX_CLIENTS];
    // 1 - the thread is running, 0 - the thread is to be stopped asap
    int runThread;
};

// this is the first packet that is sent from the server to the client upon connection
typedef struct REMOVECTX_PKT_ATTR REMoveNetPacketHelloV1 {
    int32_t sizeThis; // size of the whole packet, including this field.
    // must be set to `REMOVECTX_SERVER_VERSION`, client must abort the socket if it doesn't know our protocol
    int32_t serverVersion;
    // current logged on users:
    int32_t user1;
    int32_t user2;
    int32_t user3;
    int32_t user4;
    char username1[16 + 1];
    char username2[16 + 1];
    char username3[16 + 1];
    char username4[16 + 1];
} REMoveNetPacketHelloV1;

// treat as little endian bitflags
typedef enum REMoveNetUpdateFlagsV1 {
    // use to initialize a value
    UPDATE_FLAG_SET_NONE = 0,
    // `red`, `green`, `blue` are to be read and applied
    UPDATE_FLAG_SET_VIBRATION = (1 << 0),
    // `motor` is to be read and applied
    UPDATE_FLAG_SET_LIGHTSPHERE_COLOR = (1 << 1)
} REMoveNetUpdateFlagsV1;

// this is the second and onward packet that is sent from the server to the client
typedef struct REMOVECTX_PKT_ATTR REMoveNetPacketToClientV1 {
    int32_t sizeThis; // size of the whole packet, including this field.
    // which information to update?
    int32_t updateFlagsV1;
    // if flag `UPDATE_FLAG_SET_LIGHTSPHERE_COLOR` is set: ignore otherwise
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    // if flag `UPDATE_FLAG_SET_VIBRATION` is set: ignore otherwise
    uint8_t motor;
} REMoveNetPacketToClientV1;

// this is the only packet that is set from the client to the server (otherwise it aborts the socket)
typedef struct REMOVECTX_PKT_ATTR REMoveNetPacketFromClientV1 {
    int32_t sizeThis; // size of the whole packet, including this field.
    int32_t userHandle; // to which user this client belongs.
    float accelX;
    float accelY;
    float accelZ;
    float gyroX;
    float gyroY;
    float gyroZ;
    // -- these fields are yet to be determined {
    float sensorTemperature;
    float sphereRadius;
    float accelToSphereX;
    float accelToSphereY;
    float accelToSphereZ;
    // -- these fields are yet to be determined }
    uint16_t digitalButtonsBitmask;
    uint16_t analogTButtonValue;
    // -- extension data fields {
    uint16_t uStatus;
    uint16_t uDigital1;
    uint16_t uDigital2;
    uint16_t uAnalogRightX;
    uint16_t uAnalogRightY;
    uint16_t uAnalogLeftX;
    uint16_t uAnalogLeftY;
    uint8_t baCustom[5];
    // -- extension data fields }
} REMoveNetPacketFromClientV1;

int IREMoveContext_FreeSocket(IREMoveContext *self, int sck) {
    if (sck >= 0) {
        kprintf("[removectx]: Destroying socket %d\n", sck);
        ((int(*)(int id, int op, int sock, OrbisNetEpollEvent *ev))&sceNetEpollControl)
            (self->Data->serverEpoll, 3, sck, 0);
        sceNetShutdown(sck, 2 /* RDWR */);
        return sceNetSocketClose(sck);
    }
    return 0;
}

int IREMoveContext_AddSocketToEpoll(IREMoveContext *self, int epollId, int socketId, OrbisNetEpollEvent *epollev) {
    int ret = 0, i = 0, found = 0;
    for (i = 0; i < sizeof(self->Data->epollSockets) / sizeof(self->Data->epollSockets[0]); ++i) {
        if (self->Data->epollSockets[i] < 0) {
            self->Data->epollSockets[i] = socketId;
            found = 1;
            break;
        }
    }

    if (!found) {
        kprintf("[removectx]: !!! No sockets space left :(\n");
        ret = -1;
    }
    else {
        ret = ((int(*)(int id, int op, int sock, OrbisNetEpollEvent *ev))&sceNetEpollControl)
            (epollId, 1, socketId, epollev);
        if (ret < 0) {
            self->Data->epollSockets[i] = -1;
        }
    }
    return ret;
}

int IREMoveContext_Socket2Index(IREMoveContext *self, int socketId) {
    int ind = -1, i;
    for (i = 0; i < sizeof(self->Data->epollSockets) / sizeof(self->Data->epollSockets[0]); ++i) {
        if (self->Data->epollSockets[i] == socketId) {
            ind = i;
            break;
        }
    }
    return ind;
}

int IREMoveContext_SetIndex(IREMoveContext *self, int index, int v) {
    if (index < 0 || index >= sizeof(self->Data->epollSockets) / sizeof(self->Data->epollSockets[0])) {
        return 0;
    }
    self->Data->epollSockets[index] = v;
    return 1;
}

REMovePortData *IREMoveContext_IterateThroughPorts(IREMoveContext *self, REMovePortData *(*func)(IREMoveContext *self, REMovePortData *elem, int ind, int u), int arg) {
    REMovePortData *r = 0;
    for (int i = 0; i < sizeof(self->Data->portData) / sizeof(self->Data->portData[0]); ++i) {
        r = func(self, &self->Data->portData[i], i, arg);
        if (r) {
            return r;
        }
    }
    return 0;
}

REMovePortData *IREMoveContext_HasIndex0(IREMoveContext *self, REMovePortData *elem, int ind, int u) {
    if (elem->isOpen && elem->portUserId == u && elem->deviceIndex == 0 && !elem->clientDataLink) {
        return elem;
    }
    return 0;
}

REMovePortData *IREMoveContext_HasIndex1(IREMoveContext *self, REMovePortData *elem, int ind, int u) {
    if (elem->isOpen && elem->portUserId == u && elem->deviceIndex == 1 && !elem->clientDataLink) {
        return elem;
    }
    return 0;
}

void IREMoveContext_ParseClientData(IREMoveContext *self, const REMoveNetPacketFromClientV1 *inData, REMoveNetPacketToClientV1 *outData, int myIndex) {
    int index = myIndex - 1;
    if (index < 0) {
        kprintf("[removethr]: Index for ParseClientData is invalid: %d\n", index);
        (*(volatile int*)0) = 0;
    }
    REMoveContextClientData *client = &self->Data->clientData[index];

    if (!inData || !outData) {
        if (client->portLink) {
            // unset port's data link, will make sceMove functions report a device disconnection.
            client->portLink->clientDataLink = 0;
        }
        // а вы так можете крестобляди, а? и чтоб без юб
        memset(client, 0, sizeof(*client));
    }
    else {
        client->isConnected = 1;
        if (client->counter == INT32_MAX) {
            client->counter = 0;
        }
        int thiscounter = client->counter++;
        if ((thiscounter % 10000) == 0) {
            kprintf("[removethr]: The thread is alive. counter=%d...\n", thiscounter);
        }
        // parse incoming data...
        sceMoveData dat = { 0 };
        dat.faAccelerometer[0] = inData->accelX;
        dat.faAccelerometer[1] = inData->accelY;
        dat.faAccelerometer[2] = inData->accelZ;
        dat.faGyro[0] = inData->gyroX;
        dat.faGyro[1] = inData->gyroY;
        dat.faGyro[2] = inData->gyroZ;
        dat.buttonData.uCastToEnumSceMoveDigitalButton = inData->digitalButtonsBitmask;
        dat.buttonData.uAnalogTButtonValue = inData->analogTButtonValue;
        dat.unusedExtensionPortData.uStatus = inData->uStatus;
        dat.unusedExtensionPortData.uDigital1 = inData->uDigital1;
        dat.unusedExtensionPortData.uDigital2 = inData->uDigital2;
        dat.unusedExtensionPortData.uAnalogRightX = inData->uAnalogRightX;
        dat.unusedExtensionPortData.uAnalogRightY = inData->uAnalogRightY;
        dat.unusedExtensionPortData.uAnalogLeftX = inData->uAnalogLeftX;
        dat.unusedExtensionPortData.uAnalogLeftY = inData->uAnalogLeftY;
        dat.unusedExtensionPortData.baCustom[0] = inData->baCustom[0];
        dat.unusedExtensionPortData.baCustom[1] = inData->baCustom[1];
        dat.unusedExtensionPortData.baCustom[2] = inData->baCustom[2];
        dat.unusedExtensionPortData.baCustom[3] = inData->baCustom[3];
        dat.unusedExtensionPortData.baCustom[4] = inData->baCustom[4];
        dat.dataCounter = thiscounter;
        dat.sensorTemperature = inData->sensorTemperature;
        dat.dataTimestamp = sceKernelGetProcessTime(); /* always set at the last possible moment... */

        // shift all elements by one and insert latest packet at end of the array, so [len-1]
        sceMoveData *datarray = client->buffer;
        const int datarraylen = sizeof(client->buffer) / sizeof(client->buffer[0]);
        memmove(&datarray[0], &datarray[1], (datarraylen - 1) * sizeof(client->buffer[0]));
        datarray[datarraylen - 1] = dat;
        client->howmany++;
        if (client->howmany > datarraylen) {
            client->howmany = datarraylen;
        }

        client->currentDeviceInfo.fSphereRadius = inData->sphereRadius;
        client->currentDeviceInfo.faAccelToSphereOffset[0] = inData->accelToSphereX;
        client->currentDeviceInfo.faAccelToSphereOffset[1] = inData->accelToSphereY;
        client->currentDeviceInfo.faAccelToSphereOffset[2] = inData->accelToSphereZ;
        client->userHandle = inData->userHandle;

        if (!client->portLink) {
            REMovePortData *possiblePortLink = 0;
            // try to link us to a port, try index 0 if available first.
            REMovePortData *ind0 = IREMoveContext_IterateThroughPorts(
                self, &IREMoveContext_HasIndex0, client->userHandle);
            REMovePortData *ind1 = IREMoveContext_IterateThroughPorts(
                self, &IREMoveContext_HasIndex1, client->userHandle);
            // decide which index to occupy:
            possiblePortLink = ind0 ? ind0 : ind1;
            if (possiblePortLink) {
                possiblePortLink->clientDataLink = client;
                client->portLink = possiblePortLink;
            }
        }

        if (client->portLink) {
            if (client->portLink->isSphereResetQueued) {
                client->portLink->isSphereResetQueued = 0;
                client->setColorQueued = 1;
                OrbisUserServiceUserColor col = ORBIS_USER_SERVICE_USER_COLOR_BLUE;
                sceUserServiceGetUserColor(client->userHandle, &col);
                uint8_t r = 0, g = 0, b = 0;
                switch (col) { // very rough approximation, do not take this seriously:
                    case ORBIS_USER_SERVICE_USER_COLOR_BLUE:
                    default: {
                        r = 0; g = 0; b = 255; break;
                    }
                    case ORBIS_USER_SERVICE_USER_COLOR_RED: {
                        r = 255; g = 0; b = 0; break;
                    }
                    case ORBIS_USER_SERVICE_USER_COLOR_GREEN: {
                        r = 0; g = 255; b = 0; break;
                    }
                    case ORBIS_USER_SERVICE_USER_COLOR_PINK: {
                        r = 255; g = 192; b = 203; break;
                    }
                }
                client->red = r;
                client->green = g;
                client->blue = b;
            }
        }

        outData->sizeThis = sizeof(*outData);
        outData->updateFlagsV1 = UPDATE_FLAG_SET_NONE;
        if (client->setColorQueued) {
            client->setColorQueued = 0;
            outData->updateFlagsV1 |= UPDATE_FLAG_SET_LIGHTSPHERE_COLOR;
            outData->red = client->red;
            outData->green = client->green;
            outData->blue = client->blue;
        }
        if (client->setVibrationQueued) {
            client->setVibrationQueued = 0;
            outData->updateFlagsV1 |= UPDATE_FLAG_SET_VIBRATION;
            outData->motor = client->motor;
        }
    }
}

int IREMoveContext_ServerEvent(IREMoveContext *self, int epollId, OrbisNetEpollEvent *epollev) {
    // events for the server socket:

    int s = (int)epollev->ident;
    unsigned st = (int)epollev->events;
    int sck = 0;
    int ok = 0, usersok = 0;
    OrbisNetEpollEvent newev = { 0 };
    char connfrom[32] = {'\0'};

    // server socket died?
    if (st & 0x10) {
        kprintf("[removethr]: Server EPOLLHUP! %d\n", orbis_net_errno);
        return 0;
    }
    if (st & 0x08) {
        int err = 0;
        OrbisNetSocklen_t errlen = sizeof(err);
        sceNetGetsockopt(s, 0xffff, 0x1007, &err, &errlen);
        kprintf("[removethr]: Server EPOLLERR! %d %d\n", err, orbis_net_errno);
        return 0;
    }
    // have a new connection?
    if (st & 0x01) {
        OrbisNetSockaddrIn addr = { sizeof(OrbisNetSockaddrIn) };
        OrbisNetSocklen_t addrlen = sizeof(addr);
        sck = sceNetAccept(s, (OrbisNetSockaddr*)&addr, &addrlen);
        if (sck < 0) {
            kprintf("[removethr]: EPOLLIN fail, 0x%X %d\n", sck, orbis_net_errno);
            // unable to accept client connection, that's fine, carry on...
            return 1;
        }
        // однажды в OO появились нормальные заголовки SceNet и у крапивина отвалилась жопа
        // >:(
        const char *connfroms = ((const char*(*)(int af, const void *src, char *dst, OrbisNetSocklen_t size))&sceNetInetNtop)
            (ORBIS_NET_AF_INET, &addr.sin_addr, connfrom, sizeof(connfrom) - 1);
        kprintf("[removethr]: New client connection from %s:%d (%d)\n",
            (connfroms ? connfroms : "<unknown address>"), (int)sceNetHtons(addr.sin_port), orbis_net_errno
        );
        newev.events = 1;
        newev.data.u32 = 0; // mark as client socket
        // obtain current usernames and send them to the client to pick:
        OrbisUserServiceLoginUserIdList list = { { -1, -1, -1, -1 } };
        usersok = sceUserServiceGetLoginUserIdList(&list);
        // add to epoll events thing
        ok = IREMoveContext_AddSocketToEpoll(self, epollId, sck, &newev);
        if (ok < 0 || usersok < 0) {
            kprintf("[removethr]: Unable to add client, too many or we're screwed. %d %d %d\n", ok, orbis_net_errno, usersok);
            IREMoveContext_FreeSocket(self, sck);
            // still carry on...
            return 1;
        }
        // established a connection with the client, send a HELLO packet
        REMoveNetPacketHelloV1 pkt = { 0 };
        pkt.sizeThis = sizeof(pkt);
        pkt.serverVersion = REMOVECTX_SERVER_VERSION;
        pkt.user1 = list.userId[0];
        pkt.user2 = list.userId[1];
        pkt.user3 = list.userId[2];
        pkt.user4 = list.userId[3];
        // we don't care if these functions fail or not, users are checked by the client:
        sceUserServiceGetUserName(list.userId[0], pkt.username1, sizeof(pkt.username1));
        sceUserServiceGetUserName(list.userId[1], pkt.username2, sizeof(pkt.username2));
        sceUserServiceGetUserName(list.userId[2], pkt.username3, sizeof(pkt.username3));
        sceUserServiceGetUserName(list.userId[3], pkt.username4, sizeof(pkt.username4));
        ok = sceNetSend(sck, &pkt, sizeof(pkt), 0);
        if (ok != sizeof(pkt)) {
            kprintf("[removethr]: Failed to send client hello, sck=%d,code=%X,errno=%d\n", sck, ok, orbis_net_errno);
            int sind = IREMoveContext_Socket2Index(self, sck);
            IREMoveContext_FreeSocket(self, sck);
            IREMoveContext_SetIndex(self, sind, -1);
        }
    }

    return 1;
}

int IREMoveContext_ClientEvent(IREMoveContext *self, int epollId, OrbisNetEpollEvent *epollev) {
    scePthreadMutexLock(&self->Data->serverMutex);
    // critical section: {
    int r = 1, ok = 0;
    // events for the client socket:

    int s = (int)epollev->ident;
    unsigned st = epollev->events;
    int index = IREMoveContext_Socket2Index(self, s);
    if (index < 0) {
        kprintf("[removethr]: WTF?!! %d", s);
        r = 0;
    }
    else {
        if (st & 0x10) {
            kprintf("[removethr]: Client socket EPOLLHUP %d\n", orbis_net_errno);
            IREMoveContext_FreeSocket(self, s);
            IREMoveContext_SetIndex(self, index, -1);
            IREMoveContext_ParseClientData(self, 0, 0, index);
        }
        else if (st & 0x08) {
            int err = 0;
            OrbisNetSocklen_t errlen = sizeof(err);
            sceNetGetsockopt(s, 0xffff, 0x1007, &err, &errlen);
            kprintf("[removethr]: Client socket EPOLLERR %d %d\n", err, orbis_net_errno);
            IREMoveContext_FreeSocket(self, s);
            IREMoveContext_SetIndex(self, index, -1);
            IREMoveContext_ParseClientData(self, 0, 0, index);
        }
        else if (st & 0x01) {
            REMoveNetPacketFromClientV1 pkt = { 0 };
            int got = sceNetRecv(s, &pkt, sizeof(pkt), 0);
            if (got != sizeof(pkt) || pkt.sizeThis != sizeof(pkt)) {
                kprintf("[removethr]: Client recv err %d, %d, %d.\n", got, (int)sizeof(pkt), pkt.sizeThis);
                IREMoveContext_FreeSocket(self, s);
                IREMoveContext_SetIndex(self, index, -1);
                IREMoveContext_ParseClientData(self, 0, 0, index);
            }
            else {
                REMoveNetPacketToClientV1 sendout = { 0 };
                IREMoveContext_ParseClientData(self, &pkt, &sendout, index);
                ok = sceNetSend(s, &sendout, sizeof(sendout), 0);
                if (ok != sizeof(sendout)) {
                    kprintf("[removethr]: Client send err, %d, %d, %d.\n", ok, (int)sizeof(sendout), orbis_net_errno);
                    IREMoveContext_FreeSocket(self, s);
                    IREMoveContext_SetIndex(self, index, -1);
                    IREMoveContext_ParseClientData(self, 0, 0, index);
                }
            }
        }
    }
    // }: critical section
    scePthreadMutexUnlock(&self->Data->serverMutex);
    return r;
}

void *IREMoveContext_ServerThread(void *thread_arg) {
    IREMoveContext *self = (IREMoveContext *)thread_arg;
    OrbisNetEpollEvent events[32] = { { 0 } };
    int nevents = 0, i = 0, runthr = 0;

    for (;;) {
        scePthreadMutexLock(&self->Data->serverMutex);
        // critical section: {
        runthr = self->Data->runThread;
        // }: critical section
        scePthreadMutexUnlock(&self->Data->serverMutex);

        if (!runthr) {
            break;
        }

        nevents = ((int(*)(int eid, OrbisNetEpollEvent *events, int m, int t))&sceNetEpollWait)
            (self->Data->serverEpoll, events, sizeof(events) / sizeof(events[0]), 15 * 1000 * 1000);
        if (nevents < 0) {
            kprintf("[removethr]: sceNetEpollWait failed, 0x%X %d", nevents, orbis_net_errno);
            break;
        }
        
        if (nevents == 0) {
            /* nothing to process, all our sockets are in idle */
            continue;
        }

        /* perform epoll events... */
        for (i = 0; i < nevents; ++i) {
            OrbisNetEpollEvent *ev = &events[i];
            if (ev->data.u32 & 1 /* epoll server socket flag */) {
                if (!IREMoveContext_ServerEvent(self, self->Data->serverEpoll, ev)) {
                    break;
                }
            }
            else {
                if (!IREMoveContext_ClientEvent(self, self->Data->serverEpoll, ev)) {
                    break;
                }
            }
        }
    }

    // nothing...
    return (void *)0;
}

sceError IREMoveContext_Init(IREMoveContext *self) {
    int ok = -1, sock = -1, epoll = -1, always1 = 1;
    OrbisNetEpollEvent epollev = { 0 };

    kprintf("[removectx]: Initializing IREMoveContext* V%d...\n", REMOVECTX_SERVER_VERSION);

    kprintf("[removectx]: Creating server mutex...\n");
    ok = scePthreadMutexInit(&self->Data->serverMutex, 0, "REMoveSrvMtx");
    if (ok < 0) {
        kprintf("[removectx]: failed to init the server mutex, 0x%X\n", ok);
        return SCE_MOVE_ERROR_FATAL;
    }

    kprintf("[removectx]: Creating a epoll handle...\n");
    ok = ((int(*)(const char*, int))&sceNetEpollCreate)("REMoveSrvEpl", 0);
    if (ok < 0) {
        kprintf("[removectx]: Failed to create a epoll, 0x%X %d", ok, orbis_net_errno);
        return SCE_MOVE_ERROR_FATAL;
    }

    self->Data->serverEpoll = ok;
    epoll = ok;

    kprintf("[removectx]: Creating server socket...\n");
    ok = sceNetSocket("REMoveSrvSck", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    if (ok < 0) {
        kprintf("[removectx]: unable to create a socket, 0x%X %d\n", ok, orbis_net_errno);
        return SCE_MOVE_ERROR_FATAL;
    }

    sock = ok;

    kprintf("[removectx]: Setting server socket properties...\n");
    ok = sceNetSetsockopt(sock, 0xffff, 0x4, &always1, sizeof(always1));
    if (ok < 0) {
        kprintf("[removectx]: unable to set socket SO_REUSEADDR to 1, 0x%X %d\n", ok, orbis_net_errno);
        return SCE_MOVE_ERROR_FATAL;
    }

    kprintf("[removectx]: Binding...\n");
    OrbisNetSockaddrIn sockin = { 0 };
    sockin.sin_len = sizeof(sockin);
    sockin.sin_family = ORBIS_NET_AF_INET;
    sockin.sin_port = sceNetHtons(REMOVECTX_PORT);
    ok = sceNetBind(sock, (OrbisNetSockaddr*)&sockin, sizeof(sockin));
    if (ok < 0) {
        kprintf("[removectx]: failed to bind, 0x%X %d\n", ok, orbis_net_errno);
        return SCE_MOVE_ERROR_FATAL;
    }

    kprintf("[removectx]: Starting listen...\n");
    ok = sceNetListen(sock, sizeof(self->Data->epollSockets) / sizeof(self->Data->epollSockets[0]));
    if (ok < 0) {
        kprintf("[removectx]: failed to start listen, 0x%X %d", ok, orbis_net_errno);
        return SCE_MOVE_ERROR_FATAL;
    }

    kprintf("[removectx]: Listening now on port %d, adding to epoll...\n", (int)sceNetHtons(sockin.sin_port));
    epollev.events = 1;
    epollev.data.u32 = 1; // mark events as for server socket...
    ok = IREMoveContext_AddSocketToEpoll(self, epoll, sock, &epollev);
    if (ok < 0) {
        kprintf("[removectx]: Epoll add failed, 0x%X %d", ok, orbis_net_errno);
        return SCE_MOVE_ERROR_FATAL;
    }

    kprintf("[removectx]: Creating server thread...\n");
    scePthreadMutexLock(&self->Data->serverMutex);
    // critical section: {
    self->Data->runThread = 1;
    // }: critical section
    scePthreadMutexUnlock(&self->Data->serverMutex);
    ok = scePthreadCreate(&self->Data->serverThread, 0, (void*)&IREMoveContext_ServerThread, self, "REMoveSrvThr");
    if (ok < 0) {
        kprintf("[removectx]: Failed to spawn a server thread, 0x%X\n", ok);
        return SCE_MOVE_ERROR_FATAL;
    }

    kprintf("[removectx]: Context created OK, awaiting for clients now...\n");
    return SCE_OK;
}

sceError IREMoveContext_Term(IREMoveContext *self) {
    kprintf("[removectx]: Terminating the context...\n");

    if (self->Data->serverThread) {
        kprintf("[removectx]: Shutting down thread...\n");
        scePthreadMutexLock(&self->Data->serverMutex);
        // critical section: {
        self->Data->runThread = 0;
        // }: critical section
        scePthreadMutexUnlock(&self->Data->serverMutex);
        kprintf("[removectx]: Joining thread...\n");
        scePthreadJoin(self->Data->serverThread, 0);
        kprintf("[removectx]: Thread is fully stopped\n");
        self->Data->serverThread = 0;
    }

    // this array contains server socket at [0] and client sockets at [1...]
    // it's okay if we try to destroy the same socket id twice...
    for (int i = 0; i < sizeof(self->Data->epollSockets) / sizeof(self->Data->epollSockets[0]); ++i) {
        IREMoveContext_FreeSocket(self, self->Data->epollSockets[i]);
        self->Data->epollSockets[i] = -1;
    }

    // the poll thingy has to be gone!!
    ((int(*)(int))&sceNetEpollDestroy)(self->Data->serverEpoll);
    self->Data->serverEpoll = -1;

    // we're done here...
    kprintf("[removectx]: Shutdown complete...\n");
    if (self == Context) {
        Context = NULLCTX;
        kprintf("[removectx]: Unset global context...\n");
    }

    return SCE_OK;
}

sceHandleOrError IREMoveContext_Open(IREMoveContext *self, sceUserId userId, sceMoveDeviceType deviceType, sceInt32 iDeviceIndex) {
    REMovePortData *portLink = 0;
    int ok = 0;
    char username[16 + 1] = { '\0' };

    if (deviceType != sceMoveDeviceTypeStandard) {
        kprintf("[removeAPI]: unsupported device type %d, only 0 is supported by SceMove.\n", deviceType);
        return SCE_MOVE_ERROR_INVALID_PORT;
    }

    if (iDeviceIndex != 0 && iDeviceIndex != 1) {
        kprintf("[removeAPI]: unsupported device index %d, only 0 or 1 is supported by SceMove.\n", iDeviceIndex);
        return SCE_MOVE_ERROR_INVALID_ARG;
    }

    ok = sceUserServiceGetUserName(userId, username, sizeof(username));
    if (ok < 0) {
        kprintf("[removeAPI]: invalid user id, 0x%X\n", ok);
        return ok;
    }

    kprintf("[removeAPI]: opening port for %s (uid=%d,ind=%d)\n", username, userId, iDeviceIndex);

    scePthreadMutexLock(&self->Data->serverMutex);
    // critical section: {
    int err = SCE_MOVE_ERROR_MAX_CONTROLLERS_EXCEEDED, porthandle = 0;

    for (int i = 0; i < sizeof(self->Data->portData) / sizeof(self->Data->portData[0]); ++i) {
        if (!self->Data->portData[i].isOpen) {
            portLink = &self->Data->portData[i];
            porthandle = i;
            kprintf("[removeAPI]: found free port slot at %d\n", i);
            break;
        }

        if (self->Data->portData[i].portUserId == userId
        &&  self->Data->portData[i].deviceType == deviceType
        &&  self->Data->portData[i].deviceIndex == iDeviceIndex) {
            kprintf("[removeAPI]: a port for this controller is already opened %d\n", i);
            err = SCE_MOVE_ERROR_ALREADY_OPENED;
            break;
        }
    }

    if (!portLink) {
        kprintf("[removeAPI]: no free port slots left :(\n");
        porthandle = err;
    }
    else {
        portLink->isOpen = 1;
        // need to initialize the sphere color for the device...
        portLink->isSphereResetQueued = 1;
        portLink->portUserId = userId;
        portLink->deviceType = deviceType;
        portLink->deviceIndex = iDeviceIndex;
    }
    // }: critical section
    scePthreadMutexUnlock(&self->Data->serverMutex);

    return porthandle;
}

sceError IREMoveContext_SetVibration(IREMoveContext *self, sceHandle hDeviceHandle, sceByte motorValue) {
    if (hDeviceHandle < 0 || hDeviceHandle >= (sizeof(self->Data->portData) / sizeof(self->Data->portData[0]))) {
        kprintf("[removeAPI]: %s Invalid handle value %d\n", __func__, hDeviceHandle);
        return SCE_MOVE_ERROR_INVALID_HANDLE;
    }

    sceError ret = SCE_OK;
    scePthreadMutexLock(&self->Data->serverMutex);
    // critical section: {
    if (!self->Data->portData[hDeviceHandle].isOpen) {
        ret = SCE_MOVE_ERROR_INVALID_HANDLE;
    }
    else if (!self->Data->portData[hDeviceHandle].clientDataLink) {
        ret = SCE_MOVE_RETURN_CODE_NO_CONTROLLER_CONNECTED;
    }
    else {
        self->Data->portData[hDeviceHandle].clientDataLink->setVibrationQueued = 1;
        self->Data->portData[hDeviceHandle].clientDataLink->motor = motorValue;
    }
    // }: critical section
    scePthreadMutexUnlock(&self->Data->serverMutex);
    return ret;
}

sceError IREMoveContext_SetLightSphere(IREMoveContext *self, sceHandle hDeviceHandle, sceByte redColorValue, sceByte greenColorValue, sceByte blueColorValue) {
    if (hDeviceHandle < 0 || hDeviceHandle >= (sizeof(self->Data->portData) / sizeof(self->Data->portData[0]))) {
        kprintf("[removeAPI]: %s Invalid handle value %d\n", __func__, hDeviceHandle);
        return SCE_MOVE_ERROR_INVALID_HANDLE;
    }

    sceError ret = SCE_OK;
    scePthreadMutexLock(&self->Data->serverMutex);
    // critical section: {
    if (!self->Data->portData[hDeviceHandle].isOpen) {
        ret = SCE_MOVE_ERROR_INVALID_HANDLE;
    }
    else if (!self->Data->portData[hDeviceHandle].clientDataLink) {
        ret = SCE_MOVE_RETURN_CODE_NO_CONTROLLER_CONNECTED;
    }
    else {
        self->Data->portData[hDeviceHandle].clientDataLink->setColorQueued = 1;
        self->Data->portData[hDeviceHandle].clientDataLink->red = redColorValue;
        self->Data->portData[hDeviceHandle].clientDataLink->green = greenColorValue;
        self->Data->portData[hDeviceHandle].clientDataLink->blue = blueColorValue;
    }
    // }: critical section
    scePthreadMutexUnlock(&self->Data->serverMutex);
    return ret;
}

sceError IREMoveContext_GetDeviceInfo(IREMoveContext *self, sceHandle hDeviceHandle, sceMoveDeviceInfo *pOutDeviceInfo) {
    if (hDeviceHandle < 0 || hDeviceHandle >= (sizeof(self->Data->portData) / sizeof(self->Data->portData[0]))) {
        kprintf("[removeAPI]: %s Invalid handle value %d\n", __func__, hDeviceHandle);
        return SCE_MOVE_ERROR_INVALID_HANDLE;
    }

    if (!pOutDeviceInfo) {
        return SCE_MOVE_ERROR_INVALID_ARG;
    }

    sceError ret = SCE_OK;
    scePthreadMutexLock(&self->Data->serverMutex);
    // critical section: {
    if (!self->Data->portData[hDeviceHandle].isOpen) {
        ret = SCE_MOVE_ERROR_INVALID_HANDLE;
    }
    else if (!self->Data->portData[hDeviceHandle].clientDataLink) {
        ret = SCE_MOVE_RETURN_CODE_NO_CONTROLLER_CONNECTED;
    }
    else if (!pOutDeviceInfo) {
        ret = SCE_MOVE_ERROR_INVALID_ARG;
    }
    else {
        (*pOutDeviceInfo) = self->Data->portData[hDeviceHandle].clientDataLink->currentDeviceInfo;
    }
    // }: critical section
    scePthreadMutexUnlock(&self->Data->serverMutex);
    return ret;
}

sceError IREMoveContext_ReadStateRecent(IREMoveContext *self, sceHandle hDeviceHandle, sceInt64 dataTimestampInMicroseconds, sceMoveData *paOutMoveData, sceInt32 *pOutActualLength) {
    if (hDeviceHandle < 0 || hDeviceHandle >= (sizeof(self->Data->portData) / sizeof(self->Data->portData[0]))) {
        kprintf("[removeAPI]: %s Invalid handle value %d\n", __func__, hDeviceHandle);
        return SCE_MOVE_ERROR_INVALID_HANDLE;
    }

    if (!paOutMoveData || !pOutActualLength) {
        kprintf("[removeAPI]: %s invalid args, %p %p %lld\n", __func__, (void *)paOutMoveData, (void *)pOutActualLength, dataTimestampInMicroseconds);
        return SCE_MOVE_ERROR_INVALID_ARG;
    }

    sceError ret = SCE_OK;
    scePthreadMutexLock(&self->Data->serverMutex);
    // critical section: {
    if (!self->Data->portData[hDeviceHandle].isOpen) {
        ret = SCE_MOVE_ERROR_INVALID_HANDLE;
    }
    else if (!self->Data->portData[hDeviceHandle].clientDataLink) {
        ret = SCE_MOVE_RETURN_CODE_NO_CONTROLLER_CONNECTED;
    }
    else {
        REMoveContextClientData *cl = self->Data->portData[hDeviceHandle].clientDataLink;
        const int clelemlen = sizeof(cl->buffer[0]);
        const int clarrlen = sizeof(cl->buffer) / clelemlen;
        *pOutActualLength = 0;
        for (int i = 0; i < clarrlen; ++i) {
            // if we have a timestamp match or we hit the "hole" in the data, break.
            if (cl->buffer[i].dataTimestamp > dataTimestampInMicroseconds) {
                *pOutActualLength = clarrlen - i;
                break;
            }
        }

#if 0
        if ((*pOutActualLength) <= 0) {
            // hmmmm we are lacking sample data? block the thread then ;-;
            while (cl->howmany < 2) {
                // let the server thread capture more samples:
                scePthreadMutexUnlock(&self->Data->serverMutex);
                sceKernelUsleep(10 * 1000);
                // okay, now lock it and inspect the new value:
                scePthreadMutexLock(&self->Data->serverMutex);
                // oh shit.
                if (cl->portLink == 0) {
                    return SCE_MOVE_RETURN_CODE_NO_CONTROLLER_CONNECTED;
                }
            }
            // meh.
            *pOutActualLength = clarrlen - cl->howmany;
        }
#endif /* ugly hack network speed, do not uncomment unless really have to... */

        if ((*pOutActualLength) > 0) {
            // pass the actual data to the game:
            memcpy(paOutMoveData, &cl->buffer[clarrlen - (*pOutActualLength)], clelemlen * (*pOutActualLength));
            // reset our data:
            memset(cl->buffer, 0, clarrlen * clelemlen);
            cl->howmany = 0;
        }
    }
    // }: critical section
    scePthreadMutexUnlock(&self->Data->serverMutex);
    return ret;
}

sceError IREMoveContext_ReadStateLatest(IREMoveContext *self, sceHandle hDeviceHandle, sceMoveData *pOutMoveData) {
    if (hDeviceHandle < 0 || hDeviceHandle >= (sizeof(self->Data->portData) / sizeof(self->Data->portData[0]))) {
        kprintf("[removeAPI]: %s Invalid handle value %d\n", __func__, hDeviceHandle);
        return SCE_MOVE_ERROR_INVALID_HANDLE;
    }

    if (!pOutMoveData) {
        return SCE_MOVE_ERROR_INVALID_ARG;
    }

    sceError ret = SCE_OK;
    scePthreadMutexLock(&self->Data->serverMutex);
    // critical section: {
    if (!self->Data->portData[hDeviceHandle].isOpen) {
        ret = SCE_MOVE_ERROR_INVALID_HANDLE;
    }
    else if (!self->Data->portData[hDeviceHandle].clientDataLink) {
        ret = SCE_MOVE_RETURN_CODE_NO_CONTROLLER_CONNECTED;
    }
    else {
        REMoveContextClientData *cl = self->Data->portData[hDeviceHandle].clientDataLink;
        (*pOutMoveData) = cl->buffer[
            ((sizeof(cl->buffer) / sizeof(cl->buffer[0])) - 1)
        ];
    }
    // }: critical section
    scePthreadMutexUnlock(&self->Data->serverMutex);
    return ret;
}

sceError IREMoveContext_ResetLightSphere(IREMoveContext *self, sceHandle hDeviceHandle) {
    if (hDeviceHandle < 0 || hDeviceHandle >= (sizeof(self->Data->portData) / sizeof(self->Data->portData[0]))) {
        kprintf("[removeAPI]: %s Invalid handle value %d\n", __func__, hDeviceHandle);
        return SCE_MOVE_ERROR_INVALID_HANDLE;
    }

    sceError ret = SCE_OK;
    scePthreadMutexLock(&self->Data->serverMutex);
    // critical section: {
    if (!self->Data->portData[hDeviceHandle].isOpen) {
        ret = SCE_MOVE_ERROR_INVALID_HANDLE;
    }
    else {
        self->Data->portData[hDeviceHandle].isSphereResetQueued = 1;
    }
    // }: critical section
    scePthreadMutexUnlock(&self->Data->serverMutex);
    return ret;
}

sceError IREMoveContext_Close(IREMoveContext *self, sceHandle hDeviceHandle) {
    if (hDeviceHandle < 0 || hDeviceHandle >= (sizeof(self->Data->portData) / sizeof(self->Data->portData[0]))) {
        kprintf("[removeAPI]: %s Invalid handle value %d\n", __func__, hDeviceHandle);
        return SCE_MOVE_ERROR_INVALID_HANDLE;
    }

    sceError ret = SCE_OK;
    scePthreadMutexLock(&self->Data->serverMutex);
    // critical section: {
    if (!self->Data->portData[hDeviceHandle].isOpen) {
        ret = SCE_MOVE_ERROR_INVALID_HANDLE;
    }
    else {
        REMovePortData *portLink = &self->Data->portData[hDeviceHandle];

        if (portLink->clientDataLink) {
            REMoveContextClientData *clientLink = portLink->clientDataLink;

            for (int i = 0; i < REMOVECTX_MAX_CLIENTS; ++i) {
                if (clientLink == &self->Data->clientData[i]) {
                    int sck = self->Data->epollSockets[1 + i];
                    kprintf("[removeAPI]: Freeing client socket %d\n", sck);
                    IREMoveContext_FreeSocket(self, sck);
                    self->Data->epollSockets[1 + i] = -1;
                }
            }

            memset(clientLink, 0, sizeof(*clientLink));
            kprintf("[removeAPI]: client link destroyed %d\n", hDeviceHandle);
        }

        memset(portLink, 0, sizeof(*portLink));
    }
    // }: critical section
    scePthreadMutexUnlock(&self->Data->serverMutex);
    return ret;
}

static IREMoveContextData __ctx_data__ = {
    .serverThread = 0,
    .serverMutex = 0,
    .serverEpoll = -1,
    .epollSockets = { -1, -1, -1, -1, -1 },
    .runThread = 0,
    .clientData = { { 0 } },
    .portData = { { 0 } }
};

static IREMoveContext __ctx__ = {
    // functions: --
    .Init = &IREMoveContext_Init,
    .Term = &IREMoveContext_Term,
    .Open = &IREMoveContext_Open,
    .Close = &IREMoveContext_Close,
    .ReadStateRecent = &IREMoveContext_ReadStateRecent,
    .ReadStateLatest = &IREMoveContext_ReadStateLatest,
    .SetLightSphere = &IREMoveContext_SetLightSphere,
    .SetVibration = &IREMoveContext_SetVibration,
    .GetDeviceInfo = &IREMoveContext_GetDeviceInfo,
    .ResetLightSphere = &IREMoveContext_ResetLightSphere,
    // context data: --
    .Data = &__ctx_data__
};

IREMoveContext *Context = NULLCTX;

void IREMoveContext_CreateIfNullctx() {
    if (!Context) {
        Context = &__ctx__;
        kprintf("[REMoveCtx]: Global context created at %p\n", (void *)Context);
    }
}
