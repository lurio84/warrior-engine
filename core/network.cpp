#include "network.hpp"
#include "components.hpp"

#include <enet/enet.h>
#include <iostream>
#include <cstring>

// ── ENet global lifetime ──────────────────────────────────────────────────────
namespace {
struct ENetGuard {
    ENetGuard()  { enet_initialize(); }
    ~ENetGuard() { enet_deinitialize(); }
};
static ENetGuard s_enet;
}

// ── Serialization helpers ─────────────────────────────────────────────────────
static void w8 (uint8_t* b, int& o, uint8_t  v) { b[o++] = v; }
static void w16(uint8_t* b, int& o, int16_t  v) {
    b[o++] = (uint8_t)(v & 0xFF);
    b[o++] = (uint8_t)((v >> 8) & 0xFF);
}
static uint8_t r8 (const uint8_t* b, int& o) { return b[o++]; }
static int16_t r16(const uint8_t* b, int& o) {
    int16_t v = (int16_t)(b[o] | (b[o+1] << 8)); o += 2; return v;
}

// ── init / shutdown ───────────────────────────────────────────────────────────

bool NetworkManager::init() {
    initialized_ = true;
    return true;
}

void NetworkManager::shutdown() {
    if (remote_) {
        enet_peer_disconnect_now((ENetPeer*)remote_, 0);
        remote_ = nullptr;
    }
    if (host_) {
        enet_host_destroy((ENetHost*)host_);
        host_ = nullptr;
    }
    stats_ = {};
}

// ── host ──────────────────────────────────────────────────────────────────────

bool NetworkManager::host(uint16_t port) {
    ENetAddress addr{};
    addr.host = ENET_HOST_ANY;
    addr.port = port;

    host_ = enet_host_create(&addr, 1, 1, 0, 0);
    if (!host_) {
        std::cerr << "[Net] enet_host_create failed (port " << port << ")\n";
        return false;
    }
    local_peer_id_  = 0;
    stats_.mode     = NetStats::Mode::Host;
    std::cout << "[Net] Hosting on port " << port << " (peer_id=0)\n";
    return true;
}

// ── join ──────────────────────────────────────────────────────────────────────

bool NetworkManager::join(const std::string& ip, uint16_t port) {
    host_ = enet_host_create(nullptr, 1, 1, 0, 0);
    if (!host_) {
        std::cerr << "[Net] enet_host_create (client) failed\n";
        return false;
    }

    ENetAddress addr{};
    enet_address_set_host(&addr, ip.c_str());
    addr.port = port;

    remote_ = enet_host_connect((ENetHost*)host_, &addr, 1, 0);
    if (!remote_) {
        std::cerr << "[Net] enet_host_connect failed\n";
        return false;
    }
    local_peer_id_ = 1;
    stats_.mode    = NetStats::Mode::Client;
    std::cout << "[Net] Connecting to " << ip << ":" << port << " (peer_id=1)\n";
    return true;
}

// ── tick ──────────────────────────────────────────────────────────────────────

void NetworkManager::tick(entt::registry& reg) {
    if (!host_) return;

    // El host spawnea su propio jugador en el primer tick
    if (stats_.mode == NetStats::Mode::Host &&
        !reg.valid(player_entities_[local_peer_id_])) {
        spawn_player(reg, local_peer_id_,
                     SPAWN_X[local_peer_id_], SPAWN_Y[local_peer_id_], true);
    }

    ENetEvent ev;
    while (enet_host_service((ENetHost*)host_, &ev, 0) > 0) {
        switch (ev.type) {

        case ENET_EVENT_TYPE_CONNECT: {
            // ── Server: nuevo cliente conectado ──────────────────────────────
            if (stats_.mode == NetStats::Mode::Host) {
                uint8_t client_pid = 1;
                remote_ = ev.peer;

                // 1. Enviar PKT_WELCOME al nuevo cliente
                //    [WELCOME][su_peer_id][count=1][host_pid=0][x][y]
                int16_t hx = SPAWN_X[0], hy = SPAWN_Y[0];
                entt::entity he = player_entities_[0];
                if (reg.valid(he)) {
                    auto& tf = reg.get<components::Transform>(he);
                    hx = (int16_t)tf.x; hy = (int16_t)tf.y;
                }
                uint8_t buf[16]; int o = 0;
                w8(buf, o, PKT_WELCOME);
                w8(buf, o, client_pid);    // tu peer_id
                w8(buf, o, 1);             // cuántos jugadores activos siguen
                w8(buf, o, 0);             // host peer_id
                w16(buf, o, hx);
                w16(buf, o, hy);
                send_reliable(remote_, buf, o);

                // 2. Broadcast PKT_JOIN para que todos sepan del cliente
                o = 0;
                w8(buf, o, PKT_JOIN);
                w8(buf, o, client_pid);
                w16(buf, o, SPAWN_X[client_pid]);
                w16(buf, o, SPAWN_Y[client_pid]);
                broadcast_reliable(buf, o, nullptr);

                // 3. Spawn entidad del cliente en el host
                spawn_player(reg, client_pid, SPAWN_X[client_pid], SPAWN_Y[client_pid], false);

                stats_.connected  = true;
                stats_.peer_count = 1;
                std::cout << "[Net] Cliente conectado (peer_id=1)\n";
            }
            break;
        }

        case ENET_EVENT_TYPE_RECEIVE: {
            process_packet(reg,
                           ev.packet->data, ev.packet->dataLength,
                           ev.peer);
            enet_packet_destroy(ev.packet);
            break;
        }

        case ENET_EVENT_TYPE_DISCONNECT: {
            if (stats_.mode == NetStats::Mode::Host) {
                // Destruir entidad del cliente
                destroy_player(reg, 1);
                remote_           = nullptr;
                stats_.connected  = false;
                stats_.peer_count = 0;
                std::cout << "[Net] Cliente desconectado\n";
            } else {
                // El server se cayó
                destroy_player(reg, 0);
                remote_           = nullptr;
                stats_.connected  = false;
                stats_.peer_count = 0;
                std::cout << "[Net] Desconectado del servidor\n";
            }
            break;
        }

        default: break;
        }
    }

    // Actualizar latencia (solo cliente)
    if (stats_.mode == NetStats::Mode::Client && remote_)
        stats_.latency_ms = ((ENetPeer*)remote_)->roundTripTime;
}

// ── process_packet ────────────────────────────────────────────────────────────

void NetworkManager::process_packet(entt::registry& reg,
                                    const uint8_t* data, size_t len,
                                    void* /*sender*/) {
    if (len < 1) return;
    int o = 0;
    uint8_t type = r8(data, o);

    switch (type) {

    case PKT_WELCOME: {
        // [WELCOME][mi_peer_id][count][ peer_id, x, y × count ]
        if (len < 3) return;
        uint8_t my_id = r8(data, o);
        uint8_t count = r8(data, o);
        local_peer_id_ = my_id;

        // Spawn jugadores ya existentes (host en este caso)
        for (int i = 0; i < count && o + 5 <= (int)len; ++i) {
            uint8_t pid = r8(data, o);
            int16_t x   = r16(data, o);
            int16_t y   = r16(data, o);
            spawn_player(reg, pid, x, y, false);
        }
        // Spawn jugador local
        spawn_player(reg, my_id, SPAWN_X[my_id], SPAWN_Y[my_id], true);

        stats_.connected  = true;
        stats_.peer_count = 1;
        std::cout << "[Net] Bienvenido como peer_id=" << (int)my_id << "\n";
        break;
    }

    case PKT_JOIN: {
        if (len < 6) return;
        uint8_t pid = r8(data, o);
        int16_t x   = r16(data, o);
        int16_t y   = r16(data, o);
        spawn_player(reg, pid, x, y, (pid == local_peer_id_));
        break;
    }

    case PKT_LEAVE: {
        if (len < 2) return;
        uint8_t pid = r8(data, o);
        destroy_player(reg, pid);
        break;
    }

    case PKT_MOVE: {
        if (len < 6) return;
        uint8_t pid = r8(data, o);
        int16_t x   = r16(data, o);
        int16_t y   = r16(data, o);

        // Mover entidad en el registry
        entt::entity e = player_entities_[pid];
        if (reg.valid(e)) {
            auto& tf = reg.get<components::Transform>(e);
            tf.x = (float)x;
            tf.y = (float)y;
        }

        // Si es el host, rebroadcast a otros peers
        if (stats_.mode == NetStats::Mode::Host) {
            uint8_t buf[6]; int ob = 0;
            w8(buf, ob, PKT_MOVE);
            w8(buf, ob, pid);
            w16(buf, ob, x);
            w16(buf, ob, y);
            broadcast_reliable(buf, ob, nullptr);  // en 2-player solo hay 1 peer, ok
        }
        break;
    }

    default: break;
    }
}

// ── send_move ─────────────────────────────────────────────────────────────────

void NetworkManager::send_move(int16_t x, int16_t y) {
    if (!host_ || !remote_) return;
    uint8_t buf[6]; int o = 0;
    w8(buf, o, PKT_MOVE);
    w8(buf, o, local_peer_id_);
    w16(buf, o, x);
    w16(buf, o, y);
    send_reliable(remote_, buf, o);

    // Actualizar la propia entidad local inmediatamente (no esperar el echo)
    entt::entity e = player_entities_[local_peer_id_];
    // (la entidad ya fue movida en main antes de llamar send_move)
    (void)e;
}

// ── on_registry_cleared ───────────────────────────────────────────────────────

void NetworkManager::on_registry_cleared(entt::registry& reg) {
    // Todas las entidades fueron destruidas — recrear solo el jugador local
    for (auto& e : player_entities_) e = entt::null;

    if (stats_.mode != NetStats::Mode::Offline && stats_.connected) {
        reset_local_player(reg);
    }
}

// ── spawn / destroy / reset ───────────────────────────────────────────────────

entt::entity NetworkManager::spawn_player(entt::registry& reg,
                                           uint8_t peer_id,
                                           int16_t x, int16_t y, bool local) {
    if (peer_id >= MAX_PEERS) return entt::null;
    if (reg.valid(player_entities_[peer_id]))
        return player_entities_[peer_id];  // ya existe

    auto e = reg.create();
    auto& tf = reg.emplace<components::Transform>(e);
    tf.x = (float)x;
    tf.y = (float)y;

    auto& sp = reg.emplace<components::SpriteRef>(e);
    sp.sprite_id = "player";
    sp.tint      = local ? glm::vec4{1.f, 1.f, 1.f, 1.f}   // blanco = local
                         : glm::vec4{1.f, 0.55f, 0.f, 1.f}; // naranja = remoto

    reg.emplace<components::NetworkPlayer>(e, peer_id, local);

    player_entities_[peer_id] = e;
    return e;
}

void NetworkManager::destroy_player(entt::registry& reg, uint8_t peer_id) {
    if (peer_id >= MAX_PEERS) return;
    entt::entity e = player_entities_[peer_id];
    if (reg.valid(e)) reg.destroy(e);
    player_entities_[peer_id] = entt::null;
}

void NetworkManager::reset_local_player(entt::registry& reg) {
    spawn_player(reg, local_peer_id_,
                 SPAWN_X[local_peer_id_], SPAWN_Y[local_peer_id_], true);
}

// ── stats ─────────────────────────────────────────────────────────────────────

NetStats NetworkManager::stats() const { return stats_; }

// ── send helpers ──────────────────────────────────────────────────────────────

void NetworkManager::send_reliable(void* peer, const uint8_t* data, size_t len) {
    ENetPacket* p = enet_packet_create(data, len, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send((ENetPeer*)peer, 0, p);
}

void NetworkManager::broadcast_reliable(const uint8_t* data, size_t len, void* /*exclude*/) {
    ENetPacket* p = enet_packet_create(data, len, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast((ENetHost*)host_, 0, p);
}
