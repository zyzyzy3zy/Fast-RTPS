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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#define TEST_FRIENDS \
    FRIEND_TEST(WriterProxyTests, MissingChangesUpdate); \
    FRIEND_TEST(WriterProxyTests, LostChangesUpdate); \
    FRIEND_TEST(WriterProxyTests, ReceivedChangeSet); \
    FRIEND_TEST(WriterProxyTests, IrrelevantChangeSet);

#include "WriterProxy.h"
#include <rtps/participant/RTPSParticipantImpl.h>
#include <fastrtps/rtps/reader/StatefulReader.h>
#include <fastrtps/rtps/builtin/data/WriterProxyData.h>
#include <fastrtps/rtps/resources/TimedEvent.h>

// Make SequenceNumberSet_t compatible with GMock macros

namespace testing
{
namespace internal
{
using namespace eprosima::fastrtps::rtps;

template<>
bool AnyEq::operator()( const SequenceNumberSet_t & a, const SequenceNumberSet_t & b ) const
{
    // remember that using SequenceNumberSet_t = BitmapRange<SequenceNumber_t, SequenceNumberDiff, 256>;
    // see test\unittest\utils\BitmapRangeTests.cpp method TestResult::Check

    uint32_t num_bits[2];
    uint32_t num_longs[2];
    SequenceNumberSet_t::bitmap_type bitmap[2];

    a.bitmap_get( num_bits[0], bitmap[0], num_longs[0] );
    b.bitmap_get( num_bits[1], bitmap[1], num_longs[1] );

    if (num_bits[0] != num_bits[1] || num_longs[0] != num_longs[1])
    {
        return false;
    }
    return std::equal( bitmap[0].cbegin(), bitmap[0].cbegin() + num_longs[0], bitmap[1].cbegin() );
}
}
}

namespace eprosima {
namespace fastrtps {
namespace rtps {

TEST(WriterProxyTests, MissingChangesUpdate)
{
    using ::testing::ReturnRef;
    using ::testing::Eq;
    using ::testing::Ref;
    using ::testing::Const;

    WriterProxyData wattr( 4u, 1u );
    StatefulReader readerMock; // avoid annoying uninteresting call warnings

    // Testing the Timed events are properly configured
    WriterProxy wproxy(&readerMock, RemoteLocatorsAllocationAttributes(), ResourceLimitedContainerConfig());
    EXPECT_CALL(*wproxy.initial_acknack_, update_interval(readerMock.getTimes().initialAcknackDelay)).Times(1u);
    EXPECT_CALL(*wproxy.heartbeat_response_, update_interval(readerMock.getTimes().heartbeatResponseDelay)).Times(1u);
    EXPECT_CALL(*wproxy.initial_acknack_, restart_timer()).Times(1u);
    wproxy.start(wattr);

    // 1. Simulate initial acknack
    SequenceNumberSet_t t1( SequenceNumber_t( 0, 0 ) );
    EXPECT_CALL( readerMock, simp_send_acknack(t1)).Times( 1u );
    wproxy.perform_initial_ack_nack();

    // 2. Simulate Writer initial HEARTBEAT if there is a single sample in writer's history
    // According to RTPS Wire spec 8.3.7.5.3 firstSN.value and lastSN.value cannot be 0 or negative
    // and lastSN.value < firstSN.value
    bool assert_liveliness = false;
    uint32_t heartbeat_count = 1;
    EXPECT_CALL( *wproxy.heartbeat_response_, restart_timer() ).Times( 1u );
    wproxy.process_heartbeat( heartbeat_count++, SequenceNumber_t( 0, 1 ), SequenceNumber_t( 0, 1 ), false, false, false, assert_liveliness );

    SequenceNumberSet_t t2( SequenceNumber_t( 0, 1 ) );
    t2.add( SequenceNumber_t( 0, 1 ) );
    ASSERT_THAT( t2, wproxy.missing_changes() );
    ASSERT_EQ( SequenceNumber_t(), wproxy.available_changes_max() );
    
    // 3. Simulate reception of a HEARTBEAT after two more samples are added to the writer's history 
    EXPECT_CALL( *wproxy.heartbeat_response_, restart_timer() ).Times( 1u );
    wproxy.process_heartbeat( heartbeat_count++, SequenceNumber_t( 0, 1 ), SequenceNumber_t( 0, 3 ), false, false, false, assert_liveliness);
    
    t2.add( SequenceNumber_t( 0, 2 ) );
    t2.add( SequenceNumber_t( 0, 3 ) );
    ASSERT_THAT( t2, wproxy.missing_changes() );
    ASSERT_EQ( SequenceNumber_t(), wproxy.available_changes_max() );

    // 4. Simulate reception of a DATA(6).
    wproxy.received_change_set( SequenceNumber_t( 0, 6 ) );

    ASSERT_THAT( t2, wproxy.missing_changes() );
    ASSERT_EQ( SequenceNumber_t(), wproxy.available_changes_max() );

    // 5. Simulate reception of a HEARTBEAT(1,6)
    EXPECT_CALL( *wproxy.heartbeat_response_, restart_timer() ).Times( 1u );
    wproxy.process_heartbeat( heartbeat_count++, SequenceNumber_t( 0, 1 ), SequenceNumber_t( 0, 6 ), false, false, false, assert_liveliness );

    t2.add( SequenceNumber_t( 0, 4 ) );
    t2.add( SequenceNumber_t( 0, 5 ) );
    ASSERT_THAT( t2, wproxy.missing_changes() );
    ASSERT_EQ( SequenceNumber_t(), wproxy.available_changes_max() );

    // 6. Simulate reception of all missing DATA
    wproxy.received_change_set(SequenceNumber_t(0, 1));
    wproxy.received_change_set(SequenceNumber_t(0, 2));
    wproxy.received_change_set(SequenceNumber_t(0, 3));
    wproxy.received_change_set(SequenceNumber_t(0, 4));
    wproxy.received_change_set(SequenceNumber_t(0, 5));

    SequenceNumberSet_t t6( SequenceNumber_t( 0, 7 ) );
    ASSERT_THAT( t6, wproxy.missing_changes() );
    ASSERT_EQ( SequenceNumber_t( 0, 6 ), wproxy.available_changes_max() );

    // 7. Simulate reception of a faulty HEARTBEAT with a lower last sequence number (4)
    EXPECT_CALL( *wproxy.heartbeat_response_, restart_timer() ).Times( 1u );
    wproxy.process_heartbeat( heartbeat_count++, SequenceNumber_t( 0, 1 ), SequenceNumber_t( 0, 4 ), false, false, false, assert_liveliness );

    ASSERT_THAT( t6, wproxy.missing_changes() );
    ASSERT_EQ( SequenceNumber_t( 0, 6 ), wproxy.available_changes_max() );

    // 8. Simulate reception of DATA(8) and DATA(10)
    wproxy.received_change_set(SequenceNumber_t(0,8));
    wproxy.received_change_set( SequenceNumber_t(0,10));

    ASSERT_THAT( t6, wproxy.missing_changes() );
    ASSERT_EQ( SequenceNumber_t( 0, 6 ), wproxy.available_changes_max() );

    // 9. Simulate reception of DATA(10)
    EXPECT_CALL( *wproxy.heartbeat_response_, restart_timer() ).Times( 1u );
    wproxy.process_heartbeat( heartbeat_count++, SequenceNumber_t( 0, 1 ), SequenceNumber_t( 0, 10 ), false, false, false, assert_liveliness );

    t6.add( SequenceNumber_t( 0, 6 ) );
    t6.add( SequenceNumber_t( 0, 7 ) );
    t6.add( SequenceNumber_t( 0, 9 ) );
    ASSERT_THAT( t6, wproxy.missing_changes() );
    ASSERT_EQ( SequenceNumber_t( 0, 6 ), wproxy.available_changes_max() );
}

TEST(WriterProxyTests, LostChangesUpdate)
{
    WriterProxyData wattr(4u, 1u);
    StatefulReader readerMock;
    WriterProxy wproxy(&readerMock, RemoteLocatorsAllocationAttributes(), ResourceLimitedContainerConfig());
    EXPECT_CALL(*wproxy.initial_acknack_, update_interval(readerMock.getTimes().initialAcknackDelay)).Times(1u);
    EXPECT_CALL(*wproxy.heartbeat_response_, update_interval(readerMock.getTimes().heartbeatResponseDelay)).Times(1u);
    EXPECT_CALL(*wproxy.initial_acknack_, restart_timer()).Times(1u);
    wproxy.start(wattr);

    // Update LOST changes util sequence number 3.
    wproxy.lost_changes_update(SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 2));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 0u);

    // Add two UNKNOWN with sequence numberes 3 and 4.
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0,3)));
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0,4)));

    // Update LOST changes util sequence number 5.
    wproxy.lost_changes_update(SequenceNumber_t(0, 5));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 4));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 0u);

    // Try to update LOST changes util sequence number 4.
    wproxy.lost_changes_update(SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 4));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 0u);

    // Add two UNKNOWN changes with sequence number 5 and 8.
    // Add one MISSING change with sequence number 6.
    // Add one RECEIVED change with sequence number 7.
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0, 5)));
    ChangeFromWriter_t missing_aux_change_from_w(SequenceNumber_t(0, 6));
    missing_aux_change_from_w.setStatus(ChangeFromWriterStatus_t::MISSING);
    wproxy.changes_from_writer_.insert(missing_aux_change_from_w);
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0, 7)));
    wproxy.received_change_set(SequenceNumber_t(0, 7));
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0, 8)));

    // Update LOST changes util sequence number 8.
    wproxy.lost_changes_update(SequenceNumber_t(0, 8));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 7));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 1u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);

    // Update LOST changes util sequence number 10.
    wproxy.lost_changes_update(SequenceNumber_t(0, 10));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 9));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 0u);
}

TEST(WriterProxyTests, ReceivedChangeSet)
{
    WriterProxyData wattr(4u, 1u);
    StatefulReader readerMock;
    WriterProxy wproxy(&readerMock, RemoteLocatorsAllocationAttributes(), ResourceLimitedContainerConfig());
    EXPECT_CALL(*wproxy.initial_acknack_, update_interval(readerMock.getTimes().initialAcknackDelay)).Times(1u);
    EXPECT_CALL(*wproxy.heartbeat_response_, update_interval(readerMock.getTimes().heartbeatResponseDelay)).Times(1u);
    EXPECT_CALL(*wproxy.initial_acknack_, restart_timer()).Times(1u);
    wproxy.start(wattr);

    // Set received change with sequence number 3.
    wproxy.received_change_set(SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 0));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 3u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 1))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 2))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 3))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);

    // Add two UNKNOWN with sequence numberes 4 and 5.
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0,4)));
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0,5)));

    // Set received change with sequence number 2
    wproxy.received_change_set(SequenceNumber_t(0, 2));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 0));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 5u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 1))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 2))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 3))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);

    // Set received change with sequence number 1
    wproxy.received_change_set(SequenceNumber_t(0, 1));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 2u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);

    // Try to update LOST changes util sequence number 3.
    wproxy.received_change_set(SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 2u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);

    // Add received change with sequence number 6
    wproxy.received_change_set(SequenceNumber_t(0, 6));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 3u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);

    // Add received change with sequence number 8
    wproxy.received_change_set(SequenceNumber_t(0, 8));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 5u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 7))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);

    // Add received change with sequence number 4
    wproxy.received_change_set(SequenceNumber_t(0, 4));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 4));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 4u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 7))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);

    // Add received change with sequence number 5
    wproxy.received_change_set(SequenceNumber_t(0, 5));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 6));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 2u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 7))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);

    // Add received change with sequence number 7
    wproxy.received_change_set(SequenceNumber_t(0, 7));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 8));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 0u);
}

TEST(WriterProxyTests, IrrelevantChangeSet)
{
    WriterProxyData wattr(4u, 1u);
    StatefulReader readerMock;
    WriterProxy wproxy(&readerMock, RemoteLocatorsAllocationAttributes(), ResourceLimitedContainerConfig());
    EXPECT_CALL(*wproxy.initial_acknack_, update_interval(readerMock.getTimes().initialAcknackDelay)).Times(1u);
    EXPECT_CALL(*wproxy.heartbeat_response_, update_interval(readerMock.getTimes().heartbeatResponseDelay)).Times(1u);
    EXPECT_CALL(*wproxy.initial_acknack_, restart_timer()).Times(1u);
    wproxy.start(wattr);

    // Set irrelevant change with sequence number 3.
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 0));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 3u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 1))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 2))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 3))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 3))->isRelevant(), false);

    // Add two UNKNOWN with sequence numberes 4 and 5.
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0,4)));
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0,5)));

    // Set irrelevant change with sequence number 2
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 2));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 0));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 5u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 1))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 2))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 2))->isRelevant(), false);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 3))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 3))->isRelevant(), false);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);

    // Set irrelevant change with sequence number 1
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 1));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 2u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);

    // Try to update LOST changes util sequence number 3.
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 2u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);

    // Add irrelevant change with sequence number 6
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 6));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 3u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->isRelevant(), false);

    // Add irrelevant change with sequence number 8
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 8));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 5u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->isRelevant(), false);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 7))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->isRelevant(), false);

    // Add irrelevant change with sequence number 4
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 4));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 4));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 4u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->isRelevant(), false);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 7))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->isRelevant(), false);

    // Add irrelevant change with sequence number 5
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 5));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 6));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 2u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 7))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->isRelevant(), false);

    // Add irrelevant change with sequence number 7
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 7));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 8));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 0u);
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

