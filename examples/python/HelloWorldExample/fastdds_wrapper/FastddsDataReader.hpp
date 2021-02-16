// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file FastddsDataReader.hpp
 *
 */

#ifndef _FASTDDSDATAREADER_HPP_
#define _FASTDDSDATAREADER_HPP_

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <string>
#include <stdexcept>


template<class CustomTopicDataType>
class FastddsDataReader
{

public:

    typedef typename CustomTopicDataType::type type;

    FastddsDataReader(
            uint32_t did,
            const std::string& topic_name)
        : participant_(nullptr)
        , topic_(nullptr)
        , subscriber_(nullptr)
        , datareader_(nullptr)
        , topic_name_(topic_name)
        , type_(new CustomTopicDataType())
        , did_(did)
    {
    }

    ~FastddsDataReader()
    {
        destroy();
    }

    bool wait_for_sample(uint32_t seconds)
    {
        eprosima::fastrtps::Duration_t timeout(seconds, 0);
        return datareader_->wait_for_unread_message(timeout);
    }

    void init()
    {
        participant_ = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(
                did_,
                eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT);

        if (participant_ == nullptr)
        {
            throw new std::runtime_error("Error creating participant");
        }

        // Register type
        type_.register_type(participant_);

        // Create topic
        topic_ = participant_->create_topic(topic_name_, type_->getName(),
                        eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
        {
            std::stringstream error;
            error << "Error creating topic [" << topic_name_ << "]";
            throw new std::runtime_error(error.str());
        }

        // Create subscriber
        subscriber_ = participant_->create_subscriber(eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT);
        if (subscriber_ == nullptr)
        {
            throw new std::runtime_error("Error creating subscriber");
        }

        // Create datareader
        datareader_ = subscriber_->create_datareader(topic_, eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT);
        if (datareader_ == nullptr)
        {
            throw new std::runtime_error("Error creating datareader");
        }
    }

    void destroy()
    {
        if (participant_)
        {
            if (datareader_)
            {
                subscriber_->delete_datareader(datareader_);
                datareader_ = nullptr;
            }
            if (subscriber_)
            {
                participant_->delete_subscriber(subscriber_);
                subscriber_ = nullptr;
            }
            if (topic_)
            {
                participant_->delete_topic(topic_);
                topic_ = nullptr;
            }
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(participant_);
            participant_ = nullptr;
        }
    }

    bool take_sample(
            type& msg)
    {
        eprosima::fastdds::dds::SampleInfo info;
        if (datareader_->take_next_sample(&msg, &info) == ReturnCode_t::RETCODE_OK)
        {
            if (info.instance_state == eprosima::fastdds::dds::ALIVE_INSTANCE_STATE)
            {
                return true;
            }
        }

        return false;
    }


private:

    FastddsDataReader(
            const FastddsDataReader&) = delete;
    FastddsDataReader& operator =(
            const FastddsDataReader&);

    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::Subscriber* subscriber_;
    eprosima::fastdds::dds::DataReader* datareader_;
    std::string topic_name_;
    eprosima::fastdds::dds::TypeSupport type_;
    uint32_t did_;
};

#endif // _FASTDDSDATAREADER_HPP_
