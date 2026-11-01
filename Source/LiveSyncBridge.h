#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <atomic>

#pragma pack(push, 1)
struct LiveSyncData
{
    uint32_t magic;         // 0x4D423542 ('MB5B')
    uint32_t version;       // 1
    uint32_t sequence;      // Incremented on every update
    uint64_t writerInstanceId; // Unique instance ID of writer
    float params[128];      // Normalized parameter values
};
#pragma pack(pop)

class LiveSyncBridge
{
public:
    LiveSyncBridge() = default;

    ~LiveSyncBridge()
    {
        close();
    }

    void init()
    {
        if (mappedView != nullptr)
            return;

        currentPid = GetCurrentProcessId();
        myInstanceId = (static_cast<uint64_t>(currentPid) << 32) ^ static_cast<uint64_t>(reinterpret_cast<uintptr_t>(this));

        wchar_t exePath[MAX_PATH] = { 0 };
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        bool isResolve = (wcsstr(exePath, L"Resolve.exe") != nullptr || wcsstr(exePath, L"resolve.exe") != nullptr);
        bool isAudition = (wcsstr(exePath, L"Adobe Audition.exe") != nullptr || wcsstr(exePath, L"Audition.exe") != nullptr || wcsstr(exePath, L"audition.exe") != nullptr);
        bool isEqApo = (wcsstr(exePath, L"audiodg.exe") != nullptr || wcsstr(exePath, L"EqApoOutProcHost.exe") != nullptr || wcsstr(exePath, L"Editor.exe") != nullptr);

        const DWORD dataSize = sizeof(LiveSyncData);

        if (isEqApo)
        {
            // Equalizer APO: audiodg.exe runs as SYSTEM and Editor.exe runs as standard user.
            // NEVER touch physical disk files in Program Files or Public to avoid UAC permissions & lock contention.
            // We use high-speed in-RAM paging-backed named shared memory (zero disk touch).
            mapHandle = CreateFileMappingW(
                INVALID_HANDLE_VALUE,
                nullptr,
                PAGE_READWRITE,
                0,
                dataSize,
                L"Local\\MB5B_EqApoLiveSync"
            );

            if (mapHandle != nullptr)
            {
                mappedView = static_cast<LiveSyncData*>(MapViewOfFile(
                    mapHandle,
                    FILE_MAP_ALL_ACCESS,
                    0,
                    0,
                    dataSize
                ));

                if (mappedView != nullptr && mappedView->magic != 0x4D423542)
                {
                    mappedView->magic = 0x4D423542;
                    mappedView->version = 1;
                    mappedView->sequence = 0;
                    mappedView->writerInstanceId = 0;
                    std::memset(mappedView->params, 0, sizeof(mappedView->params));
                }
            }
            return;
        }

        const wchar_t* paths[3] = { nullptr, nullptr, nullptr };
        if (isResolve)
        {
            paths[0] = L"C:\\Users\\Public\\mb5_resolve.dat";
        }
        else if (isAudition)
        {
            paths[0] = L"C:\\Users\\Public\\mb5_audition.dat";
        }
        else
        {
            // Other DAWs (Reaper, FL Studio, Premiere, Ableton, etc.)
            paths[0] = L"C:\\Users\\Public\\mb5_daw.dat";
            paths[1] = L"C:\\Users\\Public\\mb5_live.dat";
        }

        for (const auto* path : paths)
        {
            if (path == nullptr)
                continue;

            fileHandle = CreateFileW(
                path,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );

            if (fileHandle != INVALID_HANDLE_VALUE)
                break;
        }

        if (fileHandle == INVALID_HANDLE_VALUE)
            return;

        mapHandle = CreateFileMappingW(
            fileHandle,
            nullptr,
            PAGE_READWRITE,
            0,
            dataSize,
            nullptr
        );

        if (mapHandle == nullptr)
        {
            CloseHandle(fileHandle);
            fileHandle = INVALID_HANDLE_VALUE;
            return;
        }

        mappedView = static_cast<LiveSyncData*>(MapViewOfFile(
            mapHandle,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            dataSize
        ));

        if (mappedView != nullptr && mappedView->magic != 0x4D423542)
        {
            // Initialize new buffer
            mappedView->magic = 0x4D423542;
            mappedView->version = 1;
            mappedView->sequence = 0;
            mappedView->writerInstanceId = 0;
            std::memset(mappedView->params, 0, sizeof(mappedView->params));
        }
    }

    void close()
    {
        if (mappedView != nullptr)
        {
            UnmapViewOfFile(mappedView);
            mappedView = nullptr;
        }
        if (mapHandle != nullptr)
        {
            CloseHandle(mapHandle);
            mapHandle = nullptr;
        }
        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(fileHandle);
            fileHandle = INVALID_HANDLE_VALUE;
        }
    }

    bool isConnected() const noexcept
    {
        return mappedView != nullptr;
    }

    // Publish parameters from GUI process
    void publish(const float* values, int count)
    {
        if (mappedView == nullptr || values == nullptr)
            return;

        int toCopy = (count < 128) ? count : 128;
        for (int i = 0; i < toCopy; ++i)
            mappedView->params[i] = values[i];

        mappedView->writerInstanceId = myInstanceId;
        _ReadWriteBarrier();
        mappedView->sequence++;
    }

    // Poll for updates in audio DSP engine
    bool pollUpdate(float* outValues, int count)
    {
        if (mappedView == nullptr || outValues == nullptr)
            return false;

        uint32_t currentSeq = mappedView->sequence;
        if (currentSeq == lastSeenSequence)
            return false;

        // If the writer is ourselves, no need to re-read
        if (mappedView->writerInstanceId == myInstanceId)
        {
            lastSeenSequence = currentSeq;
            return false;
        }

        int toCopy = (count < 128) ? count : 128;
        for (int i = 0; i < toCopy; ++i)
            outValues[i] = mappedView->params[i];

        lastSeenSequence = currentSeq;
        return true;
    }

private:
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    HANDLE mapHandle = nullptr;
    LiveSyncData* mappedView = nullptr;
    uint32_t currentPid = 0;
    uint64_t myInstanceId = 0;
    uint32_t lastSeenSequence = 0;
};
