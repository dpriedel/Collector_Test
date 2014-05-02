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

#include <gmock/gmock.h>

#include "FTP_Connection.h"
#include "DailyIndexFileRetriever.h"
#include "FormFileRetriever.h"

#include "CollectEDGARApp.h"

//	need these to feed into CApplication.

int G_ARGC = 0;
char** G_ARGV = nullptr;

namespace fs = boost::filesystem;

using namespace testing;

/* MATCHER_P(DirectoryContainsNFiles, value, std::string(negation ? "doesn't" : "does") + */
/*                         " contain " + std::to_string(value) + " files") */
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


class DailyEndToEndTest : public Test
{
	public:
};


TEST(DailyEndToEndTest, VerifyDownloadCorrectNumberOfFormFilesForSingleIndexFile)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --begin-date 2013-Oct-14 --form-dir /tmp/forms"
		" --login aaa@bbb.com"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	try
	{
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
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
		"--index-only"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	try
	{
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();
		
		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
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
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index1 --form-dir /tmp/forms1 "
		"--login aaa@bbb.com "
		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	try
	{
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms1"), Eq(129));
}

TEST(DailyEndToEndTest, VerifyDoesNotDownloadFormFilesForMultipleIndexFilesWhenIndexOnlySpecified)
{
	fs::remove_all("/tmp/forms1");
	fs::create_directory("/tmp/forms1");
	
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index1 --form-dir /tmp/forms1 "
		"--login aaa@bbb.com "
		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 --index-only"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	try
	{
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms1"), Eq(0));
}

TEST(DailyEndToEndTest, VerifyNoDownloadsOfExistingIndexFilesWhenReplaceNotSpecifed)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index2 --form-dir /tmp/forms1 "
		"--login aaa@bbb.com "
		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 --index-only"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
	myApp.StartUp();

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
	myApp.Run();
	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	myApp.Run();
	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");

	myApp.Quit();
	
	ASSERT_THAT(x1 == x2, Eq(true));
}

TEST(DailyEndToEndTest, VerifyDownloadsOfExistingIndexFilesWhenReplaceIsSpecifed)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index2 --form-dir /tmp/forms1 "
		"--login aaa@bbb.com "
		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 "
		"--replace-index-files --index-only"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
	myApp.StartUp();

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
	myApp.Run();
	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	myApp.Run();
	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectory("/tmp/index2");

	myApp.Quit();
	
	ASSERT_THAT(x1 == x2, Eq(false));
}

TEST(DailyEndToEndTest, VerifyNoDownloadsOfExistingFormFilesWhenReplaceNotSpecifed)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index2 --form-dir /tmp/forms2 "
		"--login aaa@bbb.com "
		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 "};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
	myApp.StartUp();

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
	myApp.Run();
	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	myApp.Run();
	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");

	myApp.Quit();
	
	ASSERT_THAT(x1 == x2, Eq(true));
}

TEST(DailyEndToEndTest, VerifyDownloadsOfExistingFormFilesWhenReplaceIsSpecifed)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index2 --form-dir /tmp/forms2 "
		"--login aaa@bbb.com "
		"--begin-date 2013-Oct-14 --end-date 2013-Oct-17 "
		"--replace-form-files"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
	myApp.StartUp();

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
	myApp.Run();
	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	myApp.Run();
	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");

	myApp.Quit();
	
	ASSERT_THAT(x1 == x2, Eq(false));
}

class QuarterlyEndToEndTest : public Test
{
	public:
};

TEST(QuarterlyEndToEndTest, VerifyDownloadsOfCorrectQuaterlyIndexFileForSingleQuarter)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index3 "
		"--login aaa@bbb.com "
		"--begin-date 2000-Jan-01 "
		"--index-only --mode quarterly"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
	myApp.StartUp();

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
	myApp.Run();
	myApp.Quit();
	
	ASSERT_THAT(fs::exists("/tmp/index3/2000/QTR1/form.idx"), Eq(true));
}

TEST(QuarterlyEndToEndTest, VerifyDownloadsOfCorrectQuaterlyIndexFilesForDateRange)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index4 "
		"--login aaa@bbb.com "
		"--begin-date 2009-Sep-01 --end-date 2010-Oct-01 "
		"--index-only --mode quarterly"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
	myApp.StartUp();

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
	myApp.Run();
	myApp.Quit();
	
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/index4"), Eq(5));
}

TEST(QuarterlyEndToEndTest, VerifyDownloadsSampleOfQuaterlyFormFilesForQuarter)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index4 --form-dir /tmp/forms4 "
		"--login aaa@bbb.com "
		"--begin-date 2009-Sep-01  "
		" --mode quarterly"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
	myApp.StartUp();

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
	myApp.Run();
	myApp.Quit();
	
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms4"), Eq(10));
}

TEST(QuarterlyEndToEndTest, VerifyDownloadsSampleOfQuaterlyFormFilesForDateRange)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp/index5 --form-dir /tmp/forms5 "
		"--login aaa@bbb.com "
		"--begin-date 2009-Sep-01 --end-date 2010-Oct-04 "
		" --mode quarterly"};
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	try
	{
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms5"), Eq(10));
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
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(fs::exists("/tmp/the_log"), Eq(true));
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
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(fs::exists("/tmp/ticker_to_CIK"), Eq(true));
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
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
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
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
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
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
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
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --form-dir /tmp/forms8 "
		"--login aaa@bbb.com "
		" --begin-date 2013-Oct-17 "
		" --ticker AAPL"
		" --form 4" };
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	try
	{
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
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
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --form-dir /tmp/forms9 "
		"--login aaa@bbb.com "
		" --end-date 2013-Oct-17 "
		" --begin-date 2013-Oct-09 "
		" --ticker AAPL"
		" --form 4" };
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	try
	{
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
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
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --form-dir /tmp/forms10 "
		"--login aaa@bbb.com "
		" --begin-date 2013-Oct-17 "
		" --log-path /tmp/the_log "
		" --form 10-K,10-Q,4" };
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	try
	{
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
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
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::string command_line{"the_program --index-dir /tmp --form-dir /tmp/forms11 "
		"--login aaa@bbb.com "
		" --begin-date 2013-Oct-17 "
		" --ticker AAPL,DHS,GOOG"
		" --form 4" };
	//std::string command_line{"the_program --index-dir /tmp"};
	std::vector<std::string> tokens =  po::split_unix(command_line);		

	try
	{
		CollectEDGARApp myApp{G_ARGC, G_ARGV, tokens};
		myApp.StartUp();

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		std::clog << "\n\nTest: " << test_info->name() << " test case: " << test_info->test_case_name() << "\n\n";
		
		myApp.Run();
		myApp.Quit();
	}
	
	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms11"), Eq(5));
}


int main(int argc, char** argv) {
	G_ARGC = argc;
	G_ARGV = argv;
	testing::InitGoogleMock(&argc, argv);
   return RUN_ALL_TESTS();
}



