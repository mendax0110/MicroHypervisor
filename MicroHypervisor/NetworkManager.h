#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <iostream>

class NetworkManager
{
public:
	NetworkManager();
	~NetworkManager();

	/**
	 * @brief Function to get the Hypervisor IP Address
	 * 
	 * @return 
	 */
	static std::string GetHypervisorIPAddress();


private:
};

#endif // NETWORKMANAGER_H
