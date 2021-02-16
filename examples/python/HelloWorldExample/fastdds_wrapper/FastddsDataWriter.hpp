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
 * @file FastddsDataWriter.hpp
 *
 */

#ifndef _FASTDDSDATAWRITER_HPP_
#define _FASTDDSDATAWRITER_HPP_

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <string>
#include <stdexcept>


template<class CustomTopicDataType>
class FastddsDataWriter
{

public:

    typedef typename CustomTopicDataType::type type;

    FastddsDataWriter(
            uint32_t did,
            const std::string& topic_name)
        : participant_(nullptr)
        , topic_(nullptr)
        , publisher_(nullptr)
        , datawriter_(nullptr)
        , topic_name_(topic_name)
        , type_(new CustomTopicDataType())
        , did_(did)
    {
    }

    ~FastddsDataWriter()
    {
        destroy();
    }

    void init()
    {
        participant_ = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(
                did_, eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT);

        if (participant_ == nullptr)
        {
            std::cerr << "Error creating participant" << std::endl;
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

        // Create publisher
        publisher_ = participant_->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT);
        if (publisher_ == nullptr)
        {
            std::cerr << "Error creating publisher" << std::endl;
            throw new std::runtime_error("Error creating publisher");
        }

        // Create datawriter
        datawriter_ = publisher_->create_datawriter(topic_, eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT);
        if (datawriter_ == nullptr)
        {
            std::cerr << "Error creating datawriter" << std::endl;
            throw new std::runtime_error("Error creating datawriter");
        }
    }

    void destroy()
    {
        if (participant_)
        {
            if (datawriter_)
            {
                publisher_->delete_datawriter(datawriter_);
                datawriter_ = nullptr;
            }
            if (publisher_)
            {
                participant_->delete_publisher(publisher_);
                publisher_ = nullptr;
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

    bool write_sample(
            type& msg)
    {
        return datawriter_->write((void*)&msg);
    }


private:

    FastddsDataWriter(
            const FastddsDataWriter&) = delete;
    FastddsDataWriter& operator =(
            const FastddsDataWriter&) = delete;

    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::Publisher* publisher_;
    eprosima::fastdds::dds::DataWriter* datawriter_;
    std::string topic_name_;
    eprosima::fastdds::dds::TypeSupport type_;
    uint32_t did_;
};

#endif // _FASTDDSDATAWRITER_HPP_
