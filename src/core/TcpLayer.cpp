// LCEServer — TCP layer implementation
// Closely follows WinsockNetLayer from the LCE source
#include "TcpLayer.h"
#include <algorithm>
#include <memory>

namespace LCEServer
{
    namespace
    {
        struct RecvThreadStart
        {
            TcpLayer* self = nullptr;
            SOCKET socket = INVALID_SOCKET;
            uint8_t smallId = 0;
        };

        void CloseSocketQuietly(SOCKET& sock)
        {
            if (sock == INVALID_SOCKET)
                return;

            shutdown(sock, SD_BOTH);
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
    }

    // Helper: receive exactly len bytes
    static bool RecvExact(SOCKET sock, uint8_t* buf, int len)
    {
        int total = 0;
        while (total < len)
        {
            int r = recv(sock, reinterpret_cast<char*>(buf) + total, len - total, 0);
            if (r <= 0) return false;
            total += r;
        }
        return true;
    }

    TcpLayer::TcpLayer() {}

    TcpLayer::~TcpLayer()
    {
        Shutdown();
    }

    bool TcpLayer::Initialize()
    {
        if (m_initialized) return true;

        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            Logger::Error("TCP", "WSAStartup failed: %d", result);
            return false;
        }

        InitializeCriticalSection(&m_connectionsLock);
        InitializeCriticalSection(&m_freeSmallIdLock);
        InitializeCriticalSection(&m_smallIdToSocketLock);
        InitializeCriticalSection(&m_sendLock);
        InitializeCriticalSection(&m_ipCacheLock);

        m_smallIdToSocket.fill(INVALID_SOCKET);
        for (auto& ip : m_ipCache)
            ip.clear();

        m_initialized = true;
        return true;
    }

    void TcpLayer::Shutdown()
    {
        m_active = false;

        if (m_listenSocket != INVALID_SOCKET)
        {
            CloseSocketQuietly(m_listenSocket);
        }

        if (m_acceptThread != NULL)
        {
            WaitForSingleObject(m_acceptThread, 3000);
            CloseHandle(m_acceptThread);
            m_acceptThread = NULL;
        }

        // Close all connections and wait for recv threads
        std::vector<HANDLE> recvThreads;
        EnterCriticalSection(&m_connectionsLock);
        for (auto& c : m_connections)
        {
            c.active = false;
            CloseSocketQuietly(c.socket);
            if (c.recvThread != NULL)
            {
                recvThreads.push_back(c.recvThread);
                c.recvThread = NULL;
            }
        }
        m_connections.clear();
        LeaveCriticalSection(&m_connectionsLock);

        EnterCriticalSection(&m_smallIdToSocketLock);
        m_smallIdToSocket.fill(INVALID_SOCKET);
        LeaveCriticalSection(&m_smallIdToSocketLock);

        EnterCriticalSection(&m_ipCacheLock);
        for (auto& ip : m_ipCache)
            ip.clear();
        LeaveCriticalSection(&m_ipCacheLock);

        for (auto h : recvThreads)
        {
            WaitForSingleObject(h, 2000);
            CloseHandle(h);
        }

        if (m_initialized)
        {
            DeleteCriticalSection(&m_connectionsLock);
            DeleteCriticalSection(&m_freeSmallIdLock);
            DeleteCriticalSection(&m_smallIdToSocketLock);
            DeleteCriticalSection(&m_sendLock);
            DeleteCriticalSection(&m_ipCacheLock);
            WSACleanup();
            m_initialized = false;
        }
    }

    bool TcpLayer::Host(int port, const char* bindIp)
    {
        if (!m_initialized && !Initialize()) return false;

        // Reserve smallIds 0-3 for host local pads. The real dedicated
        // server uses the game's IQNet player array where 0-3 are local
        // pad slots. External clients must start at 4+ to avoid the
        // client's GameNetworkManager treating them as local splitscreen.
        m_nextSmallId = 4;

        EnterCriticalSection(&m_freeSmallIdLock);
        m_freeSmallIds.clear();
        LeaveCriticalSection(&m_freeSmallIdLock);

        EnterCriticalSection(&m_smallIdToSocketLock);
        m_smallIdToSocket.fill(INVALID_SOCKET);
        LeaveCriticalSection(&m_smallIdToSocketLock);

        EnterCriticalSection(&m_ipCacheLock);
        for (auto& ip : m_ipCache)
            ip.clear();
        LeaveCriticalSection(&m_ipCacheLock);

        struct addrinfo hints = {};
        struct addrinfo* result = NULL;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = (bindIp == nullptr || bindIp[0] == 0)
                         ? AI_PASSIVE : 0;

        std::array<char, 16> portStr{};
        sprintf_s(portStr.data(), portStr.size(), "%d", port);
        const char* resolvedIp = (bindIp && bindIp[0]) ? bindIp : NULL;

        int iResult = getaddrinfo(resolvedIp, portStr.data(), &hints, &result);
        if (iResult != 0)
        {
            Logger::Error("TCP", "getaddrinfo failed for %s:%d - %d",
                resolvedIp ? resolvedIp : "*", port, iResult);
            return false;
        }

        m_listenSocket = socket(result->ai_family,
                                result->ai_socktype,
                                result->ai_protocol);
        if (m_listenSocket == INVALID_SOCKET)
        {
            Logger::Error("TCP", "socket() failed: %d", WSAGetLastError());
            freeaddrinfo(result);
            return false;
        }

        int opt = 1;
        if (setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR,
                       reinterpret_cast<const char*>(&opt),
                       sizeof(opt)) == SOCKET_ERROR)
        {
            Logger::Warn("TCP", "setsockopt(SO_REUSEADDR) failed: %d",
                WSAGetLastError());
        }

        iResult = ::bind(m_listenSocket, result->ai_addr,
                         static_cast<int>(result->ai_addrlen));
        freeaddrinfo(result);

        if (iResult == SOCKET_ERROR)
        {
            Logger::Error("TCP", "bind() failed: %d", WSAGetLastError());
            CloseSocketQuietly(m_listenSocket);
            return false;
        }

        iResult = listen(m_listenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR)
        {
            Logger::Error("TCP", "listen() failed: %d", WSAGetLastError());
            CloseSocketQuietly(m_listenSocket);
            return false;
        }

        m_active = true;
        m_acceptThread = CreateThread(NULL, 0, AcceptThreadProc,
                                      this, 0, NULL);
        if (m_acceptThread == NULL)
        {
            Logger::Error("TCP", "CreateThread(accept) failed: %d",
                GetLastError());
            m_active = false;
            CloseSocketQuietly(m_listenSocket);
            return false;
        }

        Logger::Info("TCP", "Listening on %s:%d",
            resolvedIp ? resolvedIp : "*", port);
        return true;
    }

    bool TcpLayer::SendOnSocket(SOCKET sock, const void* data, int dataSize)
    {
        if (sock == INVALID_SOCKET || dataSize <= 0) return false;

        // 4-byte big-endian length header (matches WinsockNetLayer) (matches WinsockNetLayer)
        std::array<uint8_t, 4> header{};
        header[0] = static_cast<uint8_t>((dataSize >> 24) & 0xFF);
        header[1] = static_cast<uint8_t>((dataSize >> 16) & 0xFF);
        header[2] = static_cast<uint8_t>((dataSize >> 8) & 0xFF);
        header[3] = static_cast<uint8_t>(dataSize & 0xFF);

        int totalSent = 0;
        while (totalSent < 4)
        {
            int s = send(sock, reinterpret_cast<const char*>(header.data()) + totalSent,
                         4 - totalSent, 0);
            if (s == SOCKET_ERROR || s == 0) return false;
            totalSent += s;
        }

        totalSent = 0;
        while (totalSent < dataSize)
        {
            int s = send(sock, static_cast<const char*>(data) + totalSent,
                         dataSize - totalSent, 0);
            if (s == SOCKET_ERROR || s == 0) return false;
            totalSent += s;
        }
        return true;
    }

    bool TcpLayer::SendToSmallId(uint8_t smallId,
                                  const void* data, int dataSize)
    {
        if (!m_active) return false;
        SOCKET sock = GetSocketForSmallId(smallId);
        if (sock == INVALID_SOCKET) return false;

        EnterCriticalSection(&m_sendLock);
        bool ok = SendOnSocket(sock, data, dataSize);
        LeaveCriticalSection(&m_sendLock);
        return ok;
    }

    void TcpLayer::SendReject(SOCKET sock, DisconnectReason reason)
    {
        // Matches WinsockNetLayer: 0xFF sentinel + packet id 255 +
        // 4-byte big-endian reason
        std::array<uint8_t, 6> buf{};
        buf[0] = SMALLID_REJECT;
        buf[1] = 255; // DisconnectPacket id
        int r = static_cast<int>(reason);
        buf[2] = static_cast<uint8_t>((r >> 24) & 0xFF);
        buf[3] = static_cast<uint8_t>((r >> 16) & 0xFF);
        buf[4] = static_cast<uint8_t>((r >> 8) & 0xFF);
        buf[5] = static_cast<uint8_t>(r & 0xFF);
        if (send(sock, reinterpret_cast<const char*>(buf.data()),
                 static_cast<int>(buf.size()), 0) == SOCKET_ERROR)
        {
            Logger::Warn("TCP", "Failed to send reject packet: %d",
                WSAGetLastError());
        }
    }

    SOCKET TcpLayer::GetSocketForSmallId(uint8_t smallId)
    {
        EnterCriticalSection(&m_smallIdToSocketLock);
        SOCKET s = m_smallIdToSocket[smallId];
        LeaveCriticalSection(&m_smallIdToSocketLock);
        return s;
    }

    std::string TcpLayer::GetRemoteIp(uint8_t smallId)
    {
        EnterCriticalSection(&m_ipCacheLock);
        std::string ip = m_ipCache[smallId];
        LeaveCriticalSection(&m_ipCacheLock);
        return ip;
    }

    TcpConnection* TcpLayer::FindConnectionLocked(uint8_t smallId)
    {
        for (auto& connection : m_connections)
        {
            if (connection.smallId == smallId)
                return &connection;
        }
        return nullptr;
    }

    void TcpLayer::ClearConnectionCaches(uint8_t smallId)
    {
        EnterCriticalSection(&m_smallIdToSocketLock);
        m_smallIdToSocket[smallId] = INVALID_SOCKET;
        LeaveCriticalSection(&m_smallIdToSocketLock);

        EnterCriticalSection(&m_ipCacheLock);
        m_ipCache[smallId].clear();
        LeaveCriticalSection(&m_ipCacheLock);
    }

    void TcpLayer::CloseConnection(uint8_t smallId)
    {
        EnterCriticalSection(&m_connectionsLock);
        if (auto* connection = FindConnectionLocked(smallId))
        {
            connection->active = false;
            CloseSocketQuietly(connection->socket);
        }
        LeaveCriticalSection(&m_connectionsLock);
    }

    void TcpLayer::PushFreeSmallId(uint8_t smallId)
    {
        HANDLE recvThread = NULL;
        EnterCriticalSection(&m_connectionsLock);
        if (auto* connection = FindConnectionLocked(smallId))
            recvThread = connection->recvThread;
        LeaveCriticalSection(&m_connectionsLock);

        if (recvThread != NULL)
        {
            DWORD waitResult = WaitForSingleObject(recvThread, 2000);
            if (waitResult == WAIT_TIMEOUT)
            {
                Logger::Warn("TCP",
                    "Timed out waiting for recv thread on smallId=%d; not recycling id yet",
                    smallId);
                return;
            }
            if (waitResult == WAIT_FAILED)
            {
                Logger::Warn("TCP",
                    "WaitForSingleObject failed for smallId=%d: %d",
                    smallId, GetLastError());
                return;
            }
        }

        EnterCriticalSection(&m_connectionsLock);
        for (auto& connection : m_connections)
        {
            if (connection.smallId != smallId)
                continue;

            if (connection.recvThread != NULL)
            {
                CloseHandle(connection.recvThread);
                connection.recvThread = NULL;
            }
            break;
        }
        m_connections.erase(
            std::remove_if(m_connections.begin(), m_connections.end(),
                [smallId](const TcpConnection& connection) {
                    return connection.smallId == smallId;
                }),
            m_connections.end());
        LeaveCriticalSection(&m_connectionsLock);

        ClearConnectionCaches(smallId);

        EnterCriticalSection(&m_freeSmallIdLock);
        if (std::find(m_freeSmallIds.begin(), m_freeSmallIds.end(), smallId)
            == m_freeSmallIds.end())
        {
            m_freeSmallIds.push_back(smallId);
        }
        LeaveCriticalSection(&m_freeSmallIdLock);
    }

    // Accept loop — mirrors WinsockNetLayer::AcceptThreadProc
    DWORD WINAPI TcpLayer::AcceptThreadProc(LPVOID param)
    {
        TcpLayer* self = static_cast<TcpLayer*>(param);

        while (self->m_active)
        {
            sockaddr_in remoteAddr;
            ZeroMemory(&remoteAddr, sizeof(remoteAddr));
            int addrLen = sizeof(remoteAddr);
            SOCKET clientSock = accept(self->m_listenSocket,
                reinterpret_cast<sockaddr*>(&remoteAddr), &addrLen);

            if (clientSock == INVALID_SOCKET)
            {
                if (self->m_active)
                    Logger::Error("TCP", "accept() failed: %d",
                                  WSAGetLastError());
                break;
            }

            int noDelay = 1;
            if (setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY,
                           reinterpret_cast<const char*>(&noDelay),
                           sizeof(noDelay)) == SOCKET_ERROR)
            {
                Logger::Warn("TCP",
                    "setsockopt(TCP_NODELAY) failed: %d",
                    WSAGetLastError());
            }

            // Resolve remote IP
            std::array<char, 64> ipBuf{};
            const char* resolvedRemoteIp = inet_ntop(AF_INET, &remoteAddr.sin_addr,
                ipBuf.data(), static_cast<DWORD>(ipBuf.size()));
            std::string remoteIp = resolvedRemoteIp ? resolvedRemoteIp : "<unknown>";

            Logger::Debug("TCP", "Incoming connection from %s", remoteIp.c_str());

            // IP ban check
            if (self->m_ipBanCheck && self->m_ipBanCheck(remoteIp))
            {
                Logger::Info("TCP", "Rejected %s: IP banned", remoteIp.c_str());
                SendReject(clientSock, DisconnectReason::Banned);
                CloseSocketQuietly(clientSock);
                continue;
            }

            // Server full check
            if (self->m_canAcceptCheck && !self->m_canAcceptCheck())
            {
                Logger::Info("TCP", "Rejected %s: server full", remoteIp.c_str());
                SendReject(clientSock, DisconnectReason::ServerFull);
                CloseSocketQuietly(clientSock);
                continue;
            }

            // Allocate a smallId
            uint8_t assignedId;
            EnterCriticalSection(&self->m_freeSmallIdLock);
            if (!self->m_freeSmallIds.empty())
            {
                assignedId = self->m_freeSmallIds.back();
                self->m_freeSmallIds.pop_back();
            }
            else if (self->m_nextSmallId < static_cast<unsigned>(MAX_PLAYERS_HARD))
            {
                assignedId = static_cast<uint8_t>(self->m_nextSmallId++);
            }
            else
            {
                LeaveCriticalSection(&self->m_freeSmallIdLock);
                Logger::Info("TCP", "Rejected %s: no smallIds", remoteIp.c_str());
                SendReject(clientSock, DisconnectReason::ServerFull);
                CloseSocketQuietly(clientSock);
                continue;
            }
            LeaveCriticalSection(&self->m_freeSmallIdLock);

            // Send the assigned smallId (1 byte) — client expects this
            std::array<uint8_t, 1> assignBuf{ assignedId };
            int sent = send(clientSock,
                            reinterpret_cast<const char*>(assignBuf.data()),
                            static_cast<int>(assignBuf.size()), 0);
            if (sent != 1)
            {
                Logger::Warn("TCP", "Failed to send smallId to %s", remoteIp.c_str());
                CloseSocketQuietly(clientSock);

                EnterCriticalSection(&self->m_freeSmallIdLock);
                if (std::find(self->m_freeSmallIds.begin(), self->m_freeSmallIds.end(), assignedId)
                    == self->m_freeSmallIds.end())
                {
                    self->m_freeSmallIds.push_back(assignedId);
                }
                LeaveCriticalSection(&self->m_freeSmallIdLock);
                continue;
            }

            // Register connection
            TcpConnection conn;
            conn.socket = clientSock;
            conn.smallId = assignedId;
            conn.remoteIp = remoteIp;
            conn.active = true;
            conn.recvThread = NULL;

            EnterCriticalSection(&self->m_connectionsLock);
            self->m_connections.push_back(conn);
            LeaveCriticalSection(&self->m_connectionsLock);

            // Register in lookup tables
            EnterCriticalSection(&self->m_smallIdToSocketLock);
            self->m_smallIdToSocket[assignedId] = clientSock;
            LeaveCriticalSection(&self->m_smallIdToSocketLock);

            EnterCriticalSection(&self->m_ipCacheLock);
            self->m_ipCache[assignedId] = remoteIp;
            LeaveCriticalSection(&self->m_ipCacheLock);

            Logger::Info("TCP", "Accepted %s -> smallId=%d",
                         remoteIp.c_str(), assignedId);

            // Spawn recv thread — pass connIdx + self pointer
            auto recvParam = std::make_unique<RecvThreadStart>();
            recvParam->self = self;
            recvParam->socket = clientSock;
            recvParam->smallId = assignedId;

            HANDLE hThread = CreateThread(NULL, 0, RecvThreadProc,
                                          recvParam.get(), 0, NULL);
            if (hThread == NULL)
            {
                Logger::Error("TCP",
                    "CreateThread(recv) failed for smallId=%d: %d",
                    assignedId, GetLastError());

                CloseSocketQuietly(clientSock);

                EnterCriticalSection(&self->m_connectionsLock);
                self->m_connections.erase(
                    std::remove_if(self->m_connections.begin(), self->m_connections.end(),
                        [assignedId](const TcpConnection& connection) {
                            return connection.smallId == assignedId;
                        }),
                    self->m_connections.end());
                LeaveCriticalSection(&self->m_connectionsLock);

                self->ClearConnectionCaches(assignedId);

                EnterCriticalSection(&self->m_freeSmallIdLock);
                if (std::find(self->m_freeSmallIds.begin(), self->m_freeSmallIds.end(), assignedId)
                    == self->m_freeSmallIds.end())
                {
                    self->m_freeSmallIds.push_back(assignedId);
                }
                LeaveCriticalSection(&self->m_freeSmallIdLock);
                continue;
            }

            recvParam.release();

            EnterCriticalSection(&self->m_connectionsLock);
            if (auto* connection = self->FindConnectionLocked(assignedId))
                connection->recvThread = hThread;
            LeaveCriticalSection(&self->m_connectionsLock);
        }
        return 0;
    }

    // Per-connection receive loop — mirrors WinsockNetLayer::RecvThreadProc
    DWORD WINAPI TcpLayer::RecvThreadProc(LPVOID param)
    {
        std::unique_ptr<RecvThreadStart> start(
            static_cast<RecvThreadStart*>(param));
        if (!start || start->self == nullptr)
            return 0;

        TcpLayer* self = start->self;
        SOCKET sock = start->socket;
        uint8_t clientSmallId = start->smallId;

        std::vector<uint8_t> recvBuf(RECV_BUFFER_SIZE);

        while (self->m_active)
        {
            // Read 4-byte big-endian length header
            std::array<uint8_t, 4> header{};
            if (!RecvExact(sock, header.data(), static_cast<int>(header.size())))
            {
                Logger::Debug("TCP", "Client smallId=%d disconnected (header)",
                              clientSmallId);
                break;
            }

            int packetSize = (static_cast<int>(header[0]) << 24) |
                             (static_cast<int>(header[1]) << 16) |
                             (static_cast<int>(header[2]) << 8) |
                             static_cast<int>(header[3]);

            if (packetSize <= 0 || packetSize > MAX_PACKET_SIZE)
            {
                Logger::Warn("TCP", "Invalid packet size %d from smallId=%d",
                             packetSize, clientSmallId);
                break;
            }

            if (static_cast<int>(recvBuf.size()) < packetSize)
                recvBuf.resize(packetSize);

            if (!RecvExact(sock, &recvBuf[0], packetSize))
            {
                Logger::Debug("TCP", "Client smallId=%d disconnected (body)",
                              clientSmallId);
                break;
            }

            // Deliver to callback
            if (self->m_dataCallback)
                self->m_dataCallback(clientSmallId, &recvBuf[0], packetSize);
        }

        // Mark disconnected
        EnterCriticalSection(&self->m_connectionsLock);
        if (auto* connection = self->FindConnectionLocked(clientSmallId))
        {
            connection->active = false;
            if (connection->socket == sock)
            {
                CloseSocketQuietly(connection->socket);
            }
        }
        LeaveCriticalSection(&self->m_connectionsLock);

        // Notify disconnect
        if (self->m_disconnectCallback)
            self->m_disconnectCallback(clientSmallId);

        return 0;
    }

} // namespace LCEServer
