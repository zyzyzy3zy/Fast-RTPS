// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file HelloWorld_main.cpp
 *
 */

#include <fastrtps/log/Log.h>
#include <fastrtps/utils/IPFinder.h>
#include <fastrtps/utils/IPLocator.h>
#include <fastrtps/Domain.h>
#include <fastrtps/utils/eClock.h>

using namespace eprosima;
using namespace fastrtps;
using namespace rtps;

int main(int /*argc*/, char** /*argv*/)
{
    std::vector<IPFinder::info_IP> interfaces;
    if (IPFinder::getIPs(&interfaces, false))
    {
        std::string sData = "";
        for (IPFinder::info_IP& data : interfaces)
        {
            sData += "\n**********************\nType = ";
            
            switch (data.type)
            {
            default:
            case IPFinder::IPTYPE::IP4:
                sData += "IPv4";
                break;
            case IPFinder::IPTYPE::IP4_LOCAL:
                sData += "IPv4_LOCAL";
                break;
            case IPFinder::IPTYPE::IP6:
                sData += "IPv4";
                break;
            case IPFinder::IPTYPE::IP6_LOCAL:
                sData += "IPv4 LOCAL";
                break;
            }
            
            sData += "\nscope = " + std::to_string(data.scope_id) +
                ";\nNAME= " + data.name + ";\nDEV= " + data.dev + "\nLOC= ";

            switch (data.locator.kind)
            {
            default:
            case LOCATOR_KIND_UDPv4:
                sData += "UDPv4 " + IPLocator::toIPv4string(data.locator) + " : " + std::to_string(IPLocator::getPortRTPS(data.locator));
                break;
            case LOCATOR_KIND_UDPv6:
                sData += "UDPv6 " + IPLocator::toIPv6string(data.locator) + " : " + std::to_string(IPLocator::getPortRTPS(data.locator));
                break;
            case LOCATOR_KIND_TCPv4:
                sData += "TCPv4 " + IPLocator::toIPv4string(data.locator) + " : " + std::to_string(IPLocator::getPortRTPS(data.locator)) + " : " + std::to_string(IPLocator::getLogicalPort(data.locator));
                break;
            case LOCATOR_KIND_TCPv6:
                sData += "TCPv6 " + IPLocator::toIPv6string(data.locator) + " : " + std::to_string(IPLocator::getPortRTPS(data.locator)) + " : " + std::to_string(IPLocator::getLogicalPort(data.locator));
                break;
            }
            sData += "\n";
        }
        logError(IPFinder, sData);
        eClock::my_sleep(400);
    }

    Domain::stopAll();
    Log::Reset();
    return 0;
}
