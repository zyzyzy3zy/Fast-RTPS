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

#include <rtps/reader/WriterProxy.cpp>

namespace eprosima {
namespace fastrtps {
namespace rtps {

TEST(WriterProxyTests, MissingChangesUpdateRefact)
{
    WriterProxyData wattr( 4u, 1u );
    StatefulReader readerMock;
    WriterProxy wproxy( &readerMock, RemoteLocatorsAllocationAttributes(), ResourceLimitedContainerConfig() );


}

TEST(WriterProxyTests, LostChangesUpdateRefact )
{
 
}

TEST(WriterProxyTests, ReceivedChangeSetRefact )
{

}

TEST(WriterProxyTests, IrrelevantChangeSetRefact )
{

}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima

int main(int argc, char **argv)
{
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
