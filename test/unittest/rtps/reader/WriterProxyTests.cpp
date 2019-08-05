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

namespace eprosima {
namespace fastrtps {
namespace rtps {

TEST(WriterProxyTests, MissingChangesUpdate)
{
    using ::testing::ReturnRef;
    using ::testing::NiceMock;

    WriterProxyData wattr( 4u, 1u );
    NiceMock<StatefulReader> readerMock; // avoid annoying uninteresting call warnings

    // We must prevent any callbacks from WriterProxy TimedEvents to avoid interference with out EXPECT_CALLS
    ReaderTimes never_callback;
    never_callback.initialAcknackDelay = never_callback.heartbeatResponseDelay = c_TimeInfinite;
    ON_CALL( readerMock, getTimes ).WillByDefault( ReturnRef( never_callback ) );

    // Testing the Timed events are properly configured
    WriterProxy wproxy(&readerMock, RemoteLocatorsAllocationAttributes(), ResourceLimitedContainerConfig());
    EXPECT_CALL(*wproxy.initial_acknack_, update_interval(readerMock.getTimes().initialAcknackDelay)).Times(1u);
    EXPECT_CALL(*wproxy.heartbeat_response_, update_interval(readerMock.getTimes().heartbeatResponseDelay)).Times(1u);
    EXPECT_CALL(*wproxy.initial_acknack_, restart_timer()).Times(1u);
    wproxy.start(wattr);

    // Simulate Writer initial HEARTBEAT if its history is empty
    bool assert_liveliness = false;
    wproxy.process_heartbeat( 1, SequenceNumber_t( 0, 1 ), SequenceNumber_t( 0, 0 ), false, false, false, assert_liveliness );

    // Simulate reception of a HEARTBEAT with last sequence number = 3

    wproxy.process_heartbeat( 1, SequenceNumber_t( 0, 1 ), SequenceNumber_t( 0, 3 ), false, false, false, assert_liveliness);

    //ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 0));
    //ASSERT_EQ(wproxy.changes_from_writer_.size(), 3u);
    //ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 1))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    //ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 2))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    //ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 3))->getStatus(), ChangeFromWriterStatus_t::MISSING);



    // Add two UNKNOWN with sequence numberes 4 and 5.
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0,4)));
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0,5)));

    // Update MISSING changes util sequence number 5.
    wproxy.missing_changes_update(SequenceNumber_t(0,5));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 0));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 5u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 1))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 2))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 3))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 4))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 5))->getStatus(), ChangeFromWriterStatus_t::MISSING);

    // Set all as received.
    wproxy.received_change_set(SequenceNumber_t(0, 1));
    wproxy.received_change_set(SequenceNumber_t(0, 2));
    wproxy.received_change_set(SequenceNumber_t(0, 3));
    wproxy.received_change_set(SequenceNumber_t(0, 4));
    wproxy.received_change_set(SequenceNumber_t(0, 5));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 5));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 0u);

    // Try to update MISSING changes util sequence number 4.
    wproxy.missing_changes_update(SequenceNumber_t(0, 4));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 5));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 0u);

    // Add three UNKNOWN changes with sequence number 6, 7 and 9.
    // Add one RECEIVED change with sequence number 8.
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0, 6)));
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0, 7)));
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0, 8)));
    wproxy.received_change_set(SequenceNumber_t(0, 8));
    wproxy.changes_from_writer_.insert(ChangeFromWriter_t(SequenceNumber_t(0, 9))); // doesn't make sense

    // Update MISSING changes util sequence number 8.
    wproxy.missing_changes_update(SequenceNumber_t(0, 8));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 5));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 4u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 7))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 9))->getStatus(), ChangeFromWriterStatus_t::UNKNOWN);

    // Update MISSING changes util sequence number 10.
    wproxy.missing_changes_update(SequenceNumber_t(0, 10));
    ASSERT_EQ(wproxy.changes_from_writer_low_mark_, SequenceNumber_t(0, 5));
    ASSERT_EQ(wproxy.changes_from_writer_.size(), 5u);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 6))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 7))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 8))->getStatus(), ChangeFromWriterStatus_t::RECEIVED);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 9))->getStatus(), ChangeFromWriterStatus_t::MISSING);
    ASSERT_EQ(wproxy.changes_from_writer_.find(SequenceNumber_t(0, 10))->getStatus(), ChangeFromWriterStatus_t::MISSING);
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
    WriterProxy wproxy(&readerMock,
                       RemoteLocatorsAllocationAttributes(),
                       ResourceLimitedContainerConfig());

    /// Tests that initial acknack timed event is updated with new interval
    /// Tests that heartbeat response timed event is updated with new interval
    /// Tests that initial acknack timed event is started

    EXPECT_CALL(*wproxy.initial_acknack_, update_interval(readerMock.getTimes().initialAcknackDelay)).Times(1u);
    EXPECT_CALL(*wproxy.heartbeat_response_, update_interval(readerMock.getTimes().heartbeatResponseDelay)).Times(1u);
    EXPECT_CALL(*wproxy.initial_acknack_, restart_timer()).Times(1u);
    wproxy.start(wattr);

    // Writer proxy receives sequence number 3
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be UNKNOWN
    // Sequence number 3 should be RECEIVED

    wproxy.received_change_set(SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 3u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), false);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 2u);

    // Writer proxy receives sequence number 6
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be UNKNOWN
    // Sequence number 3 should be RECEIVED
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED

    wproxy.received_change_set(SequenceNumber_t(0, 6));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 6u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), false);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 4u);

    // Writer proxy receives sequence number 2
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be RECEIVED
    // Sequence number 3 should be RECEIVED
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED

    wproxy.received_change_set(SequenceNumber_t(0, 2));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 6u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), false);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 1u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 3u);

    // Writer proxy receives sequence number 1
    // Sequence number 1 should be RECEIVED
    // Sequence number 2 should be RECEIVED
    // Sequence number 3 should be RECEIVED
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED
    // Sequence numbers 1, 2, and 3 are removed as they were all received

    wproxy.received_change_set(SequenceNumber_t(0, 1));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 3u);
    ASSERT_EQ(wproxy.are_there_missing_changes(), false);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 2u);

    // Writer proxy marks sequence number 3 as lost
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED
    // Sequence numbers 1, 2, and 3 are removed as they were all received

    wproxy.lost_changes_update(SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.are_there_missing_changes(), false);
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 3u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 2u);

    // Writer proxy receives sequence number 8
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED
    // Sequence number 7 should be UNKNOWN
    // Sequence number 8 should be RECEIVED

    wproxy.received_change_set(SequenceNumber_t(0, 8));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 5u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 3u);

    // Writer proxy receives sequence number 4
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED
    // Sequence number 7 should be UNKNOWN
    // Sequence number 8 should be RECEIVED

    wproxy.received_change_set(SequenceNumber_t(0, 4));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 4u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 1u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 2u);

    // Writer proxy receives sequence number 4
    // Sequence number 7 should be UNKNOWN
    // Sequence number 8 should be RECEIVED

    wproxy.received_change_set(SequenceNumber_t(0, 5));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 1u);

    // Writer proxy receives sequence number 7
    // No changes from writer

    wproxy.received_change_set(SequenceNumber_t(0, 7));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 0u);
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
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be UNKNOWN
    // Sequence number 3 should be RECEIVED (irrelevant)
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 3));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 3u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 2u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 4)), 1u);

    // Add irrelevant sequence number 6
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be UNKNOWN
    // Sequence number 3 should be RECEIVED (irrelevant)
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED (irrelevant)
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 6));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 6u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 4u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 4)), 1u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 7)), 2u);

    // Set irrelevant change with sequence number 2
    // Sequence number 1 should be UNKNOWN
    // Sequence number 2 should be RECEIVED (irrelevant)
    // Sequence number 3 should be RECEIVED (irrelevant)
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED (irrelevant)
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 2));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 6u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 4)), 1u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 3u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 4)), 2u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 7)), 3u);

    // Set irrelevant change with sequence number 1
    // Sequence numbers 1, 2, and 3 should be removed from writer proxy
    // Sequence number 1 should be RECEIVED (irrelevant)
    // Sequence number 2 should be RECEIVED (irrelevant)
    // Sequence number 3 should be RECEIVED (irrelevant)
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED (irrelevant)
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 1));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 3u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 2u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 7)), 1u);

    // Add irrelevant change with sequence number 8
    // Sequence number 4 should be UNKNOWN
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED (irrelevant)
    // Sequence number 7 should be UNKNOWN (irrelevant)
    // Sequence number 8 should be RECEIVED (irrelevant)
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 8));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 5u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 3u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 7)), 1u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 9)), 2u);

    // Add irrelevant change with sequence number 4
    // Sequence number 5 should be UNKNOWN
    // Sequence number 6 should be RECEIVED (irrelevant)
    // Sequence number 7 should be UNKNOWN (irrelevant)
    // Sequence number 8 should be RECEIVED (irrelevant)
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 4));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 4u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 7)), 1u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 2u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 7)), 1u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 9)), 2u);

    // Add irrelevant change with sequence number 5
    // Sequence number 7 should be UNKNOWN (irrelevant)
    // Sequence number 8 should be RECEIVED (irrelevant)
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 5));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 2u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 1u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 9)), 1u);

    // Add irrelevant change with sequence number 7
    // All sequence numbers received, no changes from writer
    wproxy.irrelevant_change_set(SequenceNumber_t(0, 7));
    ASSERT_EQ(wproxy.number_of_changes_from_writer(), 0u);
    ASSERT_EQ(wproxy.unknown_missing_changes_up_to(SequenceNumber_t(0, 9)), 0u);
    ASSERT_EQ(wproxy.irrelevant_changes_up_to(SequenceNumber_t(0, 9)), 0u);
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

