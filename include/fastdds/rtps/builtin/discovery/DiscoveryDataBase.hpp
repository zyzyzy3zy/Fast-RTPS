// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file DiscoveryDataBase.hpp
 *
 */

#ifndef _FASTDDS_RTPS_DISCOVERY_DATABASE_H_
#define _FASTDDS_RTPS_DISCOVERY_DATABASE_H_

#include <fastdds/rtps/builtin/discovery/DiscoveryDataFilter.hpp>
#include <fastdds/rtps/common/Guid.h>
#include <fastdds/rtps/builtin/data/ReaderProxyData.h>
#include <fastdds/rtps/builtin/data/WriterProxyData.h>
#include <fastrtps/utils/collections/ResourceLimitedVector.hpp>

using namespace eprosima::fastrtps::rtps;
using namespace eprosima::fastrtps;

namespace eprosima {
namespace fastdds {
namespace rtps {

class ParticipantProxyData;
class ReaderProxyData;
class WriterProxyData;

typedef std::vector<fastrtps::string_255> TopicNameSeq;
typedef std::vector<fastrtps::rtps::GuidPrefix_t> GuidPrefixSeq;

/**
 * Class to manage the discovery data base
 *@ingroup DISCOVERY_MODULE
 */
class DiscoveryDataBase
    : public PDPDataFilter<DiscoveryDataBase>
    , public EDPDataFilter<DiscoveryDataBase>
    , public EDPDataFilter<DiscoveryDataBase, false>
{
public:
    DiscoveryDataBase();

    ~DiscoveryDataBase();

    bool pdp_is_relevant(
            const fastrtps::rtps::CacheChange_t& change,
            const fastrtps::rtps::GUID_t& reader_guid) const;

    bool edp_publications_is_relevant(
            const fastrtps::rtps::CacheChange_t& change,
            const fastrtps::rtps::GUID_t& reader_guid) const;

    bool edp_subscriptions_is_relevant(
            const fastrtps::rtps::CacheChange_t& change,
            const fastrtps::rtps::GUID_t& reader_guid) const;

    /**
     * @brief Set a new relevant remote participant guid in the relevance list of another participant.
     * @param participant_guid_prefix GuidPrefix of the participant for which to add the new relevant Participant.
     * @param relevant_participant GuidPrefix of the participant to be added in the relevance list of other Participant.
     */
    void add_relevant_participant_proxy(
            const fastrtps::rtps::GuidPrefix_t& participant_guid_prefix,
            const fastrtps::rtps::GuidPrefix_t& relevant_participant);

    /**
     * @brief Set a new relevant publication topic name in the relevance list of another participant.
     * @param participant_guid_prefix GuidPrefix of the participant for which to add the new relevant topic name.
     * @param publication_topic Name of the publication topic to be added in the relevance list of the Participant.
     */
    void add_relevant_publication_topic(
            const fastrtps::rtps::GuidPrefix_t& participant_guid_prefix,
            const fastrtps::string_255& publication_topic);

    /**
     * @brief Set a new relevant subscription topic name in the relevance list of another participant.
     * @param participant_guid_prefix GuidPrefix of the participant for which to add the new relevant topic name.
     * @param subscription_topic Name of the subscription topic to be added in the relevance list of the Participant.
     */
    void add_relevant_subscription_topic(
            const fastrtps::rtps::GuidPrefix_t& participant_guid_prefix,
            const fastrtps::string_255& subscription_topic);

    /**
     * @brief Set a new relevant Participant object in the relevance list of a change.
     * @param ch_sample_identity SampleIdentity of the change for which to add the new relevant Participant object.
     * @param relevant_participant ParticipantProxyData pointer to be added in the relevance list a change.
     */
    void add_relevant_participant_proxy_to_change(
            const fastrtps::rtps::SampleIdentity& ch_sample_identity,
            ParticipantProxyData* relevant_participant);

    /**
     * @brief Set a new relevant Writer object in the relevance list of a change.
     * @param ch_sample_identity SampleIdentity of the change for which to add the new relevant Writer object.
     * @param relevant_writer WriterProxyData pointer to be added in the relevance list a change.
     */
    void add_relevant_writer_proxy_to_change(
            const fastrtps::rtps::SampleIdentity& ch_sample_identity,
            WriterProxyData* relevant_writer);

    /**
     * @brief Set a new relevant Reader object in the relevance list of a change.
     * @param ch_sample_identity SampleIdentity of the change for which to add the new relevant Reader object.
     * @param relevant_reader ReaderProxyData pointer to be added in the relevance list a change.
     */
    void add_relevant_reader_proxy_to_change(
            const fastrtps::rtps::SampleIdentity& ch_sample_identity,
            ReaderProxyData* relevant_reader);

    /**
     * @brief Set a new entry in the list of participants that have already acked an specific change.
     * @param ch_sample_identity SampleIdentity of the change for which to add the participant.
     * @param relevant_reader ParticipantProxyData pointer to be added in the relevance list a change.
     */
    void add_acked_participant_proxy_to_change(
            const fastrtps::rtps::SampleIdentity& ch_sample_identity,
            ParticipantProxyData* relevant_participant);

private:
    //! Map with the relevant participants for a specific participant
    std::map<GuidPrefix_t, GuidPrefixSeq> relevant_participant_proxies_;
    //! Map with the relevant publication topics for a specific participant
    std::map<GuidPrefix_t, TopicNameSeq> relevant_publication_topics_;
    //! Map with the relevant subscription topics for a specific participant
    std::map<GuidPrefix_t, TopicNameSeq> relevant_subscription_topics_;
    //! Map that binds a CacheChange with all relevant Participants for that CacheChange
    std::map<SampleIdentity, ResourceLimitedVector<ParticipantProxyData*>> cachechange_relevant_participant_proxies_;
    //! Map that binds a CacheChange with all relevant Writers for that CacheChange
    std::map<SampleIdentity, ResourceLimitedVector<WriterProxyData*>> cachechange_relevant_writer_proxies_;
    //! Map that binds a CacheChange with all relevant Readers for that CacheChange
    std::map<SampleIdentity, ResourceLimitedVector<ReaderProxyData*>> cachechange_relevant_reader_proxies_;
    //! Map that binds a CacheChange with the Participants for which the data(p/w/r) has been acked
    std::map<SampleIdentity, ResourceLimitedVector<ParticipantProxyData*>> cachechange_acked_participant_proxies_;
};

} /* namespace rtps */
} /* namespace fastdds */
} /* namespace eprosima */

#endif /* _FASTDDS_RTPS_DISCOVERY_DATABASE_H_ */