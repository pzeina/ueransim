//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#pragma once

#include "json.hpp"

#include <bitset>
#include <cassert>
#include <cstdint>
#include <utility>

struct octet
{
  private:
    uint8_t value;

  public:
    inline octet() noexcept : value(0)
    {
    }

    /* no explicit */ inline octet(int32_t value) noexcept : value(static_cast<uint8_t>(value & 0xFF))
    {
    }

    /* no explicit */ inline octet(uint32_t value) noexcept : value(static_cast<uint8_t>(value & 0xFF))
    {
    }

    /* no explicit */ inline constexpr operator uint8_t() const
    {
        return value;
    }

    inline explicit constexpr operator int32_t() const
    {
        return static_cast<int32_t>(value);
    }

    [[nodiscard]] inline bool bit(int index) const
    {
        assert(index >= 0 && index <= 7);
        std::bitset<8> bitset = value;
        return bitset[index];
    }
};

struct octet2
{
  private:
    uint16_t value;

  public:
    inline octet2() noexcept : value(0)
    {
    }

    inline explicit octet2(int32_t value) noexcept : value(static_cast<uint16_t>(value & 0xFFFF))
    {
    }

    inline explicit octet2(uint32_t value) noexcept : value(static_cast<uint16_t>(value & 0xFFFF))
    {
    }

    inline octet2(uint8_t octet0, uint8_t octet1) noexcept
        : value{static_cast<uint16_t>((static_cast<uint32_t>(octet0) << 8U) | (static_cast<uint32_t>(octet1)))}
    {
    }

    inline uint8_t operator[](int index) const
    {
        assert(index >= 0 && index <= 1);
        return (value >> (8 - index * 8)) & 0xFF;
    }

    inline explicit constexpr operator uint32_t() const
    {
        return static_cast<uint32_t>(value);
    }

    inline explicit constexpr operator int32_t() const
    {
        return static_cast<int32_t>(value);
    }

    inline explicit constexpr operator uint16_t() const
    {
        return value;
    }

    static inline void SetTo(const octet2 &v, uint8_t *buffer)
    {
        buffer[0] = v[0];
        buffer[1] = v[1];
    }
};

struct octet3
{
  private:
    uint32_t value;

  public:
    inline octet3() noexcept : value(0)
    {
    }

    inline explicit octet3(int32_t value) noexcept : value(static_cast<uint32_t>(value & 0xFFFFFF))
    {
    }

    inline explicit octet3(uint32_t value) noexcept : value(value & 0xFFFFFF)
    {
    }

    inline octet3(uint8_t octet0, uint8_t octet1, uint8_t octet2) noexcept
        : value{(static_cast<uint32_t>(octet0) << 16U) | (static_cast<uint32_t>(octet1) << 8U) |
                (static_cast<uint32_t>(octet2))}
    {
    }

    inline uint8_t operator[](int index) const
    {
        assert(index >= 0 && index <= 2);
        return (value >> (16 - index * 8)) & 0xFF;
    }

    inline explicit constexpr operator int32_t() const
    {
        return static_cast<int32_t>(value);
    }

    inline explicit constexpr operator uint32_t() const
    {
        return value;
    }

    static inline void SetTo(const octet3 &v, uint8_t *buffer)
    {
        buffer[0] = v[0];
        buffer[1] = v[1];
        buffer[2] = v[2];
    }
};

struct octet4
{
  private:
    uint32_t value;

  public:
    inline octet4() noexcept : value(0)
    {
    }

    inline explicit octet4(int32_t value) noexcept : value(static_cast<uint32_t>(value))
    {
    }

    inline explicit octet4(uint32_t value) noexcept : value(value)
    {
    }

    inline explicit octet4(size_t value) noexcept : value(static_cast<uint32_t>(value))
    {
    }

    inline octet4(uint8_t octet0, uint8_t octet1, uint8_t octet2, uint8_t octet3) noexcept
        : value{(static_cast<uint32_t>(octet0) << 24U) | (static_cast<uint32_t>(octet1) << 16U) |
                ((static_cast<uint32_t>(octet2) << 8U)) | (static_cast<uint32_t>(octet3))}
    {
    }

    inline uint8_t operator[](int index) const
    {
        assert(index >= 0 && index <= 3);
        return (value >> (24 - index * 8)) & 0xFF;
    }

    inline explicit constexpr operator int32_t() const
    {
        return static_cast<int32_t>(value);
    }

    inline explicit constexpr operator uint32_t() const
    {
        return value;
    }

    inline explicit constexpr operator int64_t() const
    {
        return static_cast<int64_t>(value);
    }

    inline explicit constexpr operator uint64_t() const
    {
        return static_cast<int64_t>(value);
    }

    inline bool operator==(const octet4 &other) const
    {
        return value == other.value;
    }

    static inline void SetTo(const octet4 &v, uint8_t *buffer)
    {
        buffer[0] = v[0];
        buffer[1] = v[1];
        buffer[2] = v[2];
        buffer[3] = v[3];
    }
};

struct octet8
{
  private:
    uint64_t value;

  public:
    inline octet8() noexcept : value(0)
    {
    }

    inline explicit octet8(int64_t value) noexcept : value(static_cast<uint64_t>(value))
    {
    }

    inline explicit octet8(uint64_t value) noexcept : value(value)
    {
    }

    inline octet8(uint8_t octet0, uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4, uint8_t octet5,
                  uint8_t octet6, uint8_t octet7) noexcept
        : value{(static_cast<uint64_t>(octet0) << 56U) | (static_cast<uint64_t>(octet1) << 48U) |
                ((static_cast<uint64_t>(octet2) << 40U)) | (static_cast<uint64_t>(octet3) << 32U) |
                (static_cast<uint64_t>(octet4) << 24U) | (static_cast<uint64_t>(octet5) << 16U) |
                (static_cast<uint64_t>(octet6) << 8U) | (static_cast<uint64_t>(octet7))}
    {
    }

    inline uint8_t operator[](int index) const
    {
        assert(index >= 0 && index <= 7);
        return (value >> (56 - index * 8)) & 0xFF;
    }

    inline explicit constexpr operator int64_t() const
    {
        return static_cast<int64_t>(value);
    }

    inline explicit constexpr operator uint64_t() const
    {
        return value;
    }

    static inline void SetTo(const octet8 &v, uint8_t *buffer)
    {
        buffer[0] = v[0];
        buffer[1] = v[1];
        buffer[2] = v[2];
        buffer[3] = v[3];
        buffer[4] = v[4];
        buffer[5] = v[5];
        buffer[6] = v[6];
        buffer[7] = v[7];
    }
};

Json ToJson(const octet &v);
Json ToJson(const octet2 &v);
Json ToJson(const octet3 &v);
Json ToJson(const octet4 &v);
Json ToJson(const octet8 &v);
