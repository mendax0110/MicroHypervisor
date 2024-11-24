#define _WINSOCKAPI_
#include "NetworkManager.h"
#include <Windows.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <ws2tcpip.h>
#include <Iphlpapi.h>

NetworkManager::NetworkManager() {}

NetworkManager::~NetworkManager() {}

std::string NetworkManager::GetHypervisorIPAddress()
{
    ULONG bufferSize = 0;
    ULONG result = GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &bufferSize);
    if (result != ERROR_BUFFER_OVERFLOW)
    {
        throw std::runtime_error("Failed to retrieve the required buffer size for adapter information.");
    }

    std::vector<BYTE> buffer(bufferSize);
    IP_ADAPTER_ADDRESSES* adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    result = GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, adapterAddresses, &bufferSize);
    if (result != NO_ERROR)
    {
        throw std::runtime_error("Failed to retrieve network adapter information.");
    }

    IP_ADAPTER_ADDRESSES* currentAdapter = adapterAddresses;
    while (currentAdapter)
    {
        std::wstring adapterName = currentAdapter->FriendlyName;
        std::wstring description = currentAdapter->Description;

        // Convert wide strings to standard strings
        std::string adapterNameStr(adapterName.begin(), adapterName.end());
        std::string descriptionStr(description.begin(), description.end());

        // Check if this adapter is associated with the hypervisor
        if (descriptionStr.find("Hypervisor") != std::string::npos || adapterNameStr.find("vEthernet") != std::string::npos)
        {
            // Retrieve the first unicast address
            IP_ADAPTER_UNICAST_ADDRESS* unicast = currentAdapter->FirstUnicastAddress;
            if (unicast)
            {
                SOCKADDR* addr = unicast->Address.lpSockaddr;
                if (addr->sa_family == AF_INET) // IPv4
                {
                    char ipAddress[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &((struct sockaddr_in*)addr)->sin_addr, ipAddress, INET_ADDRSTRLEN);
                    return std::string(ipAddress);
                }
                else if (addr->sa_family == AF_INET6) // IPv6 (optional)
                {
                    char ipAddress[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, &((struct sockaddr_in6*)addr)->sin6_addr, ipAddress, INET6_ADDRSTRLEN);
                    return std::string(ipAddress);
                }
            }
        }

        currentAdapter = currentAdapter->Next;
    }

    throw std::runtime_error("No hypervisor virtual network adapter found.");
}
