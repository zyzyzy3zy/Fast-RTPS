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

#include <fastdds/rtps/writer/StatefulWriter.h>
#include <fastdds/rtps/participant/RTPSParticipant.h>

using namespace eprosima::fastrtps::rtps;

class StatefulWriter_p: public StatefulWriter
{
    public:

    StatefulWriter_p(WriterAttributes& w_att) :
        StatefulWriter(RTPSParticipant(), GUID_t(), w_att, WriterHistory()) {}

    StatefulWriter_p() :
        StatefulWriter(RTPSParticipant(), GUID_t(), WriterAttributes(), WriterHistory()) {}
};

class StatefulWriterTests : public ::testing::Test
{
};

TEST_F(StatefulWriterTests, updateAttributes_test)
{
    std::cout << "Starting test" << std::endl;
    StatefulWriter_p writer;
    EXPECT_NE(writer, nullptr);
}

int main(int argc, char **argv)
{
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
