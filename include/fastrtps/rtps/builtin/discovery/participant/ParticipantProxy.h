// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file ParticipantProxy.h
 */

#ifndef FASTRTPS_RTPS_BUILTIN_DISCOVERY_PARTICIPANT_PARTICIPANTPROXY_H_
#define FASTRTPS_RTPS_BUILTIN_DISCOVERY_PARTICIPANT_PARTICIPANTPROXY_H_
#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <fastrtps/rtps/builtin/data/ParticipantProxyData.h>

#include <vector>

namespace eprosima {
namespace fastrtps {
namespace rtps {

/**
    * Class WriterProxy that contains the state of each matched writer for a specific reader.
    * @ingroup READER_MODULE
    */
class ParticipantProxy
{
    friend class PDP;
    friend class EDP;

public:

    ParticipantProxy(const RTPSParticipantAllocationAttributes& allocation);

    ~ParticipantProxy();

    //! Clear the data (restore to default state).
    void clear();

    void assert_liveliness();

    const std::chrono::steady_clock::time_point& last_received_message_tm() const
    {
        return last_received_message_tm_;
    }

    const std::chrono::microseconds& lease_duration() const
    {
        std::lock_guard<std::recursive_mutex> lck(proxy_data_->ppd_mutex_);
        return proxy_data_->lease_duration_us_;
    }

    std::shared_ptr<ParticipantProxyData> get_ppd()
    {
        return proxy_data_;
    }

    void set_ppd(const std::shared_ptr<ParticipantProxyData> & p)
    {
        proxy_data_ = p;
    }

    std::recursive_mutex& get_ppd_mutex()
    {
        return proxy_data_->ppd_mutex_;
    }

    GUID_t get_guid() const
    {
        return proxy_data_->m_guid;
    }

    GuidPrefix_t get_guid_prefix() const
    {
        return get_guid().guidPrefix;
    }

    void set_ppd(std::shared_ptr<ParticipantProxyData> && p)
    {
        proxy_data_ = std::move(p);
    }

    TimedEvent* get_lease_duration_event()
    {
        return lease_duration_event_;
    }

    void set_lease_duration_event(TimedEvent* event)
    {
        assert(nullptr == lease_duration_event_);

        if(nullptr == lease_duration_event_)
        {
            lease_duration_event_ = event;
        }
    }

    // should we do callback on lease durtion event?
    bool should_check_lease_duration_;

private:

    //! hard reference to the global ParticipantProxyData
    std::shared_ptr<ParticipantProxyData> proxy_data_;

    //! hard reference to the global ReaderProxyDatas this participant is aware of
    ResourceLimitedVector<std::shared_ptr<ReaderProxyData>> readers_;

    //! hard reference to the global WriterProxyDatas this participant is aware of
    ResourceLimitedVector<std::shared_ptr<WriterProxyData>> writers_;

    //! hard reference to the global builtin ReaderProxyDatas this participant is aware of
    ResourceLimitedVector<std::shared_ptr<ReaderProxyData>> builtin_readers_;

    //! hard reference to the global builtin WriterProxyDatas this participant is aware of
    ResourceLimitedVector<std::shared_ptr<WriterProxyData>> builtin_writers_;

    // Lease duration event for the PDP callbacks
    TimedEvent* lease_duration_event_;
    
    //! Store the last timestamp it was received a RTPS message from the remote participant.
    std::chrono::steady_clock::time_point last_received_message_tm_;

};

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */

#endif
#endif /* FASTRTPS_RTPS_READER_WRITERPROXY_H_ */
