# try_sigsafe (C) / try_sigsafe_pp (C++) library

## Purpose

Try to execute some code, a (callback) function or method - catching signal traps/exceptions.

The point is, to allow division by zero, sigsegv oder floating point traps
in the code - but recover gracefully.

In a simple scenario, a longish operation could be interrupted with Ctrl-C,
without the need to check condition variables during that computation.

All that, without the need to check the values before an floating point or integer operation.


## Internal

The helper functions setups signal handlers to catch the errors ..
but also sets up setjmp() for longjmp() from within the signal handler.

This is legal in C .. and does the job - tested on Linux and Windows platforms.


## Limitations

The provided function should NOT allocate/reserve resources, e.g. memory, just registered on the local stack!
Such resources would LEAK - in case of an error condition triggering the signal handling.

Windows signal handling has several limitations: most signals as SIGUSR1/2 are not handled!


## License

MIT, see [LICENSE](LICENSE) file


## Examples

see the test files [test.cpp](test.cpp) for C++ and [test_c.c](test_c.c) for C as "simple" demonstration.


## Links / Resources

* https://stackoverflow.com/questions/2350489/how-to-catch-segmentation-fault-in-linux
* https://stackoverflow.com/questions/2350489/how-to-catch-segmentation-fault-in-linux/2436368#2436368
* https://stackoverflow.com/questions/2350489/how-to-catch-segmentation-fault-in-linux/53436496#53436496
* https://stackoverflow.com/questions/7334595/longjmp-out-of-signal-handler
* https://en.cppreference.com/w/c/program/longjmp
* https://man7.org/linux/man-pages/man7/signal.7.html
* https://man7.org/linux/man-pages/man3/longjmp.3.html
* https://learn.microsoft.com/de-de/cpp/c-runtime-library/signal-constants?view=msvc-170
* https://learn.microsoft.com/de-de/cpp/c-runtime-library/reference/signal?view=msvc-170
* https://en.cppreference.com/w/cpp/numeric/fenv.html
* https://stackoverflow.com/questions/19823143/thread-local-data-in-c
* https://en.cppreference.com/w/c/header/stdatomic.html

* MSVC _controlfp_s() https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2010/c9676k6h(v=vs.100)
* MSVC AddVectoredContinueHandler() https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-addvectoredcontinuehandler
* MSVC EXCEPTION_RECORD https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-exception_record
* PVECTORED_EXCEPTION_HANDLER callback function https://learn.microsoft.com/en-us/windows/win32/api/winnt/nc-winnt-pvectored_exception_handler
* Vectored Exception Handling https://learn.microsoft.com/en-us/windows/win32/debug/vectored-exception-handling
* AddVectoredExceptionHandler function https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-addvectoredexceptionhandler
