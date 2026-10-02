#pragma once
#include <array>
#include <cstdint>
namespace ntp {
struct Packet { std::array<std::uint8_t, 48> bytes{}; };
struct Exchange { std::int64_t t1_ns{}; std::int64_t t2_ns{}; std::int64_t t3_ns{}; std::int64_t t4_ns{}; };
std::uint32_t read_be32(const std::uint8_t* p) noexcept;
std::uint64_t read_be64(const std::uint8_t* p) noexcept;
void write_be64(std::uint8_t* p, std::uint64_t v) noexcept;
std::uint64_t unix_ns_to_ntp64(std::int64_t unix_ns) noexcept;
std::int64_t ntp64_to_unix_ns(std::uint64_t ntp) noexcept;
Packet make_request(std::uint64_t client_tx_timestamp) noexcept;
bool valid_response(const std::uint8_t* data, std::size_t len, std::uint64_t request_tx) noexcept;
bool parse_exchange(const std::uint8_t* data, std::size_t len, std::uint64_t request_tx, Exchange& out) noexcept;
}
