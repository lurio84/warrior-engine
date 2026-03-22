#pragma once
#include <entt/entt.hpp>
#include <string>
#include <cstdint>

// NetworkManager — Phase 9 (ENet, 2 jugadores, reliable UDP)
// El header no expone ENet para no contaminar main.cpp.

struct NetStats {
    enum class Mode { Offline, Host, Client } mode = Mode::Offline;
    int      peer_count  = 0;
    uint32_t latency_ms  = 0;   // RTT; 0 = sin medida
    bool     connected   = false;
};

class NetworkManager {
public:
    bool init();          // llama enet_initialize (idempotente)
    void shutdown();

    bool host(uint16_t port);
    bool join(const std::string& ip, uint16_t port);

    // Llama una vez por frame, fuera del bucle de timestep fijo.
    // Crea/destruye entidades de jugadores en reg según los eventos de red.
    void tick(entt::registry& reg);

    // Llamar después de reg.clear() (hot-reload) para recrear el jugador local.
    void on_registry_cleared(entt::registry& reg);

    // Envía la nueva posición del jugador local a los peers.
    void send_move(int16_t x, int16_t y);

    // Devuelve la entidad del jugador local (entt::null si no hay sesión activa).
    entt::entity local_player() const { return player_entities_[local_peer_id_]; }

    NetStats stats() const;

private:
    // ── Packet types ─────────────────────────────────────────────────────────
    enum PktType : uint8_t {
        PKT_WELCOME = 0x01,  // server→client: tu peer_id + jugadores activos
        PKT_JOIN    = 0x02,  // broadcast: nuevo jugador (peer_id, x, y)
        PKT_LEAVE   = 0x03,  // broadcast: jugador salió  (peer_id)
        PKT_MOVE    = 0x04,  // broadcast: jugador movido (peer_id, x, y)
    };

    static constexpr int     MAX_PEERS = 2;
    static constexpr int16_t SPAWN_X[MAX_PEERS] = {4, 10};
    static constexpr int16_t SPAWN_Y[MAX_PEERS] = {7,  7};

    // ENet handles — opacos al exterior
    void* host_   = nullptr;   // ENetHost*
    void* remote_ = nullptr;   // ENetPeer* (único peer remoto en 2-player)

    uint8_t     local_peer_id_ = 0;
    bool        initialized_   = false;
    NetStats    stats_;

    entt::entity player_entities_[MAX_PEERS] = {entt::null, entt::null};

    // ── Helpers ───────────────────────────────────────────────────────────────
    entt::entity spawn_player(entt::registry& reg,
                              uint8_t peer_id, int16_t x, int16_t y, bool local);
    void         destroy_player(entt::registry& reg, uint8_t peer_id);
    void         reset_local_player(entt::registry& reg);

    void process_packet(entt::registry& reg,
                        const uint8_t* data, size_t len, void* sender_peer);
    void send_reliable(void* peer, const uint8_t* data, size_t len);
    void broadcast_reliable(const uint8_t* data, size_t len, void* exclude = nullptr);
};
