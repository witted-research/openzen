#ifndef ZEN_COMPONENTS_IMUCOMPONENT_H_
#define ZEN_COMPONENTS_IMUCOMPONENT_H_

#include <atomic>

#include "SensorComponent.h"
#include "io/interfaces/AsyncIoInterface.h"
#include "utility/Ownership.h"

#include "LpMatrix.h"

namespace zen
{
    class ImuComponent : public SensorComponent
    {
    public:
        ImuComponent(uint8_t id, unsigned int version, std::unique_ptr<IZenSensorProperties> properties, AsyncIoInterface& ioInterface);

        /** Tries to initialize settings of the sensor's component that can fail.
         * After succesfully completing init, m_properties should be set.
         */
        ZenSensorInitError init() override;

        ZenError processData(uint8_t function, const unsigned char* data, size_t length) override;

        const char* type() const override { return g_zenSensorType_Imu; }

    private:
        ZenError processSensorData(const unsigned char* data, size_t length);

        struct IMUState
        {
            LpMatrix3x3f accAlignMatrix;
            LpMatrix3x3f gyrAlignMatrix;
            LpMatrix3x3f softIronMatrix;
            LpVector3f accBias;
            LpVector3f gyrBias;
            LpVector3f hardIronOffset;
            int32_t samplingRate;
        };
        Owner<IMUState> m_cache;

        AsyncIoInterface& m_ioInterface;
        
        const unsigned int m_version;
        const uint8_t m_id;
    };
}
#endif
