#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <iostream>

class NetworkManager
{
public:
	NetworkManager();
	~NetworkManager();

	static std::string GetHypervisorIPAddress();


private:
};

#endif // NETWORKMANAGER_H
