# MultiThreaded Circular Buffer

The [circular buffer](http://www.boost.org/doc/libs/1_55_0/doc/html/circular_buffer.html)
that ships with [Boost](http://www.boost.org/) is great. However, it's
not thread safe. This one is.

Developed against and Boost 1.54 and tested with Boost 1.55.

## Installation

It's a header file. Put the header file in an include directory and you should
be good to go.

You'll also need Boost and [CppUnit](http://sourceforge.net/projects/cppunit/)
if you want to run the tests.

## Testing

Test coverage is as good as I could think up. During development, I thought my tests
covered every scenario but then in production my little buffer deadlocked. So I added
more tests, found the problem and it's been smooth sailing ever since.

To run the tests you'll need a C++11 compiler and
[CppUnit](http://sourceforge.net/projects/cppunit/).

`make test`

You'll probably have to twiddle the Makefile for your environment.

Speaking of environments, this has been tested on Linux and OSX, gcc/g++ and Clang.

The `master` branch _should_ compile on Linux, the `wolverine` branch _should_
compile on OSX.

## TODO

This is considered bug free and feature complete until new events warrant.

## License

Released under the MIT license. Have at thee!

## Authors

 * Kurt Neufeld (https://github.com/kneufeld)

_if you submit a patch make sure you append your name here_
