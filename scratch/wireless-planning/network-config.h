#ifndef NETWORK_DATA_STRUCT_H
#define NETWORK_DATA_STRUCT_H

#include <vector>

#include "ns3/ipv4-address.h"
#include "ns3/type-id.h"
#include "ns3/object.h"
#include "ns3/vector.h"
#include "ns3/wimax-phy.h"
#include "ns3/wimax-helper.h"
#include "ns3/wimax-module.h"

using namespace std;

namespace ns3 {

    /**
     * @brief The objetive of this class is to enable the access to NetworkData struct.
     *
     *  Comes from the need to create a node, taken information manually.
     *
     *
     * The names of the attributes are self-explanatory.
     *
     * Many typedef are defined to make the code more readable.
     *
     *
     * How can I define new types without constructing a class?
     */
    class NetworkConfig {
    public:

        enum CommunicationStandard {
            WIFI,
            WIMAX
        };

        /**
         * @enum macType
         *
         * @brief Enumeration of the possible types of MAC.
         *
         */
        enum MacType {
            AP, ///< Access Point non-QoSWIFI.   0
            STA, ///< Station non-QoS        .   1
            ADHOC, ///< Ad hoc non-QoS       .   2
            QAP, ///< QoS Access Point       .   3
            QSTA, ///< QoS Station           .   4
            QADHOC, ///< QoS Ad hoc          .   5
            UNDEF
        };

        /**
         * @struct Device Data
         */
        typedef struct {
            string name; ///< Unused          
            Ipv4Address ipAddress; ///< Unused
            //Separte chs for wifi and wimax?
            uint16_t chId; ///< Unique Ch ID to wich the device is connected to.
            double distance; ///< Distance to AP/BS
            double max_distance; ///< Max distance to AP/BS from the furthest station in network 
            enum CommunicationStandard commStandard;
            //WIFI
            enum MacType wifiMacType; // AP / STA / ADHOC + QoS / non-QoS
            //WIMAX
            enum WimaxHelper::NetDeviceType wimaxDeviceType; /// BS / SS
            enum WimaxHelper::SchedulerType scherdulerType;
            enum WimaxPhy::ModulationType modulationType;
        } DeviceData;

        typedef vector<DeviceData> VectorDeviceData;

        typedef struct {
            string name;
            Vector position;
            VectorDeviceData vectorDeviceData; ///< Contains all the devices of a node. Each interface corresponds to a device.
        } NodeData;

        typedef vector<NodeData> VectorNodeData;

        typedef struct {
            uint16_t id; ///< channel number
            string wifiMode; ///< e.g. wifia-6mbs @see WifiPhy
        } WifiChannelData;
        typedef vector<WifiChannelData> VectorWifiChannelData;

        typedef struct {
            uint16_t id; ///< channel number
            WimaxHelper wimax;
            Ptr<SimpleOfdmWimaxChannel> channel;
        } WimaxChannelData;
        typedef vector<WimaxChannelData> VectorWimaxChannelData;

        typedef struct {
            VectorWifiChannelData vWifiChData;
            VectorWimaxChannelData vWimaxChData;
        } VectorChannelData;

        typedef struct {
            VectorChannelData vectorChannelData;
            VectorNodeData vectorNodeData;
        } NetworkData;

        NetworkConfig();
        ~NetworkConfig();

        static TypeId GetTypeId(void);

        /**
         * @brief Sets device data into DeviceData struct.
         *
         * @param name
         * @param IP address
         * @param channel number (Unused, remove it?)
         * @param macType AP, STA or ADHOC (enum type)
         *
         * @return NetworkConfig::DeviceData
         */
        NetworkConfig::DeviceData SetWifiDeviceData(string name, Ipv4Address address,
                uint16_t chId, enum NetworkConfig::MacType macType);

        /**
         * @brief Sets device data into DeviceData struct.
         *
         * @param channel id (link id)
         * @param macType AP, STA or ADHOC (enum type)
         * @param distance to AP (meters)
         *
         * @return NetworkConfig::DeviceData
         */
        NetworkConfig::DeviceData SetWifiDeviceData(uint16_t chId,
                enum NetworkConfig::MacType macType, double distance, double max_distance);

        /**
         * @brief Sets channel data into a vector of channel data struct.
         *
         * @param ID number
         * @param mode @see WifiPhy 
         * @param vectorWifiChannelData stores all the information of all the channels of the network. When we set a channel it is automatically added.
         */
        void SetWifiChannelData(uint16_t id, string mode, NetworkConfig::VectorWifiChannelData& vectorWifiChannelData);

        NetworkConfig::DeviceData SetWimaxDeviceData(uint16_t chId, double distance,
                double max_distance,
                enum WimaxHelper::NetDeviceType wimaxDeviceType,
                enum WimaxHelper::SchedulerType scherdulerType,
                enum WimaxPhy::ModulationType modulationType);

        NetworkData m_networkData;
    };

} // namespace ns3

#endif /* NETWORK_DATA_STRUCT_H */
