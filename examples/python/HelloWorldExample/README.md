This example shows how to use SWIG to create a FastDDS wrapper to be used form python.

## Requisites

1. FastDDS needs to be installed in your system
2. Python 3 needs to be installed to run the example
3. Python 3 development tools need to be installed to compile the example 
   - On *nix distros install the `python3-dev` package with your usual package management application.
   - On Windows, the python installer already adds the required files.
4. SWIG is installed in your system.
   - For Windows, please refer to the [SWIG download page](http://www.swig.org/download.html)
   - Many *nix distros provide the `swig` package that can be installed with your usual package management application.


## Running the example

Open two different consoles.

In the first one run a publisher:
```
python HelloWorldExample.py publisher
```

In the second one run a subscriber:
```
python HelloWorldExample.py subscriber
```

You should see the publisher sending data messages and the subscriber receiving them.

To stop the example, use `ctrl+C` on both consoles.


## Architectural description of the example

The example uses a layered design, each one providing an abstraction.
The files under `fastdds_wrapper` define a minimal interface to create a FastDDS DataWriter / DataReader
and a very simple data type for these endpoints to exchange.
The files under `swig` contain the information for SWIG to create the python bindings.

### Topic data type

This example uses the data type defined in ``HelloWorld.idl``.
This data type contains two fields:
  - An arbitrary string of characters `message`.
  - An integer `index`.

From this IDL definition, classes `HelloWorld` and `HelloWorldTopicDataType` are created
using [*Fast DDS-Gen*](https://fast-dds.docs.eprosima.com/en/latest/fastddsgen/introduction/introduction.html)

Class `HelloWorld` provides accessors and mutators for the `message` and `index` fields,
and also a series of operations required by FastDDS to handle the type.
Users do not need to worry about these extra operations.
Similarly, class `HelloWorldTopicDataType` is only needed internally to define the DataReader and DataWriter,
and has no interesting operations for the user.

### Simplified FastDDS interface

`FastddsDataWriter` is a template that creates a generic DataWriter suitable for any data type.
The constructor takes a Domain identifier and a topic name.
The only operation available in this minimal interface is to publish a new sample value using `write_sample`.

`FastddsDataReader` is a template that creates a generic DataReader suitable for any data type.
The constructor takes a Domain identifier and a topic name.
It provides two operations:
  - `wait_for_sample` blocks the calling thread until a new sample has been received or the given timeout expires.
  - `take_sample` copies any new sample received by the DataReader on the provided buffer.

Both templates take a `TopicDataType` as template argument.
In this example, we are using especializations for `HelloWorldTopicDataType`.


### Analysis of the SWIG configuration file

File `fastdds_wrapper.i` contains several relevant sections that we are going to describe.

#### Destination python module

```
%module fastdds_wrapper
```

This tells SWIG that all the python wrapper described in this file shall be created in the `fastds_wrapper` module.
This means that in python we will have to import this module in order to use the bindings.

#### C++ include section

```
%{
#include "FastddsDataReader.hpp"
#include "FastddsDataWriter.hpp"
#include "HelloWorld.h"
#include "HelloWorldTopicDataType.h"
%}
```

These are the C++ header files that describe the classes with which the `fastdds_wrapper` module has to interact.
Note that we have included the data type and DataReader and DataWriter classes here.
Although `HelloWorldTopicDataType` is only internal and the user is not going to interact with it,
SWIG still needs to know about this class in order to create the template specializations.

Note that the includes are surrounded by `%{ ... %}`.


#### Other include section

```
%include "std_string.i"
```

SWIG allows to include other SWIG configuration files.
In this case, `std_string.i` is a file supplied by SWIG itself that provides python bindings for `std::string`,
which we need for the DataReader and DataWriter constructors, as well as for the `message` field in the data type.

Note that this is a SWIG directive, and the command starts with `%`.

#### Alias section

```
typedef int uint32_t;
```

SWIG understands basic types (like `int` or `char`),
and converts them accordingly to the destination language equivalent.
However, we need to define our own equivalent for fixed-width types like `uint32_t`.
SWIG allows defining this equivalents with a `typedef` declaration.
In this case, we are telling SWIG that a `uint32_t` is equal to a standard integer.


#### Python interface section

The rest of the file defines which classes we want to be accessible in the `fastdds_wrapper`
and which of their public operations will be available.
Note that the C++ headers included above may declare more classes that we don't need to access from python,
or may define methods in these classes that we are not interested in using.
In other words, the python binding can *reduce* the declared C++ API.
For example, for the `HelloWorld` type we only need the field accessors and mutators:

```
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
```

With this, SWIG will provide a `fastdds_wrapper.HelloWorld` class
with four operations to manipulate its fields.

We are also binding the `HelloWorldTopicDataType`, even though we don't need to use it from python.
However, SWIG needs it to make the template specialization (see below).
That is why the binding of this class is empty except the constructor and destructor.


#### Template specialization

According to SWIG documentation, the tool can process and understand templates in the C++ input headers.
However, it cannot generate a wrapper for them.
One of the reasons is that not all target languages supports templages.

The solution is to provide an alias for each of the specializations we are using.
In this case, we have to define specializations of the DataReader and DataWriter for the
HelloWorld data type:

```
%template(HelloWorldDataWriter) FastddsDataWriter<HelloWorldTopicDataType>;
%template(HelloWorldDataReader) FastddsDataReader<HelloWorldTopicDataType>;
```

