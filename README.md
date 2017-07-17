CollectEDGARData_Test
=====================

TDD related files for CollectEDGARData project


See the 'building' file from the companion CollectEDGARData project.  

This project requires both the app_framework and CollectEDGARData projects.

The EndToEnd_Test.cpp and Unit_Test.cpp files are an attempt to follow the TDD methodolgy for
C++11 as described in the book "Modern C++ Programming with Test Driven Development" by Langr.

I found this book to be very well written and helpful.

To build these programs requires the Google Test and Mock libraries (https://code.google.com/p/googletest/downloads/list)
and (https://code.google.com/p/googlemock/downloads/list).  I used version 1.8.

NOTE:  Almost all of the test code now runs against a local test configuration on my system.
I have downloaded a selection of files from the EDGAR site into a directory hierarchy which mimics
that of the EDGAR server.  I use a small Python program to serve these files but any simple
directory server should do.


