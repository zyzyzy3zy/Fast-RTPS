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

#include "BlackboxTests.hpp"

#include "PubSubReader.hpp"
#include "PubSubWriter.hpp"

#include <fastrtps/transport/test_UDPv4Transport.h>
#include <fastrtps/xmlparser/XMLProfileManager.h>
#include <gtest/gtest.h>

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

class PubSubHistory : public testing::TestWithParam<bool>
{
public:

    void SetUp() override
    {
        LibrarySettingsAttributes library_settings;
        if (GetParam())
        {
            library_settings.intraprocess_delivery = IntraprocessDeliveryType::INTRAPROCESS_FULL;
            xmlparser::XMLProfileManager::library_settings(library_settings);
        }
    }

    void TearDown() override
    {
        LibrarySettingsAttributes library_settings;
        if (GetParam())
        {
            library_settings.intraprocess_delivery = IntraprocessDeliveryType::INTRAPROCESS_OFF;
            xmlparser::XMLProfileManager::library_settings(library_settings);
        }
    }

};

// Test created to check bug #1568 (Github #34)
TEST_P(PubSubHistory, PubSubAsNonReliableKeepLastReaderSmallDepth)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    reader.history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).
    history_depth(2).
    resource_limits_allocated_samples(2).
    resource_limits_max_samples(2).init();

    ASSERT_TRUE(reader.isInitialized());

    writer.reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator();

    while (data.size() > 1)
    {
        auto expected_data(data);

        // Send data
        writer.send(data);
        // In this test all data should be sent.
        ASSERT_TRUE(data.empty());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        reader.startReception(expected_data);
        // Block reader until reception finished or timeout.
        size_t current_received = reader.block_for_at_least(2);
        reader.stopReception();
        // Should be received only two samples.
        ASSERT_EQ(current_received, 2u);
        data = reader.data_not_received();
    }
}

//Test created to deal with Issue 39 on Github
TEST_P(PubSubHistory, CacheChangeReleaseTest)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    //Reader Config
    reader.reliability(eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS);
    reader.history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS);
    reader.history_depth(1);
    reader.resource_limits_allocated_samples(1);
    reader.resource_limits_max_samples(1);
    reader.init();
    ASSERT_TRUE(reader.isInitialized());

    writer.resource_limits_allocated_samples(1);
    writer.resource_limits_max_samples(1);
    writer.history_kind(KEEP_LAST_HISTORY_QOS);
    writer.history_depth(1);
    writer.reliability(BEST_EFFORT_RELIABILITY_QOS);
    writer.init();
    ASSERT_TRUE(writer.isInitialized());


    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator();

    reader.startReception(data);

    writer.send(data);
    ASSERT_TRUE(data.empty());
    size_t current_received = reader.block_for_all(std::chrono::seconds(3));

    ASSERT_GE(current_received, static_cast<size_t>(1));
}

// Test created to check bug #1555 (Github #31)
TEST_P(PubSubHistory, PubSubAsReliableKeepLastReaderSmallDepth)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    reader.reliability(RELIABLE_RELIABILITY_QOS).
    history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).
    history_depth(2).
    resource_limits_allocated_samples(2).
    resource_limits_max_samples(2).init();

    ASSERT_TRUE(reader.isInitialized());

    writer.init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator();

    while (data.size() > 1)
    {
        auto data_backup(data);
        decltype(data) expected_data;
        expected_data.push_back(data_backup.back()); data_backup.pop_back();
        expected_data.push_back(data_backup.back()); data_backup.pop_back();

        // Send data
        writer.send(data);
        // In this test all data should be sent.
        ASSERT_TRUE(data.empty());
        writer.waitForAllAcked(std::chrono::seconds(300));
        // Should be received only two samples.
        reader.startReception(expected_data);
        // Block reader until reception finished or timeout.
        reader.block_for_all();
        reader.stopReception();

        data = data_backup;
    }
}

// Test created to check bug #1738 (Github #54)
TEST_P(PubSubHistory, PubSubAsReliableKeepLastWriterSmallDepth)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    reader.reliability(RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    writer.
    history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).
    history_depth(2).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator();

    reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

// Test created to check bug #1558 (Github #33)
TEST(PubSubHistory, PubSubKeepAll)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    reader.reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
    history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
    resource_limits_allocated_samples(2).
    resource_limits_max_samples(2).init();

    ASSERT_TRUE(reader.isInitialized());

    writer.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
    max_blocking_time({0, 0}).
    resource_limits_allocated_samples(2).
    resource_limits_max_samples(2).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator();

    while (!data.empty())
    {
        auto expected_data(data);

        // Send data
        writer.send(data);

        for (auto& value : data)
        {
            expected_data.remove(value);
        }

        // In this test the history has 20 max_samples.
        ASSERT_LE(expected_data.size(), 2u);
        writer.waitForAllAcked(std::chrono::seconds(300));
        reader.startReception(expected_data);
        // Block reader until reception finished or timeout.
        reader.block_for_all();
        reader.stopReception();
    }
}

// Test created to check bug #1558 (Github #33)
TEST(PubSubHistory, PubSubKeepAllTransient)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    reader.reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
    history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
    resource_limits_allocated_samples(2).
    resource_limits_max_samples(2).init();

    ASSERT_TRUE(reader.isInitialized());

    writer.history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
    durability_kind(eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS).
    max_blocking_time({0, 0}).
    resource_limits_allocated_samples(2).
    resource_limits_max_samples(2).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator();

    while (!data.empty())
    {
        auto expected_data(data);

        // Send data
        writer.send(data);

        for (auto& value : data)
        {
            expected_data.remove(value);
        }

        // In this test the history has 20 max_samples.
        ASSERT_LE(expected_data.size(), 2u);
        writer.waitForAllAcked(std::chrono::seconds(300));
        reader.startReception(expected_data);
        // Block reader until reception finished or timeout.
        reader.block_for_all();
        reader.stopReception();
    }
}

TEST_P(PubSubHistory, PubReliableKeepAllSubNonReliable)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    reader.init();

    ASSERT_TRUE(reader.isInitialized());

    writer.reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
    history_kind(eprosima::fastrtps::KEEP_ALL_HISTORY_QOS).
    resource_limits_allocated_samples(1).
    resource_limits_max_samples(1).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator();

    reader.startReception(data);
    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    reader.block_for_at_least(2);
}

//Verify that Cachechanges are removed from History when the a Writer unmatches
TEST_P(PubSubHistory, StatefulReaderCacheChangeRelease)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    reader.history_depth(2).
    reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();
    ASSERT_TRUE(reader.isInitialized());
    writer.history_depth(2).
    reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();
    ASSERT_TRUE(writer.isInitialized());

    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator(2);
    auto expected_data(data);

    writer.send(data);
    ASSERT_TRUE(data.empty());
    writer.waitForAllAcked(std::chrono::seconds(300));
    writer.destroy();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    reader.startReception(expected_data);

    ASSERT_EQ(reader.getReceivedCount(), 0u);
}

template<typename T>
void send_async_data(
        PubSubWriter<T>& writer,
        std::list<typename T::type> data_to_send)
{
    // Send data
    writer.send(data_to_send);
    // In this test all data should be sent.
    ASSERT_TRUE(data_to_send.empty());
}

TEST_P(PubSubHistory, PubSubAsReliableMultithreadKeepLast1)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    reader.history_depth(1).
    reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    writer.history_depth(1).init();

    ASSERT_TRUE(writer.isInitialized());

    // Because its volatile the durability
    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_helloworld_data_generator(300);

    reader.startReception(data);

    std::thread thr1(&send_async_data<HelloWorldType>, std::ref(writer),
            std::list<HelloWorld>(data.begin(), std::next(data.begin(), 100)));
    std::thread thr2(&send_async_data<HelloWorldType>, std::ref(writer),
            std::list<HelloWorld>(std::next(data.begin(), 100), std::next(data.begin(), 200)));
    std::thread thr3(&send_async_data<HelloWorldType>, std::ref(writer),
            std::list<HelloWorld>(std::next(data.begin(), 200), data.end()));

    thr1.join();
    thr2.join();
    thr3.join();

    // Block reader until reception finished or timeout.
    reader.block_for_at_least(105);
}

// Test created to check bug #6319 (Github #708)
TEST_P(PubSubHistory, PubSubAsReliableKeepLastReaderSmallDepthTwoPublishers)
{
    PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    PubSubWriter<HelloWorldType> writer2(TEST_TOPIC_NAME);

    reader
    .reliability(RELIABLE_RELIABILITY_QOS)
    .history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS)
    .history_depth(1)
    .resource_limits_allocated_samples(1)
    .resource_limits_max_samples(1);

    writer.max_blocking_time({ 120, 0 });
    writer2.max_blocking_time({ 120, 0 });

    reader.init();
    ASSERT_TRUE(reader.isInitialized());

    writer.init();
    ASSERT_TRUE(writer.isInitialized());

    writer2.init();
    ASSERT_TRUE(writer2.isInitialized());

    // Wait for discovery.
    writer.wait_discovery();
    writer2.wait_discovery();
    reader.wait_discovery();

    HelloWorld data;
    data.message("Hello world!");

    // First writer sends two samples (reader would only keep the last one)
    data.index(1u);
    ASSERT_TRUE(writer.send_sample(data));
    data.index(2u);
    ASSERT_TRUE(writer.send_sample(data));

    // Wait for reader to acknowledge samples
    while (!writer.waitForAllAcked(std::chrono::milliseconds(100)))
    {
    }

    // Second writer sends one sample (reader should discard previous one)
    data.index(3u);
    ASSERT_TRUE(writer2.send_sample(data));

    // Wait for reader to acknowledge sample
    while (!writer2.waitForAllAcked(std::chrono::milliseconds(100)))
    {
    }

    // Only last sample should be present
    HelloWorld received;
    ASSERT_TRUE(reader.takeNextData(&received));
    ASSERT_EQ(received.index(), 3u);
}

TEST(BlackBox, PubSubAsReliableKeepLastWithKey)
{
    PubSubReader<KeyedHelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<KeyedHelloWorldType> writer(TEST_TOPIC_NAME);

    uint32_t keys = 2;

    reader.resource_limits_max_instances(keys).
    reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
    history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    writer.resource_limits_max_instances(keys).
    reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    auto data = default_keyedhelloworld_data_generator();
    reader.startReception(data);

    // Send data
    writer.send(data);
    ASSERT_TRUE(data.empty());

    reader.block_for_all();
    reader.stopReception();
}

TEST(BlackBox, PubSubAsReliableKeepLastReaderSmallDepthWithKey)
{
    PubSubReader<KeyedHelloWorldType> reader(TEST_TOPIC_NAME);
    PubSubWriter<KeyedHelloWorldType> writer(TEST_TOPIC_NAME);

    uint32_t keys = 2;
    uint32_t depth = 2;

    reader.history_depth(depth).
    resource_limits_max_samples(keys * depth).
    resource_limits_allocated_samples(keys * depth).
    resource_limits_max_instances(keys).
    resource_limits_max_samples_per_instance(depth).
    reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).
    history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).init();

    ASSERT_TRUE(reader.isInitialized());

    writer.history_depth(depth).
    resource_limits_max_samples(keys * depth).
    resource_limits_allocated_samples(keys * depth).
    resource_limits_max_instances(keys).
    resource_limits_max_samples_per_instance(depth).
    reliability(eprosima::fastrtps::RELIABLE_RELIABILITY_QOS).init();

    ASSERT_TRUE(writer.isInitialized());

    // Wait for discovery.
    writer.wait_discovery();
    reader.wait_discovery();

    //We want the number of messages to be multiple of keys*depth
    auto data = default_keyedhelloworld_data_generator(3 * keys * depth);
    while (data.size() > 1)
    {
        auto expected_data(data);

        // Send data
        writer.send(data);
        ASSERT_TRUE(data.empty());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        reader.startReception(expected_data);
        size_t current_received = reader.block_for_at_least(keys * depth);
        reader.stopReception();
        ASSERT_EQ(current_received, keys * depth);

        data = reader.data_not_received();
    }
}

// Test created to check bug #8945
TEST_P(PubSubHistory, WriterWithoutReadersTransientLocal)
{
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    writer.
    history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).
    durability_kind(eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS).
    max_blocking_time({0, 0}).
    resource_limits_allocated_samples(2).
    resource_limits_max_samples(2).
    history_depth(2).init();

    ASSERT_TRUE(writer.isInitialized());

    {
        // Remove the reader
        PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
        reader.reliability(RELIABLE_RELIABILITY_QOS).init();
        ASSERT_TRUE(reader.isInitialized());
        // Wait for discovery.
        writer.wait_discovery();
        reader.wait_discovery();
    }

    auto data = default_helloworld_data_generator();

    //reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    //reader.block_for_at_least(2);
}

// Test created to check bug #8945
/*
#if HAVE_SQLITE3
TEST_P(PubSubHistory, WriterWithoutReadersTransient)
{
    // Configure Writer Properties
    PropertyPolicy writer_policy;
    writer_policy.properties().emplace_back("dds.persistence.plugin", "builtin.SQLITE3");
    writer_policy.properties().emplace_back("dds.persistence.sqlite3.filename", "persistence.db");
    writer_policy.properties().emplace_back("dds.persistence.guid", "77.72.69.74.65.72.5f.70.65.72.73.5f|67.75.69.64");

    // Create DataWriter and configure the durability and reliability QoS
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);
    writer.entity_property_policy(writer_policy);

    writer.
    history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).
    durability_kind(eprosima::fastdds::dds::TRANSIENT_DURABILITY_QOS).
    history_depth(2).init();

    ASSERT_TRUE(writer.isInitialized());

    {
        // Remove the reader
        PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
        reader.reliability(RELIABLE_RELIABILITY_QOS).init();
        ASSERT_TRUE(reader.isInitialized());
        // Wait for discovery.
        writer.wait_discovery();
        reader.wait_discovery();
    }

    auto data = default_helloworld_data_generator();

    //reader.startReception(data);

    // Send data
    writer.send(data);
    // In this test all data should be sent.
    ASSERT_TRUE(data.empty());
    // Block reader until reception finished or timeout.
    //reader.block_for_at_least(2);
}
#endif // HAVE_SQLITE3
*/

// Test created to check bug #8945
TEST(PubSubHistory, WriterWithoutReadersVolatile)
{
    auto testTransport = std::make_shared<test_UDPv4TransportDescriptor>();
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    writer.
    reliability(RELIABLE_RELIABILITY_QOS).
    disable_builtin_transport().
    add_user_transport_to_pparams(testTransport).
    history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).
    durability_kind(eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS).
    //durability_kind(eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS).
    history_depth(2).
    resource_limits_allocated_samples(0).
    resource_limits_max_samples(2).
    max_blocking_time(0).init();

    ASSERT_TRUE(writer.isInitialized());

    auto data = default_helloworld_data_generator();

    // Send data
    writer.send(data);
    // All data should be sent is there is no reader.
    ASSERT_TRUE(data.empty());

    {
        PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
        reader.reliability(RELIABLE_RELIABILITY_QOS).
        durability_kind(TRANSIENT_LOCAL_DURABILITY_QOS).
        history_depth(1).
        resource_limits_allocated_samples(0).
        resource_limits_max_samples(1).init();
        ASSERT_TRUE(reader.isInitialized());
        // Wait for discovery.
        writer.wait_discovery();
        reader.wait_discovery();

        // Regenerate data
        data = default_helloworld_data_generator();
        reader.startReception(data);
        // We expect to receive 9 and 10
        std::this_thread::sleep_for(std::chrono::seconds(2));
        reader.destroy();

        //test_UDPv4Transport::test_UDPv4Transport_ShutdownAllNetwork = true;
        writer.send(data);
        /*
        uint64_t size = data.size();
        for (uint i = 0; i < size; ++i)
        {
            ASSERT_TRUE(writer.send_sample(data.front()));
            data.pop_front();
        }
        */
        // All data should be sent is there is no reader.
        ASSERT_TRUE(data.empty());
    }

    // Regenerate data
    data = default_helloworld_data_generator();
    writer.send(data);
    ASSERT_TRUE(data.empty());


    /* TODO:
La forma fácil de probar el 8945 es usando keep_all, con un resource_limits, y max_blocking_time 0
Si la historia se llena cuando no debería, la llamada a write retornará false

- Esperas que llegue un reader
- Llenas el history
- Compruebas que el write se bloquearía (retorna false si da timeout el max_blocking_time)
- Apagas el reader
- Compruebas que el write no se bloquea
- Escribes tantas veces como hueco tenga el history (resource_limits.max_samples)
- Compruebas que el write no se bloquea

Si quieres puedes añadir que antes de que llegue el reader tienes que poder llenar el history entero varias veces sin bloquearte
El stop reception desactiva el listener, pero no el reader
*/

}

TEST(PubSubHistory, HEXAGON_TEST)
{
    // Configure Writer Properties
    PropertyPolicy writer_policy;
    writer_policy.properties().emplace_back("dds.persistence.plugin", "builtin.SQLITE3");
    writer_policy.properties().emplace_back("dds.persistence.sqlite3.filename", "persistence.db");
    writer_policy.properties().emplace_back("dds.persistence.guid", "77.72.69.74.65.72.5f.70.65.72.73.5f|67.75.69.64");
    PubSubWriter<HelloWorldType> writer(TEST_TOPIC_NAME);

    writer.
    entity_property_policy(writer_policy).
    reliability(RELIABLE_RELIABILITY_QOS).
    history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).
    //durability_kind(eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS).
    durability_kind(eprosima::fastdds::dds::TRANSIENT_DURABILITY_QOS).
    asynchronously(ASYNCHRONOUS_PUBLISH_MODE).
    heartbeat_period_seconds(2).
    heartbeat_period_nanosec(200*1000*1000).
    //durability_kind(eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS).
    //history_depth(1300).
    resource_limits_allocated_samples(1300).
    resource_limits_max_samples(1300).
    //max_blocking_time(0).
    init();

    ASSERT_TRUE(writer.isInitialized());

    auto data = default_helloworld_data_generator(1800);

    std::thread reader_thread([&]()
    {
        PropertyPolicy reader_policy;
        reader_policy.properties().emplace_back("dds.persistence.plugin", "builtin.SQLITE3");
        reader_policy.properties().emplace_back("dds.persistence.sqlite3.filename", "r1.db");
        reader_policy.properties().emplace_back("dds.persistence.guid", "77.72.69.74.65.72.5f.70.65.72.73.5f|67.75.69.65");

        PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
        reader.
        entity_property_policy(reader_policy).
        reliability(RELIABLE_RELIABILITY_QOS).
        history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).
        //durability_kind(eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS).
        durability_kind(eprosima::fastdds::dds::TRANSIENT_DURABILITY_QOS).
        resource_limits_allocated_samples(1300).
        resource_limits_max_samples(1300).
        init();
        ASSERT_TRUE(reader.isInitialized());
        auto rec_data(data);
        reader.startReception(rec_data);
        //reader.startReception(data);

        //writer.wait_discovery();
        reader.wait_discovery();

        std::cout << "--- READER READY ---" << std::endl;
        reader.block_for_at_least(1400);
        std::cout << "--- READER FINISHED ---" << std::endl;
    });

    std::thread reader_thread_2([&]()
    {
        PropertyPolicy reader_policy;
        reader_policy.properties().emplace_back("dds.persistence.plugin", "builtin.SQLITE3");
        reader_policy.properties().emplace_back("dds.persistence.sqlite3.filename", "r2.db");
        reader_policy.properties().emplace_back("dds.persistence.guid", "77.72.69.74.65.72.5f.70.65.72.73.5f|67.75.69.66");

        PubSubReader<HelloWorldType> reader(TEST_TOPIC_NAME);
        reader.
        entity_property_policy(reader_policy).
        reliability(RELIABLE_RELIABILITY_QOS).
        history_kind(eprosima::fastrtps::KEEP_LAST_HISTORY_QOS).
        //durability_kind(eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS).
        durability_kind(eprosima::fastdds::dds::TRANSIENT_DURABILITY_QOS).
        resource_limits_allocated_samples(1300).
        resource_limits_max_samples(1300).
        init();
        ASSERT_TRUE(reader.isInitialized());
        auto rec_data(data);
        reader.startReception(rec_data);
        //reader.startReception(data);

        //writer.wait_discovery();
        reader.wait_discovery();

        std::cout << "--- READER READY ---" << std::endl;
        reader.block_for_at_least(1400);
        std::cout << "--- READER FINISHED ---" << std::endl;
    });

    std::cout << "--- Waiting discovery ---" << std::endl;
    writer.wait_discovery(2);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    writer.send(data, 1);
    ASSERT_TRUE(data.empty());
    std::cout << "--- ALL DATA SENT ---" << std::endl;
    reader_thread.join();
    reader_thread_2.join();
    data = default_helloworld_data_generator(1300);
    std::cout << "--- SECOND DATA ---" << std::endl;
    writer.send(data, 1);
    std::cout << "--- SECOND DATA SENT ---" << std::endl;
    ASSERT_TRUE(data.empty());
}

INSTANTIATE_TEST_CASE_P(PubSubHistory,
        PubSubHistory,
        testing::Values(false, true),
        [](const testing::TestParamInfo<PubSubHistory::ParamType>& info)
        {
            if (info.param)
            {
                return "Intraprocess";
            }
            return "NonIntraprocess";
        });
