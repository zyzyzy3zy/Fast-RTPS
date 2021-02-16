%module fastdds_wrapper
%{
#include "FastddsDataReader.hpp"
#include "FastddsDataWriter.hpp"
#include "HelloWorld.h"
#include "HelloWorldTopicDataType.h"
%}

%include "std_string.i"

typedef int uint32_t;

template<class CustomTopicDataType>
class FastddsDataWriter
{
public:
    typedef typename CustomTopicDataType::type type;

    FastddsDataWriter(
        uint32_t did,
        const std::string& topic_name);
    ~FastddsDataWriter();

    bool write_sample(
            type& msg);
};

template<class CustomTopicDataType>
class FastddsDataReader
{
public:
    typedef typename CustomTopicDataType::type type;

    FastddsDataReader(
        uint32_t did,
        const std::string& topic_name);
    ~FastddsDataReader();

    bool wait_for_sample(
        uint32_t seconds);
    bool take_sample(
            type& msg);
};

class HelloWorld
{
public:

    HelloWorld();
    ~HelloWorld();

    void index(uint32_t _index);
    uint32_t index();
    void message(const std::string& _message);
    std::string message();
};

class HelloWorldTopicDataType
{
public:
    typedef HelloWorld type;

    HelloWorldTopicDataType();
    ~HelloWorldTopicDataType();
};

%template(HelloWorldDataWriter) FastddsDataWriter<HelloWorldTopicDataType>;
%template(HelloWorldDataReader) FastddsDataReader<HelloWorldTopicDataType>;
