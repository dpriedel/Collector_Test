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

	/* This file is part of CollectEDGARData. */

	/* CollectEDGARData is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* CollectEDGARData is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with CollectEDGARData.  If not, see <http://www.gnu.org/licenses/>. */


// =====================================================================================
//        Class:
//  Description:
// =====================================================================================


#include <algorithm>
#include <string>
#include <chrono>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/program_options/parsers.hpp>

#include <gmock/gmock.h>

#include "FTP_Connection.h"
#include "DailyIndexFileRetriever.h"
#include "FormFileRetriever.h"

#include "CollectEDGARApp.h"

//	need these to feed into CollectEDGARApp.

int G_ARGC = 0;
char** G_ARGV = nullptr;

namespace fs = boost::filesystem;
namespace po = boost::program_options;

using namespace testing;

/* MATCHER_P(DirectoryContainsNFiles, value, std::string(negation ? "doesn't" : "does") + */
/*                         " contain " + std::to_string(value) + " files") */

// NOTE:  file will be counted even if it is empty which suits my tests since a failed
// attempt will leave an empty file in the target directory.  This lets me see
// that an attempt was made and what file was to be downloaded which is all I need
// for these tests.

int CountFilesInDirectoryTree(const fs::path& directory)
{
	int count = std::count_if(fs::recursive_directory_iterator(directory), fs::recursive_directory_iterator(),
			[](const fs::directory_entry& entry) { return entry.status().type() == fs::file_type::regular_file; });
	return count;
}

bool DirectoryTreeContainsDirectory(const fs::path& tree, const fs::path& directory)
{
	for (auto x = fs::recursive_directory_iterator(tree); x != fs::recursive_directory_iterator(); ++x)
	{
		if (x->status().type() == fs::file_type::directory_file)
		{
			if (x->path().leaf() == directory)
				return true;
		}
	}
	return false;
}

std::map<std::string, std::time_t> CollectLastModifiedTimesForFilesInDirectory(const fs::path& directory)
{
	std::map<std::string, std::time_t> results;
	for (auto x = fs::directory_iterator(directory); x != fs::directory_iterator(); ++x)
	{
		results[x->path().leaf().string()] = fs::last_write_time(x->path());
	}

	return results;
}

std::map<std::string, std::time_t> CollectLastModifiedTimesForFilesInDirectoryTree(const fs::path& directory)
{
	std::map<std::string, std::time_t> results;
	for (auto x = fs::recursive_directory_iterator(directory); x != fs::recursive_directory_iterator(); ++x)
	{
		results[x->path().leaf().string()] = fs::last_write_time(x->path());
	}

	return results;
}


// NOTE: I'm going to run all these daily index tests against my local FTP server.

class DailyEndToEndTest : public Test
{
	public:
};


TEST(DailyEndToEndTest, VerifyDownloadCorrectNumberOfFormFilesForSingleIndexFile)
{
	fs::remove_all("/tmp/forms");
	fs::create_directory("/tmp/forms");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --begin-date 2013-Oct-14 --form-dir /tmp/forms"
        " --host localhost"
		" --login aaa@bbb.com"};
	// std::string command_line{"EndToEndTest --begin-date 2013-Oct-14"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}

    // catch any problems trying to setup application

	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms"), Eq(18));
}

TEST(DailyEndToEndTest, VerifyDoesNotDownloadFormFilesWhenIndexOnlySpecified)
{
	fs::remove_all("/tmp/forms");
	fs::create_directory("/tmp/forms");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --begin-date 2013-Oct-14 --form-dir /tmp/forms "
		"--login aaa@bbb.com "
        "--host localhost "
		"--index-only"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}

	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms"), Eq(0));
}

TEST(DailyEndToEndTest, VerifyDownloadCorrectNumberOfFormFilesForMultipleIndexFiles)
{
	fs::remove_all("/tmp/index1");
	fs::remove_all("/tmp/forms1");
	fs::create_directory("/tmp/forms1");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index1 --form-dir /tmp/forms1 "
		"--login aaa@bbb.com "
        "--host localhost "
        "--max 17 "
		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}

	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms1"), Eq(17));
}

TEST(DailyEndToEndTest, VerifyDoesNotDownloadFormFilesForMultipleIndexFilesWhenIndexOnlySpecified)
{
	fs::remove_all("/tmp/index1");
	fs::remove_all("/tmp/forms1");
	fs::create_directory("/tmp/forms1");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index1 --form-dir /tmp/forms1 "
        "--host localhost "
		"--login aaa@bbb.com "
		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 --index-only"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}

	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms1"), Eq(0));
}

// TEST(DailyEndToEndTest, VerifyNoDownloadsOfExistingIndexFilesWhenReplaceNotSpecifed)
// {
// 	fs::remove_all("/tmp/index2");
// 	fs::remove_all("/tmp/forms2");
// 	fs::create_directory("/tmp/forms2");
//
// 	//	NOTE: the program name 'the_program' in the command line below is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp/index2 --form-dir /tmp/forms1 "
//         "--host localhost "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 --index-only"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
// 		CollectEDGARApp myApp;
//         myApp.init(tokens);
//
// 		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
// 		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
//
//         myApp.run();
// 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");
//
// 	std::this_thread::sleep_for(std::chrono::seconds{1});
//
// 	myApp.Run();
// 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");
//
// 	myApp.Quit();
//
// 	ASSERT_THAT(x1 == x2, Eq(true));
// }
//
// TEST(DailyEndToEndTest, VerifyDownloadsOfExistingIndexFilesWhenReplaceIsSpecifed)
// {
// 	fs::remove_all("/tmp/index2");
// 	fs::remove_all("/tmp/forms1");
// 	fs::create_directory("/tmp/forms1");
//
// 	//	NOTE: the program name 'the_program' in the command line below is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp/index2 --form-dir /tmp/forms1 "
//         "--host localhost "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 "
// 		"--replace-index-files --index-only"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
// 		CollectEDGARApp myApp;
//         myApp.init(tokens);
//
// 		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
// 		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
//
//         myApp.run();
// 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");
//
// 	std::this_thread::sleep_for(std::chrono::seconds{1});
//
// 	myApp.Run();
// 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");
//
// 	myApp.Quit();
//
// 	ASSERT_THAT(x1 == x2, Eq(false));
// }

// TEST(DailyEndToEndTest, VerifyNoDownloadsOfExistingFormFilesWhenReplaceNotSpecifed)
// {
// 	fs::remove_all("/tmp/index2");
// 	fs::remove_all("/tmp/forms2");
// 	fs::create_directory("/tmp/forms2");
//
// 	//	NOTE: the program name 'the_program' in the command line below is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp/index2 --form-dir /tmp/forms2 "
//         "--host localhost "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 "};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
// 	CollectEDGARApp myApp;
//     myApp.init(tokens);
//
// 	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
// 	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
//
//     myApp.run();
// 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");
//
// 	std::this_thread::sleep_for(std::chrono::seconds{1});
//
// 	myApp.run();
// 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");
//
// 	myApp.Quit();
//
// 	ASSERT_THAT(x1 == x2, Eq(true));
// }

// TEST(DailyEndToEndTest, VerifyDownloadsOfExistingFormFilesWhenReplaceIsSpecifed)
// {
// 	fs::remove_all("/tmp/index2");
// 	fs::remove_all("/tmp/forms2");
// 	fs::create_directory("/tmp/forms2");
//
// 	//	NOTE: the program name 'the_program' in the command line below is ignored in the
// 	//	the test program.
//
// 	std::string command_line{"the_program --index-dir /tmp/index2 --form-dir /tmp/forms2 "
//         "--host localhost "
// 		"--login aaa@bbb.com "
// 		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 "
// 		"--replace-form-files"};
// 	//std::string command_line{"the_program --index-dir /tmp"};
// 	std::vector<std::string> tokens =  po::split_unix(command_line);
//
// 		CollectEDGARApp myApp;
//         myApp.init(tokens);
//
// 		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
// 		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
//
//         myApp.run();
// 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");
//
// 	std::this_thread::sleep_for(std::chrono::seconds{1});
//
// 	myApp.Run();
// 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");
//
// 	myApp.Quit();
//
// 	ASSERT_THAT(x1 == x2, Eq(false));
// }

// NOTE: the quarterly index tests will run against the actual EDGAR server.

class QuarterlyEndToEndTest : public Test
{
	public:
};

TEST(QuarterlyEndToEndTest, VerifyDownloadsOfCorrectQuaterlyIndexFileForSingleQuarter)
{
	fs::remove_all("/tmp/index3");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index3 "
		"--login aaa@bbb.com "
		"--begin-date 2000-Jan-01 "
		"--index-only --mode quarterly"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	CollectEDGARApp myApp;
    myApp.init(tokens);

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

    myApp.run();

	ASSERT_THAT(fs::exists("/tmp/index3/2000/QTR1/form.idx"), Eq(true));
}

TEST(QuarterlyEndToEndTest, VerifyDownloadsOfCorrectQuaterlyIndexFilesForDateRange)
{
	fs::remove_all("/tmp/index4");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index4 "
		"--login aaa@bbb.com "
		"--begin-date 2009-Sep-01 --end-date 2010-Oct-01 "
		"--index-only --mode quarterly"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	CollectEDGARApp myApp;
    myApp.init(tokens);

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

    myApp.run();

	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/index4"), Eq(5));
}

TEST(QuarterlyEndToEndTest, VerifyDownloadsSampleOfQuaterlyFormFilesForQuarter)
{
	fs::remove_all("/tmp/index4");
	fs::remove_all("/tmp/forms4");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index4 --form-dir /tmp/forms4 "
        "--max 9 "
		"--login aaa@bbb.com "
		"--begin-date 2009-Sep-01  "
		" --mode quarterly"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	CollectEDGARApp myApp;
    myApp.init(tokens);

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

    myApp.run();

	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms4"), Eq(9));
}

TEST(QuarterlyEndToEndTest, VerifyDownloadsSampleOfQuaterlyFormFilesForDateRange)
{
	fs::remove_all("/tmp/index5");
	fs::remove_all("/tmp/forms5");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index5 --form-dir /tmp/forms5 "
        "--max 11 "
		"--login aaa@bbb.com "
		"--begin-date 2009-Sep-01 --end-date 2010-Oct-04 "
		" --mode quarterly"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}
	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms5"), Eq(11));
}

class TickerEndToEndTest : public Test
{
	public:
};

TEST(TickerEndToEndTest, VerifyWritesToLogFile)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program  "
		"--login aaa@bbb.com "
		" --mode ticker-only --ticker AAPL "
		" --log-path /tmp/the_log"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}
	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT((fs::exists("/tmp/the_log") && ! fs::is_empty("/tmp/the_log")), Eq(true));
}

TEST(TickerEndToEndTest, VerifyTickerLookupFor1Ticker)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program  "
		"--login aaa@bbb.com "
		" --mode ticker-only --ticker AAPL "
		" --log-path /tmp/the_log --ticker-cache /tmp/ticker_to_CIK"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}
	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT((fs::exists("/tmp/ticker_to_CIK") && ! fs::is_empty("/tmp/ticker_to_CIK")), Eq(true));
}

TEST(TickerEndToEndTest, VerifyTickerLookupForFileOfTickers)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program  "
		"--login aaa@bbb.com "
		" --mode ticker-only --ticker-file ./test_tickers_file "
		" --log-path /tmp/the_log --ticker-cache /tmp/tickers_to_CIK"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}
	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(fs::exists("/tmp/tickers_to_CIK"), Eq(true));
}

TEST(QuarterlyEndToEndTest, VerifyDownloadFiltersByTickerForQuaterlyFormFilesForSingleQuarter)
{
	fs::remove_all("/tmp/forms6");
	fs::remove_all("/tmp/index5");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index5 --form-dir /tmp/forms6 "
		"--login aaa@bbb.com "
		"--begin-date 2009-Sep-01 "
		" --mode quarterly "
		" --log-path /tmp/the_log "
		" --ticker AAPL"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}
	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms6"), Eq(1));
}

TEST(QuarterlyEndToEndTest, VerifyDownloadFiltersByTickerForQuaterlyFormFilesForDateRange)
{
	fs::remove_all("/tmp/forms7");
	fs::remove_all("/tmp/index5");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index5 --form-dir /tmp/forms7 "
		"--login aaa@bbb.com "
		"--begin-date 2009-Sep-01 "
		" --end-date 2010-Oct-21 "
		" --mode quarterly "
		" --log-path /tmp/the_log "
		" --ticker AAPL"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}
	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms7"), Eq(4));
}

class DailyEndToEndTestWithTicker : public Test
{
	public:
};

TEST(DailyEndToEndTestWithTicker, VerifyDownloadCorrectNumberOfFormFilesForSingleIndexFileWithTickerFilter)
{
	fs::remove_all("/tmp/forms8");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --form-dir /tmp/forms8 "
        "--host localhost "
		"--login aaa@bbb.com "
		" --begin-date 2013-Oct-17 "
		" --ticker AAPL"
		" --form 4" };
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}

	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms8"), Eq(4));
}

TEST(DailyEndToEndTestWithTicker, VerifyDownloadCorrectNumberOfFormFilesForDateRangeWithTickerFilter)
{
	fs::remove_all("/tmp/forms9");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --form-dir /tmp/forms9 "
        "--host localhost "
		"--login aaa@bbb.com "
		" --end-date 2013-Oct-17 "
		" --begin-date 2013-Oct-09 "
		" --ticker AAPL"
		" --form 4" };
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}

	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms9"), Eq(5));
}

class DailyEndToEndTestWithMultipleFormTypes : public Test
{
	public:
};

TEST(DailyEndToEndTestWithMultipleFormTypes, VerifyDownloadMultipleTypesOfFormFilesForSingleDate)
{
	fs::remove_all("/tmp/forms10");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --form-dir /tmp/forms10 "
        "--host localhost "
		"--login aaa@bbb.com "
		" --begin-date 2013-Oct-17 "
		" --log-path /tmp/the_log "
		" --form 10-K,10-Q,4" };
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}

	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(DirectoryTreeContainsDirectory("/tmp/forms10", "10-K"), Eq(true));
	ASSERT_THAT(DirectoryTreeContainsDirectory("/tmp/forms10", "10-Q"), Eq(true));
	ASSERT_THAT(DirectoryTreeContainsDirectory("/tmp/forms10", "4"), Eq(true));
}

TEST(DailyEndToEndTestWithTicker, VerifyDownloadCorrectNumberOfFormFilesForSingleIndexFileWithMultipleTickerFilter)
{
	fs::remove_all("/tmp/forms11");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --form-dir /tmp/forms11 "
        "--host localhost "
		"--login aaa@bbb.com "
		" --begin-date 2013-Oct-17 "
		" --ticker AAPL,DHS,GOOG"
		" --form 4" };
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);

	try
	{
		CollectEDGARApp myApp;
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";

        myApp.run();
	}

	catch (std::exception& theProblem)
	{
		CollectEDGARApp::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms11"), Eq(4));
}


int main(int argc, char** argv) {
	G_ARGC = argc;
	G_ARGV = argv;
	testing::InitGoogleMock(&argc, argv);
   return RUN_ALL_TESTS();
}
