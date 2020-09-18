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
 * @file DiscoveryDataBase.cpp
 *
 */

#include <fastdds/rtps/builtin/discovery/DiscoveryDataBase.hpp>

namespace eprosima {
namespace fastdds {
namespace rtps {

DiscoveryDataBase::DiscoveryDataBase()
{
}

DiscoveryDataBase::~DiscoveryDatabase()
{
    if (!cachechange_relevant_participant_proxies_.empty())
    {
        for(std::map<
                SampleIdentity,
                ResourceLimitedVector<ParticipantProxyData*>
                >::iterator itr = cachechange_relevant_participant_proxies_.begin();
            itr != cachechange_relevant_participant_proxies_.end();
            itr++)
        {
            itr->second.clear();
        }
    }

    if (!cachechange_relevant_writer_proxies_.empty())
    {
        for(std::map<
                SampleIdentity,
                ResourceLimitedVector<ParticipantProxyData*>
                >::iterator itr = cachechange_relevant_writer_proxies_.begin();
            itr != cachechange_relevant_writer_proxies_.end();
            itr++)
        {
            itr->second.clear();
        }
    }

    if (!cachechange_relevant_reader_proxies_.empty())
    {
        for(std::map<
                SampleIdentity,
                ResourceLimitedVector<ParticipantProxyData*>
                >::iterator itr = cachechange_relevant_reader_proxies_.begin();
            itr != cachechange_relevant_reader_proxies_.end();
            itr++)
        {
            itr->second.clear();
        }
    }

    if (!cachechange_acked_participant_proxies_.empty())
    {
        for(std::map<
                SampleIdentity,
                ResourceLimitedVector<ParticipantProxyData*>
                >::iterator itr = cachechange_acked_participant_proxies_.begin();
            itr != cachechange_acked_participant_proxies_.end();
            itr++)
        {
            itr->second.clear();
        }
    }
}

bool DiscoveryDataBase::pdp_is_relevant(
        const fastrtps::rtps::CacheChange_t& change,
        const fastrtps::rtps::GUID_t& reader_guid) const
{
    (void)change;
    (void)reader_guid;
    return true;
}

bool DiscoveryDataBase::edp_publications_is_relevant(
        const fastrtps::rtps::CacheChange_t& change,
        const fastrtps::rtps::GUID_t& reader_guid) const
{
    (void)change;
    (void)reader_guid;
    return true;
}

bool DiscoveryDataBase::edp_subscriptions_is_relevant(
        const fastrtps::rtps::CacheChange_t& change,
        const fastrtps::rtps::GUID_t& reader_guid) const
{
    (void)change;
    (void)reader_guid;
    return true;
}

void DiscoveryDataBase::add_relevant_participant_proxy(
        const fastrtps::rtps::GuidPrefix_t& participant_guid_prefix,
        const fastrtps::rtps::GuidPrefix_t& relevant_participant)
{
    bool exists = false;
    auto it = relevant_participant_proxies_.find(participant_guid_prefix);
    if (it != relevant_participant_proxies_.end())
    {
        for (GuidPrefix_t& pit : it->second)
        {
            if (pit == relevant_participant)
            {
                exists = true;
            }
        }
    }

    if(!exists)
    {
        relevant_participant_proxies_[participant_guid_prefix].push_back(relevant_participant);
    }
}

void DiscoveryDataBase::add_relevant_publication_topic(
        const fastrtps::rtps::GuidPrefix_t& participant_guid_prefix,
        const fastrtps::string_255& publication_topic)
{
    bool exists = false;
    auto it = relevant_publication_topics_.find(participant_guid_prefix);
    if (it != relevant_publication_topics_.end())
    {
        for (fastrtps::string_255& pub_it : it->second)
        {
            if (pub_it == publication_topic)
            {
                exists = true;
            }
        }
    }

    if(!exists)
    {
        relevant_publication_topics_[participant_guid_prefix].push_back(publication_topic);
    }
}

void DiscoveryDataBase::add_relevant_subscription_topic(
        const fastrtps::rtps::GuidPrefix_t& participant_guid_prefix,
        const fastrtps::string_255& subscription_topic)
{
    bool exists = false;
    auto it = relevant_subscription_topics_.find(ch_sample_identity);
    if (it != relevant_subscription_topics_.end())
    {
        for (fastrtps::string_255& sub_it : it->second)
        {
            if (sub_it == subscription_topic)
            {
                exists = true;
            }
        }
    }

    if(!exists)
    {
        relevant_subscription_topics_[participant_guid_prefix].push_back(subscription_topic);
    }
}

void DiscoveryDataBase::add_relevant_participant_proxy_to_change(
        const fastrtps::rtps::SampleIdentity& ch_sample_identity,
        ParticipantProxyData* relevant_participant)
{
    bool exists = false;
    auto it = cachechange_relevant_participant_proxies_.find(ch_sample_identity);
    if (it != cachechange_relevant_participant_proxies_.end())
    {
        for (ParticipantProxyData* pit : it->second)
        {
            if (pit->m_guid.guidPrefix == relevant_participant->m_guid.guidPrefix)
            {
                exists = true;
            }
        }
    }

    if(!exists)
    {
        cachechange_relevant_participant_proxies_[ch_sample_identity].push_back(relevant_participant);
    }
}

void DiscoveryDataBase::add_relevant_writer_proxy_to_change(
        const fastrtps::rtps::SampleIdentity& ch_sample_identity,
        WriterProxyData* relevant_writer)
{
    bool exists = false;
    auto it = cachechange_relevant_writer_proxies_.find(ch_sample_identity);
    if (it != cachechange_relevant_writer_proxies_.end())
    {
        for (ParticipantProxyData* pit : it->second)
        {
            if (pit->m_guid == relevant_writer->m_guid)
            {
                exists = true;
            }
        }
    }

    if(!exists)
    {
        cachechange_relevant_writer_proxies_[ch_sample_identity].push_back(relevant_writer);
    }
}

void DiscoveryDataBase::add_relevant_reader_proxy_to_change(
        const fastrtps::rtps::SampleIdentity& ch_sample_identity,
        ReaderProxyData* relevant_reader)
{
    bool exists = false;
    auto it = cachechange_relevant_reader_proxies_.find(ch_sample_identity);
    if (it != cachechange_relevant_reader_proxies_.end())
    {
        for (ParticipantProxyData* pit : it->second)
        {
            if (pit->m_guid == relevant_reader->m_guid)
            {
                exists = true;
            }
        }
    }

    if(!exists)
    {
        cachechange_relevant_reader_proxies_[ch_sample_identity].push_back(relevant_reader);
    }
}

void DiscoveryDataBase::add_acked_participant_proxy_to_change(
        const fastrtps::rtps::SampleIdentity& ch_sample_identity,
        ParticipantProxyData* relevant_participant)
{
    bool exists = false;
    auto it = cachechange_acked_participant_proxies_.find(ch_sample_identity);
    if (it != cachechange_acked_participant_proxies_.end())
    {
        for (ParticipantProxyData* pit : it->second)
        {
            if (pit->m_guid.guidPrefix == relevant_participant->m_guid.guidPrefix)
            {
                exists = true;
            }
        }
    }

    if(!exists)
    {
        cachechange_acked_participant_proxies_[ch_sample_identity].push_back(relevant_participant);
    }
}


} /* namespace rtps */
} /* namespace fastdds */
} /* namespace eprosima */