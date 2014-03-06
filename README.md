CollectEDGARData_Test
=====================

TDD related files for CollectEDGARData project


See the 'building' file from the companion CollectEDGARData project.  

This project requires both the app_framework and CollectEDGARData projects.

The EndToEnd_Test.cpp and Unit_Test.cpp files are an attempt to follow the TDD methodolgy for
C++11 as described in the book "Modern C++ Programming with Test Driven Development" by Langr.

I found this book to be very well written and helpful.

To build these programs requires the Google Test and Mock libraries (https://code.google.com/p/googletest/downloads/list)
and (https://code.google.com/p/googlemock/downloads/list).  I used version 1.7.

NOTE:  There is code in the CollectEDGARApp.cpp which is commented out but should be uncommented when doing testing.

The methods containing this code are: Do_Run_DailyIndexFiles and Do_Run_QuarterlyIndexFiles.  What the commented
out code does is limit the number of forms to be downloaded to a random selection of 10 forms per form type.  This is because
the EDGAR FTP server seems to refuse connections if you hit it too hard during the daytime.


