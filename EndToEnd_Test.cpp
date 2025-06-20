// =====================================================================================
//
//       Filename:  EndToEndTest.cpp
//
//    Description:  Driver program for end-to-end tests
//
//        Version:  1.0
//        Created:  01/03/2014 11:13:53 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:
//
// =====================================================================================

/* This file is part of CollectorData. */

/* CollectorData is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or */
/* (at your option) any later version. */

/* CollectorData is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License */
/* along with CollectorData.  If not, see <http://www.gnu.org/licenses/>. */

// =====================================================================================
//        Class:
//  Description:
// =====================================================================================

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <gmock/gmock.h>

#include "CollectorApp.h"

#include "Collector_Utils.h"

namespace fs = std::filesystem;
// namespace po = boost::program_options;

using namespace testing;

/* MATCHER_P(DirectoryContainsNFiles, value, std::string(negation ? "doesn't" :
 * "does") + */
/*                         " contain " + std::to_string(value) + " files") */

// NOTE:  file will be counted even if it is empty which suits my tests since a
// failed attempt will leave an empty file in the target directory.  This lets
// me see that an attempt was made and what file was to be downloaded which is
// all I need for these tests.

int CountFilesInDirectoryTree(const fs::path &directory)
{
    int count =
        std::count_if(fs::recursive_directory_iterator(directory), fs::recursive_directory_iterator(),
                      [](const fs::directory_entry &entry) { return entry.status().type() == fs::file_type::regular; });
    return count;
}

bool DirectoryTreeContainsDirectory(const fs::path &tree, const fs::path &directory)
{
    for (auto x = fs::recursive_directory_iterator(tree); x != fs::recursive_directory_iterator(); ++x)
    {
        if (x->status().type() == fs::file_type::directory)
        {
            if (x->path().filename() == directory)
            {
                return true;
            }
        }
    }
    return false;
}

std::map<std::string, fs::file_time_type> CollectLastModifiedTimesForFilesInDirectory(const fs::path &directory)
{
    std::map<std::string, fs::file_time_type> results;

    auto save_mod_time([&results](const auto &dir_ent) {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
            results[dir_ent.path().filename().string()] = fs::last_write_time(dir_ent.path());
        }
    });

    std::for_each(fs::directory_iterator(directory), fs::directory_iterator(), save_mod_time);

    return results;
}

std::map<std::string, fs::file_time_type> CollectLastModifiedTimesForFilesInDirectoryTree(const fs::path &directory)
{
    std::map<std::string, fs::file_time_type> results;

    auto save_mod_time([&results](const auto &dir_ent) {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
            results[dir_ent.path().filename().string()] = fs::last_write_time(dir_ent.path());
        }
    });

    std::for_each(fs::recursive_directory_iterator(directory), fs::recursive_directory_iterator(), save_mod_time);

    return results;
}

// NOTE: I'm going to run all these daily index tests against my local FTP
// server.

class DailyEndToEndTest : public Test
{
public:
};

TEST_F(DailyEndToEndTest, VerifyDownloadCorrectNumberOfFormFilesForSingleIndexFile)
{
    if (fs::exists("/tmp/master.20131011.idx"))
    {
        fs::remove("/tmp/master.20131011.idx");
    }

    if (fs::exists("/tmp/forms"))
    {
        fs::remove_all("/tmp/forms");
    }
    fs::create_directory("/tmp/forms");

    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program", "--index-dir", "/tmp",       "--begin-date", "2013-Oct-14",
                                    "--form-dir",  "/tmp/forms",  "--host",     "localhost",    "--port",
                                    "8443",        "--log-level", "information"};

    try
    {
        CollectorApp myApp(tokens);

        const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
    }

    // catch any problems trying to setup application

    catch (const std::exception &theProblem)
    {
        // poco_fatal(myApp->logger(), theProblem.what());

        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
        throw; //	so test framework will get it too.
    }
    catch (...)
    { // handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
        throw;
    }
    ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms"), Eq(18));
}

TEST_F(DailyEndToEndTest, VerifyDoesNotDownloadFormFilesWhenIndexOnlySpecified)
{
    if (fs::exists("/tmp/forms"))
    {
        fs::remove_all("/tmp/forms");
    }
    fs::create_directory("/tmp/forms");

    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program", "--index-dir", "/tmp",       "--begin-date",
                                    "2013-Oct-14", "--form-dir",  "/tmp/forms", "--host",
                                    "localhost",   "--port",      "8443",       "--index-only"};
    try
    {
        CollectorApp myApp(tokens);

        const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
    }

    catch (std::exception &theProblem)
    {
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
        throw; //	so test framework will get it too.
    }
    catch (...)
    { // handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
        throw;
    }
    ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms"), Eq(0));
}

TEST_F(DailyEndToEndTest, VerifyDownloadCorrectNumberOfFormFilesForMultipleIndexFiles)
{
    if (fs::exists("/tmp/index1"))
    {
        fs::remove_all("/tmp/index1");
    }
    if (fs::exists("/tmp/forms1"))
    {
        fs::remove_all("/tmp/forms1");
    }
    fs::create_directory("/tmp/forms1");

    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program", "--index-dir",  "/tmp/index1", "--form-dir", "/tmp/forms1",
                                    "--host",      "localhost",    "--port",      "8443",       "--max",
                                    "17",          "--begin-date", "2013-Oct-14", "--end-date", "2013-Oct-17"};

    try
    {
        CollectorApp myApp(tokens);

        const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
    }

    catch (std::exception &theProblem)
    {
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
        throw; //	so test framework will get it too.
    }
    catch (...)
    { // handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
        throw;
    }
    ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms1"), Eq(17));
}

TEST_F(DailyEndToEndTest, VerifyExceptionsThrownWhenDiskIsFull)
{
    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program",  "--index-dir",  "/tmp/ofstream_test/test_2",
                                    "--index-only", "--host",       "localhost",
                                    "--port",       "8443",         "--max",
                                    "17",           "--begin-date", "2013-Oct-14",
                                    "--end-date",   "2013-Oct-17"};

    try
    {
        CollectorApp myApp(tokens);

        const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            ASSERT_THROW(myApp.Run(), std::system_error);
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
    }

    catch (std::exception &theProblem)
    {
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
        throw; //	so test framework will get it too.
    }
    catch (...)
    { // handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
        throw;
    }
    ASSERT_THAT(fs::exists("/tmp/ofstream_test/test_2"), Eq(false));
}

TEST_F(DailyEndToEndTest, VerifyDoesNotDownloadFormFilesForMultipleIndexFilesWhenIndexOnlySpecified)
{
    if (fs::exists("/tmp/index1"))
    {
        fs::remove_all("/tmp/index1");
    }
    if (fs::exists("/tmp/forms1"))
    {
        fs::remove_all("/tmp/forms1");
    }
    fs::create_directory("/tmp/forms1");

    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program", "--index-dir", "/tmp/index1", "--form-dir",  "/tmp/forms1",
                                    "--host",      "localhost",   "--port",      "8443",        "--begin-date",
                                    "2013-Oct-14", "--end-date",  "2013-Oct-17", "--index-only"};

    try
    {
        CollectorApp myApp(tokens);

        const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
    }

    catch (std::exception &theProblem)
    {
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
        throw; //	so test framework will get it too.
    }
    catch (...)
    { // handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
        throw;
    }
    ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms1"), Eq(0));
}

// TEST(DailyEndToEndTest,
// VerifyNoDownloadsOfExistingIndexFilesWhenReplaceNotSpecifed)
// {
// 	fs::remove_all("/tmp/index2");
// 	fs::remove_all("/tmp/forms2");
// 	fs::create_directory("/tmp/forms2");
//
// 	std::string command_line{"CollectorApp --index-dir /tmp/index2
// --form-dir /tmp/forms1 "
//         "--host localhost "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 --index-only"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
// 	CollectorApp myApp;
//     myApp.init(tokens);
//
// 	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
// 	myApp.logger().information(std::string("\n\nTest: ") + test_info->name()
// + " test case: " + test_info->test_suite_name() + "\n\n");
//
//     myApp.run();
// 	decltype(auto) x1 =
// CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");
//
// 	std::this_thread::sleep_for(std::chrono::seconds{1});
//
// 	myApp.run();
// 	decltype(auto) x2 =
// CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");
//
// 	ASSERT_THAT(x1 == x2, Eq(true));
// }
//
// TEST(DailyEndToEndTest,
// VerifyDownloadsOfExistingIndexFilesWhenReplaceIsSpecifed)
// {
// 	fs::remove_all("/tmp/index2");
// 	fs::remove_all("/tmp/forms1");
// 	fs::create_directory("/tmp/forms1");
//
// 	std::string command_line{"CollectorApp --index-dir /tmp/index2
// --form-dir /tmp/forms1 "
//         "--host localhost "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 "
// 		"--replace-index-files --index-only"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
// 	CollectorApp myApp;
//     myApp.init(tokens);
//
// 	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
// 	myApp.logger().information(std::string("\n\nTest: ") + test_info->name()
// + " test case: " + test_info->test_suite_name() + "\n\n");
//
//     myApp.run();
// 	decltype(auto) x1 =
// CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");
//
// 	std::this_thread::sleep_for(std::chrono::seconds{1});
//
// 	myApp.run();
// 	decltype(auto) x2 =
// CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");
//
// 	ASSERT_THAT(x1 == x2, Eq(false));
// }
//
// TEST(DailyEndToEndTest,
// VerifyNoDownloadsOfExistingFormFilesWhenReplaceNotSpecifed)
// {
// 	fs::remove_all("/tmp/index2");
// 	fs::remove_all("/tmp/forms2");
// 	fs::create_directory("/tmp/forms2");
//
// 	//	NOTE: the program name 'the_program' in the command line below
// is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp/index2 --form-dir
// /tmp/forms2 "
//         "--host localhost "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 "};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
// 	CollectorApp myApp;
//     myApp.init(tokens);
//
// 	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
// 	myApp.logger().information(std::string("\n\nTest: ") + test_info->name()
// + " test case: " + test_info->test_suite_name() + "\n\n");
//
//     myApp.run();
// 	decltype(auto) x1 =
// CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");
//
// 	std::this_thread::sleep_for(std::chrono::seconds{1});
//
// 	myApp.run();
// 	decltype(auto) x2 =
// CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");
//
// 	ASSERT_THAT(x1 == x2, Eq(true));
// }
//
TEST_F(DailyEndToEndTest, VerifyDownloadsOfExistingFormFilesWhenReplaceIsSpecified)
{
    if (fs::exists("/tmp/index2"))
    {
        fs::remove_all("/tmp/index2");
    }
    if (fs::exists("/tmp/forms2"))
    {
        fs::remove_all("/tmp/forms2");
    }
    fs::create_directory("/tmp/forms2");

    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program", "--index-dir",
                                    "/tmp/index2", "--form-dir",
                                    "/tmp/forms2", "--host",
                                    "localhost",   "--port",
                                    "8443",        "--begin-date",
                                    "2013-Oct-14", "--end-date",
                                    "2013-Oct-17", "--replace-form-files"};

    CollectorApp myApp(tokens);

    const auto *test_info = UnitTest::GetInstance()->current_test_info();
    spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

    bool startup_OK = myApp.Startup();
    if (startup_OK)
    {
        myApp.Run();
        myApp.Shutdown();
    }
    else
    {
        std::cout << "Problems starting program.  No processing done.\n";
    }
    decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");

    std::this_thread::sleep_for(std::chrono::seconds{1});

    startup_OK = myApp.Startup();
    if (startup_OK)
    {
        myApp.Run();
        myApp.Shutdown();
    }
    else
    {
        std::cout << "Problems starting program.  No processing done.\n";
    }
    decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");

    ASSERT_FALSE(x1 == x2);
}

// // NOTE: the quarterly index tests will run against the actual EDGAR server.

class QuarterlyEndToEndTest : public Test
{
public:
};

TEST_F(QuarterlyEndToEndTest, VerifyDownloadsOfCorrectQuaterlyIndexFileForSingleQuarter)
{
    if (fs::exists("/tmp/index3"))
    {
        fs::remove_all("/tmp/index3");
    }

    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program", "--index-dir",  "/tmp/index3", "--host",
                                    "localhost",   "--port",       "8443",        "--begin-date",
                                    "2000-Jan-01", "--index-only", "--mode",      "quarterly"};

    CollectorApp myApp(tokens);

    const auto *test_info = UnitTest::GetInstance()->current_test_info();
    spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

    bool startup_OK = myApp.Startup();
    if (startup_OK)
    {
        myApp.Run();
        myApp.Shutdown();
    }
    else
    {
        std::cout << "Problems starting program.  No processing done.\n";
    }

    ASSERT_THAT(fs::exists("/tmp/index3/2000/QTR1/master.idx"), Eq(true));
}

// TEST(QuarterlyEndToEndTest,
// VerifyDownloadsOfCorrectQuaterlyIndexFilesForDateRange)
// {
// 	fs::remove_all("/tmp/index4");
//
// 	//	NOTE: the program name 'the_program' in the command line below
// is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp/index4 "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2009-Sep-01 --end-date 2010-Oct-01 "
// 		"--index-only --mode quarterly"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
// 	CollectorApp myApp;
//     myApp.init(tokens);
//
// 	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
// 	myApp.logger().information(std::string("\n\nTest: ") + test_info->name()
// + " test case: " + test_info->test_suite_name() + "\n\n");
//
//     myApp.run();
//
// 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/index4"), Eq(5));
// }
//
// TEST(QuarterlyEndToEndTest,
// VerifyDownloadsSampleOfQuaterlyFormFilesForQuarter)
// {
// 	fs::remove_all("/tmp/index4");
// 	fs::remove_all("/tmp/forms4");
//
// 	//	NOTE: the program name 'the_program' in the command line below
// is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp/index4 --form-dir
// /tmp/forms4 "
//         "--max 9 "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2009-Sep-01  "
// 		" --mode quarterly"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
// 	CollectorApp myApp;
//     myApp.init(tokens);
//
// 	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
// 	myApp.logger().information(std::string("\n\nTest: ") + test_info->name()
// + " test case: " + test_info->test_suite_name() + "\n\n");
//
//     myApp.run();
//
// 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms4"), Eq(9));
// }
//
TEST_F(QuarterlyEndToEndTest, VerifyDownloadsSampleOfQuaterlyFormFilesForDateRange)
{
    if (fs::exists("/tmp/index5"))
    {
        fs::remove_all("/tmp/index5");
    }
    if (fs::exists("/tmp/forms5"))
    {
        fs::remove_all("/tmp/forms5");
    }

    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program", "--index-dir", "/tmp/index5", "--form-dir",   "/tmp/forms5",
                                    "--max",       "10",          "--host",      "www.sec.gov",  "--port",
                                    "443",         "--log-level", "information", "--begin-date", "2009-Sep-01",
                                    "--end-date",  "2010-Oct-04", "--mode",      "quarterly"};

    try
    {
        CollectorApp myApp(tokens);

        const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
    }
    catch (std::exception &theProblem)
    {
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
        throw; //	so test framework will get it too.
    }
    catch (...)
    { // handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
        throw;
    }
    ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms5"), Eq(10));
}

class TickerEndToEndTest : public Test
{
public:
};

// TEST(TickerEndToEndTest, VerifyWritesToLogFile)
// {
//     if (fs::exists("/tmp/the_log"))
//         fs::remove("/tmp/the_log");
//
// 	//	NOTE: the program name 'the_program' in the command line below
// is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program  "
// 		"--login aaa@bbb.com "
// 		" --mode ticker-only --ticker AAPL "
// 		" --log-path /tmp/the_log"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
//     CollectorApp myApp;
// 	try
// 	{
//         myApp.init(tokens);
//
// 		decltype(auto) test_info =
// UnitTest::GetInstance()->current_test_info();
// 		myApp.logger().information(std::string("\n\nTest: ") +
// test_info->name() + " test case: " + test_info->test_suite_name() + "\n\n");
//
//         myApp.run();
// 	}
// 	catch (std::exception& theProblem)
// 	{
// 		myApp.logger().error(std::string("Something fundamental went
// wrong: ") + theProblem.what()); 		throw;	//	so test
// framework will get it too.
// 	}
// 	catch (...)
// 	{		// handle exception: unspecified
// 		myApp.logger().error("Something totally unexpected happened.");
// 		throw;
// 	}
// 	ASSERT_THAT((fs::exists("/tmp/the_log") && !
// fs::is_empty("/tmp/the_log")), Eq(true));
// }
//
TEST_F(TickerEndToEndTest, VerifyTickerLookupFor1Ticker)
{
    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program", "--log-level",    "debug",
                                    "--mode",      "ticker-only",    "--ticker",
                                    "AAPL",        "--ticker-cache", "/vol_DA/SEC/Ticker2CIK_CacheFile"};

    try
    {
        CollectorApp myApp(tokens);

        const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
    }
    catch (std::exception &theProblem)
    {
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
        throw; //	so test framework will get it too.
    }
    catch (...)
    { // handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
        throw;
    }
    ASSERT_THAT((fs::exists("/vol_DA/SEC/Ticker2CIK_CacheFile") && !fs::is_empty("/vol_DA/SEC/Ticker2CIK_CacheFile")),
                Eq(true));
}

// TEST(TickerEndToEndTest, VerifyTickerLookupForFileOfTickers)
// {
// 	//	NOTE: the program name 'the_program' in the command line below
// is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program  "
// 		"--login aaa@bbb.com "
// 		" --mode ticker-only --ticker-file ./test_tickers_file "
// 		" --log-path /tmp/the_log --ticker-cache
// /vol_DA/SEC/Ticker2CIK_CacheFile"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
//     CollectorApp myApp;
// 	try
// 	{
//         myApp.init(tokens);
//
// 		decltype(auto) test_info =
// UnitTest::GetInstance()->current_test_info();
// 		myApp.logger().information(std::string("\n\nTest: ") +
// test_info->name() + " test case: " + test_info->test_suite_name() + "\n\n");
//
//         myApp.run();
// 	}
// 	catch (std::exception& theProblem)
// 	{
// 		myApp.logger().error(std::string("Something fundamental went
// wrong: ") + theProblem.what()); 		throw;	//	so test
// framework will get it too.
// 	}
// 	catch (...)
// 	{		// handle exception: unspecified
// 		myApp.logger().error("Something totally unexpected happened.");
// 		throw;
// 	}
// 	ASSERT_THAT(fs::exists("/tmp/tickers_to_CIK"), Eq(true));
// }

// TEST(QuarterlyEndToEndTest,
// VerifyDownloadFiltersByTickerForQuaterlyFormFilesForSingleQuarter)
// {
// 	fs::remove_all("/tmp/forms6");
// 	fs::remove_all("/tmp/index5");
//
// 	//	NOTE: the program name 'the_program' in the command line below
// is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp/index5 --form-dir
// /tmp/forms6 "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2009-Sep-01 "
// 		" --mode quarterly "
// 		" --log-path /tmp/the_log "
// 		" --ticker AAPL"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
//     CollectorApp myApp;
// 	try
// 	{
//         myApp.init(tokens);
//
// 		decltype(auto) test_info =
// UnitTest::GetInstance()->current_test_info();
// 		myApp.logger().information(std::string("\n\nTest: ") +
// test_info->name() + " test case: " + test_info->test_suite_name() + "\n\n");
//
//         myApp.run();
// 	}
// 	catch (std::exception& theProblem)
// 	{
// 		myApp.logger().error(std::string("Something fundamental went
// wrong: ") + theProblem.what()); 		throw;	//	so test
// framework will get it too.
// 	}
// 	catch (...)
// 	{		// handle exception: unspecified
// 		myApp.logger().error("Something totally unexpected happened.");
// 		throw;
// 	}
// 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms6"), Eq(1));
// }
//
// TEST(QuarterlyEndToEndTest,
// VerifyDownloadFiltersByTickerForQuaterlyFormFilesForDateRange)
// {
// 	fs::remove_all("/tmp/forms7");
// 	fs::remove_all("/tmp/index5");
//
// 	//	NOTE: the program name 'the_program' in the command line below
// is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp/index5 --form-dir
// /tmp/forms7 "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2009-Sep-01 "
// 		" --end-date 2010-Oct-21 "
// 		" --mode quarterly "
// 		" --log-path /tmp/the_log "
// 		" --ticker AAPL"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
//     CollectorApp myApp;
// 	try
// 	{
//         myApp.init(tokens);
//
// 		decltype(auto) test_info =
// UnitTest::GetInstance()->current_test_info();
// 		myApp.logger().information(std::string("\n\nTest: ") +
// test_info->name() + " test case: " + test_info->test_suite_name() + "\n\n");
//
//         myApp.run();
// 	}
// 	catch (std::exception& theProblem)
// 	{
// 		myApp.logger().error(std::string("Something fundamental went
// wrong: ") + theProblem.what()); 		throw;	//	so test
// framework will get it too.
// 	}
// 	catch (...)
// 	{		// handle exception: unspecified
// 		myApp.logger().error("Something totally unexpected happened.");
// 		throw;
// 	}
// 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms7"), Eq(4));
// }
//
class DailyEndToEndTestWithTicker : public Test
{
public:
};

// TEST(DailyEndToEndTestWithTicker,
// VerifyDownloadCorrectNumberOfFormFilesForSingleIndexFileWithTickerFilter)
// {
// 	fs::remove_all("/tmp/forms8");
//
// 	//	NOTE: the program name 'the_program' in the command line below
// is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp --form-dir
// /tmp/forms8 "
//         "--host localhost "
// 		"--login aaa@bbb.com "
// 		" --begin-date 2013-Oct-17 "
// 		" --ticker AAPL"
// 		" --form 4" };
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
//     CollectorApp myApp;
// 	try
// 	{
//         myApp.init(tokens);
//
// 		decltype(auto) test_info =
// UnitTest::GetInstance()->current_test_info();
// 		myApp.logger().information(std::string("\n\nTest: ") +
// test_info->name() + " test case: " + test_info->test_suite_name() + "\n\n");
//
//         myApp.run();
// 	}
//
// 	catch (std::exception& theProblem)
// 	{
// 		myApp.logger().error(std::string("Something fundamental went
// wrong: ") + theProblem.what()); 		throw;	//	so test
// framework will get it too.
// 	}
// 	catch (...)
// 	{		// handle exception: unspecified
// 		myApp.logger().error("Something totally unexpected happened.");
// 		throw;
// 	}
// 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms8"), Eq(4));
// }
//
TEST_F(DailyEndToEndTestWithTicker, VerifyDownloadCorrectNumberOfFormFilesForDateRangeWithTickerFilter)
{
    if (fs::exists("/tmp/index9"))
    {
        fs::remove_all("/tmp/index9");
    }

    if (fs::exists("/tmp/forms9"))
    {
        fs::remove_all("/tmp/forms9");
    }

    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program",
                                    "--index-dir",
                                    "/tmp/index9",
                                    "--form-dir",
                                    "/tmp/forms9",
                                    "--host",
                                    "localhost",
                                    "--port",
                                    "8443",
                                    "--end-date",
                                    "2013-Oct-17",
                                    "--begin-date",
                                    "2013-Oct-09",
                                    "--ticker",
                                    "AAPL",
                                    "--ticker-cache",
                                    "/vol_DA/SEC/Ticker2CIK_CacheFile",
                                    "--log-level",
                                    "information",
                                    "--form",
                                    "4"};

    try
    {
        CollectorApp myApp(tokens);

        const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
    }

    catch (std::exception &theProblem)
    {
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
        throw; //	so test framework will get it too.
    }
    catch (...)
    { // handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
        throw;
    }
    ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms9"), Eq(5));
}

// class DailyEndToEndTestWithMultipleFormTypes : public Test
// {
// 	public:
// };
//
// TEST(DailyEndToEndTestWithMultipleFormTypes,
// VerifyDownloadMultipleTypesOfFormFilesForSingleDate)
// {
// 	fs::remove_all("/tmp/forms10");
//
// 	//	NOTE: the program name 'the_program' in the command line below
// is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp --form-dir
// /tmp/forms10 "
//         "--host localhost "
// 		"--login aaa@bbb.com "
// 		" --begin-date 2013-Oct-17 "
// 		" --log-path /tmp/the_log "
// 		" --form 10-K,10-Q,4" };
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
//     CollectorApp myApp;
// 	try
// 	{
//         myApp.init(tokens);
//
// 		decltype(auto) test_info =
// UnitTest::GetInstance()->current_test_info();
// 		myApp.logger().information(std::string("\n\nTest: ") +
// test_info->name() + " test case: " + test_info->test_suite_name() + "\n\n");
//
//         myApp.run();
// 	}
//
// 	catch (std::exception& theProblem)
// 	{
// 		myApp.logger().error(std::string("Something fundamental went
// wrong: ") + theProblem.what()); 		throw;	//	so test
// framework will get it too.
// 	}
// 	catch (...)
// 	{		// handle exception: unspecified
// 		myApp.logger().error("Something totally unexpected happened.");
// 		throw;
// 	}
// 	ASSERT_THAT(DirectoryTreeContainsDirectory("/tmp/forms10", "10-K"),
// Eq(true)); 	ASSERT_THAT(DirectoryTreeContainsDirectory("/tmp/forms10",
// "10-Q"), Eq(true));
// 	ASSERT_THAT(DirectoryTreeContainsDirectory("/tmp/forms10", "4"),
// Eq(true));
// }
//
// TEST(DailyEndToEndTestWithTicker,
// VerifyDownloadCorrectNumberOfFormFilesForSingleIndexFileWithMultipleTickerFilter)
// {
// 	fs::remove_all("/tmp/forms11");
//
// 	//	NOTE: the program name 'the_program' in the command line below
// is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp --form-dir
// /tmp/forms11 "
//         "--host localhost "
// 		"--login aaa@bbb.com "
// 		" --begin-date 2013-Oct-17 "
// 		" --ticker AAPL,DHS,GOOG"
// 		" --form 4" };
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
//     CollectorApp myApp;
// 	try
// 	{
//         myApp.init(tokens);
//
// 		decltype(auto) test_info =
// UnitTest::GetInstance()->current_test_info();
// 		myApp.logger().information(std::string("\n\nTest: ") +
// test_info->name() + " test case: " + test_info->test_suite_name() + "\n\n");
//
//         myApp.run();
// 	}
//
// 	catch (std::exception& theProblem)
// 	{
// 		myApp.logger().error(std::string("Something fundamental went
// wrong: ") + theProblem.what()); 		throw;	//	so test
// framework will get it too.
// 	}
// 	catch (...)
// 	{		// handle exception: unspecified
// 		myApp.logger().error("Something totally unexpected happened.");
// 		throw;
// 	}
// 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms11"), Eq(4));
// }
//

class EndToEndTestFinancialNotes : public Test
{
public:
};

TEST_F(EndToEndTestFinancialNotes, VerifyDownloadAndExtractionOfSpecifiedData)
{
    if (fs::exists("/tmp/fin_stmts_downloads"))
    {
        fs::remove_all("/tmp/fin_stmts_downloads");
    }

    //	NOTE: the program name 'the_program' in the command line below is
    // ignored in the 	the test program.

    std::vector<std::string> tokens{"the_program",
                                    "--host",
                                    "www.sec.gov",
                                    "--port",
                                    "443",
                                    "--end-date",
                                    "2024-Feb-17",
                                    "--begin-date",
                                    "2023-Aug-09",
                                    "--log-level",
                                    "debug",
                                    "--mode",
                                    "notes",
                                    "--notes-directory",
                                    "/tmp/fin_stmts_downloads"};

    try
    {
        CollectorApp myApp(tokens);

        const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ", test_info->test_suite_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
    }

    catch (std::exception &theProblem)
    {
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
        throw; //	so test framework will get it too.
    }
    catch (...)
    { // handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
        throw;
    }
    ASSERT_THAT(CountFilesInDirectoryTree("/tmp/fin_stmts_downloads"), Eq(33));
}

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * InitLogging Description:
 * =====================================================================================
 */
void InitLogging()
{
    // auto my_default_logger = spdlog::stdout_color_mt("testing_logger");
    // spdlog::set_default_logger(my_default_logger);

} /* -----  end of function InitLogging  ----- */

int main(int argc, char **argv)
{
    // simpler logging setup than unit test because here
    // the app class will set up required logging.

    auto my_default_logger = spdlog::stdout_color_mt("testing_logger");
    spdlog::set_default_logger(my_default_logger);

    // InitLogging();

    InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
