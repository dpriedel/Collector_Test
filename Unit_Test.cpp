// =====================================================================================
//
//       Filename:  Unit_Test.cpp
//
//    Description:  Driver program for Unit tests
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


#include <string>
#include <chrono>
#include <thread>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <gmock/gmock.h>

#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/AutoPtr.h"
#include <iostream>
#include <sstream>
#include <Poco/Net/NetException.h>
#include "Poco/Logger.h"
#include "Poco/Channel.h"
#include "Poco/Message.h"

#include "Poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/HTTPSStreamFactory.h"
#include "Poco/Net/FTPStreamFactory.h"
#include "Poco/ConsoleChannel.h"
// #include "Poco/SimpleFileChannel.h"

#include "HTTPS_Downloader.h"
#include "DailyIndexFileRetriever.h"
#include "FormFileRetriever.h"
#include "QuarterlyIndexFileRetriever.h"
#include "TickerConverter.h"

namespace fs = boost::filesystem;

using namespace testing;


using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::AutoPtr;
using Poco::Net::HTTPStreamFactory;
using Poco::Net::HTTPSStreamFactory;
using Poco::Net::FTPStreamFactory;

//	need these to feed into testing framework.

int G_ARGC = 0;
char** G_ARGV = nullptr;

Poco::Logger* THE_LOGGER = nullptr;

// using one of the example Poco programs to get going

class EDGAR_UnitTest: public Application
	/// This sample demonstrates some of the features of the Util::Application class,
	/// such as configuration file handling and command line arguments processing.
	///
	/// Try EDGAR_UnitTest --help (on Unix platforms) or EDGAR_UnitTest /help (elsewhere) for
	/// more information.
{
public:
	EDGAR_UnitTest(): _helpRequested(false)
	{
	}

protected:
	void initialize(Application& self)
	{
		loadConfiguration(); // load default configuration files, if present
		Application::initialize(self);
		// add your own initialization code here
	}

	void uninitialize()
	{
		// add your own uninitialization code here
		Application::uninitialize();
	}

	void reinitialize(Application& self)
	{
		Application::reinitialize(self);
		// add your own reinitialization code here
	}

	void defineOptions(OptionSet& options)
	{
		Application::defineOptions(options);

		options.addOption(
			Option("help", "h", "display help information on command line arguments")
				.required(false)
				.repeatable(false)
				.callback(OptionCallback<EDGAR_UnitTest>(this, &EDGAR_UnitTest::handleHelp)));

		options.addOption(
			Option("gtest_filter", "", "select which tests to run.")
				.required(false)
				.repeatable(true)
				.argument("name=value")
				.callback(OptionCallback<EDGAR_UnitTest>(this, &EDGAR_UnitTest::handleDefine)));

		/* options.addOption( */
		/* 	Option("define", "D", "define a configuration property") */
		/* 		.required(false) */
		/* 		.repeatable(true) */
		/* 		.argument("name=value") */
		/* 		.callback(OptionCallback<EDGAR_UnitTest>(this, &EDGAR_UnitTest::handleDefine))); */

		/* options.addOption( */
		/* 	Option("config-file", "f", "load configuration data from a file") */
		/* 		.required(false) */
		/* 		.repeatable(true) */
		/* 		.argument("file") */
		/* 		.callback(OptionCallback<EDGAR_UnitTest>(this, &EDGAR_UnitTest::handleConfig))); */

		/* options.addOption( */
		/* 	Option("bind", "b", "bind option value to test.property") */
		/* 		.required(false) */
		/* 		.repeatable(false) */
		/* 		.argument("value") */
		/* 		.binding("test.property")); */
	}

	void handleHelp(const std::string& name, const std::string& value)
	{
		_helpRequested = true;
		displayHelp();
		stopOptionsProcessing();
	}

	void handleDefine(const std::string& name, const std::string& value)
	{
		defineProperty(value);
	}

	void handleConfig(const std::string& name, const std::string& value)
	{
		loadConfiguration(value);
	}

	void displayHelp()
	{
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("A sample application that demonstrates some of the features of the Poco::Util::Application class.");
		helpFormatter.format(std::cout);
	}

	void defineProperty(const std::string& def)
	{
		std::string name;
		std::string value;
		std::string::size_type pos = def.find('=');
		if (pos != std::string::npos)
		{
			name.assign(def, 0, pos);
			value.assign(def, pos + 1, def.length() - pos);
		}
		else name = def;
		config().setString(name, value);
	}

	int main(const ArgVec& args)
	{
        setLogger(*THE_LOGGER);
		if (!_helpRequested)
		{
			HTTPStreamFactory::registerFactory();
			HTTPSStreamFactory::registerFactory();
			FTPStreamFactory::registerFactory();

			logger().information("Command line:");
			std::ostringstream ostr;
			for (ArgVec::const_iterator it = argv().begin(); it != argv().end(); ++it)
			{
				ostr << *it << ' ';
			}
			logger().information(ostr.str());
			logger().information("Arguments to main():");
			for (ArgVec::const_iterator it = args.begin(); it != args.end(); ++it)
			{
				logger().information(*it);
			}
			logger().information("Application properties:");
			printProperties("");

			//	run our tests

			testing::InitGoogleMock(&G_ARGC, G_ARGV);
			return RUN_ALL_TESTS();
		}
		return Application::EXIT_OK;
	}

	void printProperties(const std::string& base)
	{
		AbstractConfiguration::Keys keys;
		config().keys(base, keys);
		if (keys.empty())
		{
			if (config().hasProperty(base))
			{
				std::string msg;
				msg.append(base);
				msg.append(" = ");
				msg.append(config().getString(base));
				logger().information(msg);
			}
		}
		else
		{
			for (AbstractConfiguration::Keys::const_iterator it = keys.begin(); it != keys.end(); ++it)
			{
				std::string fullKey = base;
				if (!fullKey.empty()) fullKey += '.';
				fullKey.append(*it);
				printProperties(fullKey);
			}
		}
	}

private:
	bool _helpRequested;
};

//  some utility routines used to check test output.

MATCHER_P(StringEndsWith, value, std::string(negation ? "doesn't" : "does") +
                        " end with " + value)
{
	return boost::algorithm::ends_with(arg, value);
}

int CountFilesInDirectoryTree(const fs::path& directory)
{
	int count = std::count_if(fs::recursive_directory_iterator(directory), fs::recursive_directory_iterator(),
			[](const fs::directory_entry& entry) { return entry.status().type() == fs::file_type::regular_file; });
	return count;
}

std::map<std::string, std::time_t> CollectLastModifiedTimesForFilesInDirectory(const fs::path& directory)
{
	std::map<std::string, std::time_t> results;
    auto dir_end = fs::directory_iterator();
	for (auto x = fs::directory_iterator(directory); x != dir_end; ++x)
	{
		if (x->status().type() == fs::file_type::regular_file)
			results[x->path().leaf().string()] = fs::last_write_time(x->path());
	}

	return results;
}

std::map<std::string, std::time_t> CollectLastModifiedTimesForFilesInDirectoryTree(const fs::path& directory)
{
	std::map<std::string, std::time_t> results;
	auto dir_end = fs::recursive_directory_iterator();
	for (auto x = fs::recursive_directory_iterator(directory); x != dir_end; ++x)
	{
		if (x->status().type() == fs::file_type::regular_file)
			results[x->path().leaf().string()] = fs::last_write_time(x->path());
	}

	return results;
}

int CountTotalFormsFilesFound(const FormFileRetriever::FormsList& file_list)
{
	int grand_total{0};
	for (const auto& elem : file_list)
		grand_total += elem.second.size();

	return grand_total;
}

// NOTE: for some of these tests, I run an HTTPS server on localhost using
// a directory structure that mimics part of the SEC server.
//
class HTTPS_UnitTest : public Test
{


};

// /* TEST_F(RetrieverUnitTest, VerifyRetrievesIndexFileforNearestDate) */
// /* { */
// /* 	idxFileRet.CheckDate(std::string{"2013-12-13"}); */
// /* 	decltype(auto) nearest_date = idxFileRet.FindIndexFileDateNearest(); */
//
// /* 	idxFileRet.MakeRemoteIndexFileURI(); */
// /* 	idxFileRet.MakeLocalIndexFileName(); */
// /* 	idxFileRet.RetrieveRemoteIndexFile(); */
//
// /* 	ASSERT_THAT(fs::exists(idxFileRet.GetLocalIndexFileName()), Eq(true)); */
//
// /* } */
//
TEST_F(HTTPS_UnitTest, TestExceptionOnFailureToConnectToHTTPSServer)
{
	HTTPS_Downloader a_server{"https://xxxlocalhost:8443"};
	ASSERT_THROW(a_server.RetrieveDataFromServer(""), Poco::Net::NetException);
}

TEST_F(HTTPS_UnitTest, TestAbilityToConnectToHTTPSServer)
{
	HTTPS_Downloader a_server{"https://localhost:8443"};
	// ASSERT_NO_THROW(a_server.OpenHTTPSConnection());
	std::string data = a_server.RetrieveDataFromServer("/test.txt");
	ASSERT_THAT(data, Eq(std::string{"Hello, there!\n"}));
}

// /* TEST_F(FTP_UnitTest, TestAbilityToChangeWorkingDirectory) */
// /* { */
// /* 	FTP_Server a_server{"localhost", "anonymous", index_file_name"aaa@bbb.net"}; */
// /* 	a_server.OpenFTPConnection(); */
// /* 	a_server.ChangeWorkingDirectoryTo("edgar/daily-index"); */
// /* 	ASSERT_THAT(a_server.GetWorkingDirectory().size(), Gt(0)); */
//
// /* } */
//
// TEST_F(FTP_UnitTest, TestExceptionOnFailureToChangeWorkingDirectory)
// {
// 	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
// 	a_server.OpenFTPConnection();
// 	ASSERT_THROW(a_server.ChangeWorkingDirectoryTo("edgar/xxxxy-index"), Poco::Net::FTPException);
//
// }
//
TEST_F(HTTPS_UnitTest, TestAbilityToListDirectoryContents)
{
	HTTPS_Downloader a_server{"https://localhost:8443"};
	decltype(auto) directory_list = a_server.ListDirectoryContents("/edgar/full-index/2013/QTR4");
	ASSERT_TRUE(std::find(directory_list.begin(), directory_list.end(), "company.gz") != directory_list.end());

}

TEST_F(HTTPS_UnitTest, VerifyAbilityToDownloadFileWhichExists)
{
	if (fs::exists("/tmp/form.20131010.idx"))
		fs::remove("/tmp/form.20131010.idx");

	HTTPS_Downloader a_server{"https://localhost:8443"};
	a_server.DownloadFile("/edgar/daily-index/2013/QTR4/form.20131010.idx", "/tmp/form.20131010.idx");
	ASSERT_THAT(fs::exists("/tmp/form.20131010.idx"), Eq(true));
}

TEST_F(HTTPS_UnitTest, VerifyThrowsExceptionWhenTryToDownloadFileDoesntExist)
{
	if (fs::exists("/tmp/form.20131008.idx"))
		fs::remove("/tmp/form.20131008.idx");
	HTTPS_Downloader a_server{"https://localhost:8443"};
	ASSERT_THROW(a_server.DownloadFile("/edgar/daily-index/2013/QTR4/form.20131008.idx", "/tmp/form.20131008.idx"), std::runtime_error);
}

class RetrieverUnitTest : public Test
{
public:

	HTTPS_Downloader a_server{"https://localhost:8443"};
	DailyIndexFileRetriever idxFileRet{a_server, "/edgar/daily-index", *THE_LOGGER};
};

TEST_F(RetrieverUnitTest, VerifyRejectsInvalidDates)
{
	ASSERT_THROW(idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-Oxt-13")), std::out_of_range);
}

TEST_F(RetrieverUnitTest, VerifyRejectsFutureDates)
{
	// let's make a date in the future

	auto today = bg::day_clock::local_day();
	auto tomorrow = today + bg::date_duration(1);

	ASSERT_THROW(idxFileRet.FindRemoteIndexFileNameNearestDate(tomorrow), Poco::AssertionViolationException);
}

//	Finally, we get to the point

TEST_F(RetrieverUnitTest, TestFindIndexFileDateNearestWhereDateExists)
{
	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
	ASSERT_THAT(idxFileRet.GetActualIndexFileDate() == bg::from_simple_string("2013-10-11"), Eq(true));
}

TEST_F(RetrieverUnitTest, TestFindIndexFileDateNearestWhereDateDoesNotExist)
{
	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-14"));
	ASSERT_THAT(idxFileRet.GetActualIndexFileDate() == bg::from_simple_string("2013-10-11"), Eq(true));
}

// TEST_F(RetrieverUnitTest, TestExceptionThrownWhenRemoteIndexFileNameUnknown)
// {
// 	ASSERT_THROW(idxFileRet.CopyRemoteIndexFileTo("/tmp"), Poco::AssertionViolationException);
// }
//
 TEST_F(RetrieverUnitTest, TestRetrieveIndexFileWhereDateExists)
 {
	if (fs::exists("/tmp/form.20131011.idx"))
		fs::remove("/tmp/form.20131011.idx");

 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
 	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

 	ASSERT_THAT(fs::exists(local_daily_index_file_name), Eq(true));
 }

 TEST_F(RetrieverUnitTest, TestHierarchicalRetrieveIndexFileWhereDateExists)
 {
	if (fs::exists("/tmp/2013/QTR4/form.20131011.idx"))
		fs::remove("/tmp/2013/QTR4/form.20131011.idx");

 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
 	auto local_daily_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp");

 	ASSERT_THAT(fs::exists(local_daily_index_file_name), Eq(true));
 }

TEST_F(RetrieverUnitTest, TestRetrieveIndexFileDoesNotReplaceWhenReplaceNotSpecified)
{
	if (fs::exists("/tmp/form.20131011.idx"))
		fs::remove("/tmp/form.20131011.idx");

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");
	decltype(auto) d1 = fs::last_write_time(local_daily_index_file_name);

	std::this_thread::sleep_for(std::chrono::seconds{1});

	//	we will wait 1 second then do it again so we can compare timestamps

	idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp", false);
	decltype(auto) d2 = fs::last_write_time(local_daily_index_file_name);
	ASSERT_THAT(d2 == d1, Eq(true));
}

TEST_F(RetrieverUnitTest, TestHierarchicalRetrieveIndexFileDoesNotReplaceWhenReplaceNotSpecified)
{
	if (fs::exists("/tmp/2013/QTR4/form.20131011.idx"))
		fs::remove("/tmp/2013/QTR4/form.20131011.idx");

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
	auto local_daily_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp");
	decltype(auto) d1 = fs::last_write_time(local_daily_index_file_name);

	std::this_thread::sleep_for(std::chrono::seconds{1});

	//	we will wait 1 second then do it again so we can compare timestamps

	idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp", false);
	decltype(auto) d2 = fs::last_write_time(local_daily_index_file_name);
	ASSERT_THAT(d2 == d1, Eq(true));
}

TEST_F(RetrieverUnitTest, TestRetrieveIndexFileDoesReplaceWhenReplaceIsSpecified)
{
	if (fs::exists("/tmp/form.20131011.idx"))
		fs::remove("/tmp/form.20131011.idx");

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");
	decltype(auto) d1 = fs::last_write_time(local_daily_index_file_name);

	std::this_thread::sleep_for(std::chrono::seconds{1});

	//	we will wait 1 second then do it again so we can compare timestamps

	idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp", true);
	decltype(auto) d2 = fs::last_write_time(local_daily_index_file_name);
	ASSERT_THAT(d2 == d1, Eq(false));
}

TEST_F(RetrieverUnitTest, TestHierarchicalRetrieveIndexFileDoesReplaceWhenReplaceIsSpecified)
{
	if (fs::exists("/tmp/2013/QTR4/form.20131011.idx"))
		fs::remove("/tmp/2013/QTR4/form.20131011.idx");

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
	auto local_daily_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp");
	decltype(auto) d1 = fs::last_write_time(local_daily_index_file_name);

	std::this_thread::sleep_for(std::chrono::seconds{1});

	//	we will wait 1 second then do it again so we can compare timestamps

	idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp", true);
	decltype(auto) d2 = fs::last_write_time(local_daily_index_file_name);
	ASSERT_THAT(d2 == d1, Eq(false));
}

class ParserUnitTest : public Test
{
public:
	HTTPS_Downloader a_server{"https://localhost:8443"};
	/* FTP_Server a_server{"ftp.sec.gov", "anonymous", "aaa@bbb.net"}; */
	DailyIndexFileRetriever idxFileRet{a_server, "/edgar/daily-index", *THE_LOGGER};
};

//	The following block of tests relate to working with the Index file located above.

TEST_F(ParserUnitTest, VerifyFindProperNumberOfFormEntriesInIndexFile)
{
	if (fs::exists("/tmp/form.20131010.idx"))
		fs::remove("/tmp/form.20131010.idx");

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{a_server, *THE_LOGGER};
	std::vector<std::string> forms_list{"10-Q"};
	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);
	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(25));

}

TEST_F(ParserUnitTest, VerifyDownloadOfFormFilesListedInIndexFile)
{
	if (fs::exists("/tmp/form.20131010.idx"))
		fs::remove("/tmp/form.20131010.idx");

    if (fs::exists("/tmp/forms_unit"))
	   fs::remove_all("/tmp/forms_unit");

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{a_server, *THE_LOGGER};
	std::vector<std::string> forms_list{"10-Q"};
	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);

	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit");
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms_unit"), Eq(CountTotalFormsFilesFound(file_list)));
}

TEST_F(ParserUnitTest, VerifyDownloadOfFormFilesWithSlashInName)
{
	if (fs::exists("/tmp/form.20131010.idx"))
		fs::remove("/tmp/form.20131010.idx");

    if (fs::exists("/tmp/forms_unit"))
	   fs::remove_all("/tmp/forms_unit");

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{a_server, *THE_LOGGER};
	std::vector<std::string> forms_list{"10-Q/A"};
	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);

	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit");
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms_unit"), Eq(CountTotalFormsFilesFound(file_list)));
}

 TEST_F(ParserUnitTest, VerifyDownloadOfFormFilesDoesNotReplaceWhenReplaceNotSpecified)
 {
	if (fs::exists("/tmp/form.20131010.idx"))
		fs::remove("/tmp/form.20131010.idx");

    if (fs::exists("/tmp/forms_unit2"))
	   fs::remove_all("/tmp/forms_unit2");

 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
 	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{a_server, *THE_LOGGER};
 	std::vector<std::string> forms_list{"10-Q"};
 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);

 	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit2");
 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit2");

 	std::this_thread::sleep_for(std::chrono::seconds{1});

 	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit2");
 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit2");

 	ASSERT_THAT(x1 == x2, Eq(true));
 }

 TEST_F(ParserUnitTest, VerifyDownloadOfFormFilesDoesReplaceWhenReplaceIsSpecified)
 {
	if (fs::exists("/tmp/form.20131010.idx"))
		fs::remove("/tmp/form.20131010.idx");

    if (fs::exists("/tmp/forms_unit3"))
	   fs::remove_all("/tmp/forms_unit3");

 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
 	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{a_server, *THE_LOGGER};
 	std::vector<std::string> forms_list{"10-Q"};
 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);

 	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit3");
 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit3");

 	std::this_thread::sleep_for(std::chrono::seconds{1});


 	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit3", true);
 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit3");

 	ASSERT_THAT(x1 == x2, Eq(false));
 }

 TEST_F(ParserUnitTest, VerifyFindsCorrectNumberOfFormFilesListedInMultipleIndexFiles)
 {
    if (fs::exists("/tmp/downloaded_p"))
    fs::remove_all("/tmp/downloaded_p");

 	[[maybe_unused]] decltype(auto) results = idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-10"), bg::from_simple_string("2013-10-21"));
 	auto local_index_files = idxFileRet.CopyIndexFilesForDateRangeTo(results, "/tmp/downloaded_p");
 // 	decltype(auto) index_file_list = idxFileRet.GetfRemoteIndexFileNamesForDateRange();

    FormFileRetriever form_file_getter{a_server, *THE_LOGGER};
 	std::vector<std::string> forms_list{"10-Q"};
 	decltype(auto) form_file_list = form_file_getter.FindFilesForForms(forms_list, "/tmp/downloaded_p", local_index_files);

 	ASSERT_THAT(CountTotalFormsFilesFound(form_file_list), Eq(255));
 }


 class RetrieverMultipleDailies : public Test
 {
 public:
	HTTPS_Downloader a_server{"https://localhost:8443"};
	DailyIndexFileRetriever idxFileRet{a_server, "/edgar/daily-index", *THE_LOGGER};
 };

 TEST_F(RetrieverMultipleDailies, VerifyRejectsFutureDates)
 {
 	ASSERT_THROW(idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-13"), bg::from_simple_string("2018-10-18")),
 			   Poco::AssertionViolationException);
 }

 TEST_F(RetrieverMultipleDailies, VerifyFindsCorrectDateRangeWhenBoundingDatesNotFound)
 {
 	idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-14"), bg::from_simple_string("2013-10-20"));
 	decltype(auto) results = idxFileRet.GetActualDateRange();

 	ASSERT_THAT(results, Eq(std::make_pair(bg::from_simple_string("2013-Oct-15"), bg::from_simple_string("2013-10-18"))));
 }

 TEST_F(RetrieverMultipleDailies, VerifyFindsCorrectNumberOfIndexFilesInRange)
 {
 	decltype(auto) results = idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-10"), bg::from_simple_string("2013-10-21"));
 	ASSERT_THAT(results.size(), Eq(7));
 }

 TEST_F(RetrieverMultipleDailies, VerifyThrowsWhenNoIndexFilesInRange)
 {
 	ASSERT_THROW(idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2012-Oct-10"), bg::from_simple_string("2012-10-21")),
 			Poco::AssertionViolationException);
 }

 TEST_F(RetrieverMultipleDailies, VerifyDownloadsCorrectNumberOfIndexFilesForDateRange)
 {
    if (fs::exists("/tmp/downloaded_l"))
	   fs::remove_all("/tmp/downloaded_l");

 	decltype(auto) file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-10"), bg::from_simple_string("2013-10-21"));
 	idxFileRet.CopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l");

 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/downloaded_l"), Eq(file_list.size()));
 }

 TEST_F(RetrieverMultipleDailies, VerifyDownloadOfIndexFilesForDateRangeDoesNotReplaceWhenReplaceNotSpecified)
 {
    if (fs::exists("/tmp/downloaded_l"))
	   fs::remove_all("/tmp/downloaded_l");

 	[[maybe_unused]] decltype(auto) file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-10"), bg::from_simple_string("2013-10-21"));
 	idxFileRet.CopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l");
 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_l");

 	std::this_thread::sleep_for(std::chrono::seconds{1});

 	idxFileRet.CopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l");
 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_l");

 	ASSERT_THAT(x1 == x2, Eq(true));
 }

TEST_F(RetrieverMultipleDailies, VerifyDownloadOfIndexFilesForDateRangeDoesReplaceWhenReplaceIsSpecified)
 {
    if (fs::exists("/tmp/downloaded_l"))
	   fs::remove_all("/tmp/downloaded_l");

 	[[maybe_unused]] decltype(auto) file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-10"), bg::from_simple_string("2013-10-21"));
 	idxFileRet.CopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l");
 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_l");

 	std::this_thread::sleep_for(std::chrono::seconds{1});

 	idxFileRet.CopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l", true);
 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_l");

 	ASSERT_THAT(x1 == x2, Eq(false));
 }


class QuarterlyUnitTest : public Test
{
public:
	HTTPS_Downloader a_server{"https://localhost:8443"};
	QuarterlyIndexFileRetriever idxFileRet{a_server, "/edgar/full-index", *THE_LOGGER};
};

TEST_F(QuarterlyUnitTest, VerifyRejectsInvalidDates)
{
	ASSERT_THROW(idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2013-Oxt-13")), std::out_of_range);
}

 TEST_F(QuarterlyUnitTest, VerifyRejectsFutureDates)
 {
 	ASSERT_THROW(idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2018-10-13")), Poco::AssertionViolationException);
 }

 TEST_F(QuarterlyUnitTest, TestFindIndexFileGivenFirstDayInQuarter)
 {
 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2000-01-01"));
 	ASSERT_THAT(file_name == "/edgar/full-index/2000/QTR1/form.zip", Eq(true));
 }

 TEST_F(QuarterlyUnitTest, TestFindIndexFileGivenLastDayInQuarter)
 {
 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2002-06-30"));
 	ASSERT_THAT(file_name == "/edgar/full-index/2002/QTR2/form.zip", Eq(true));
 }

TEST_F(QuarterlyUnitTest, TestFindAllQuarterlyIndexFilesForAYear)
{
	decltype(auto) file_names = idxFileRet.MakeIndexFileNamesForDateRange(bg::from_simple_string("2002-Jan-01"), bg::from_string("2003-Jan-1"));
	ASSERT_THAT(file_names.size(), Eq(5));
}

TEST_F(QuarterlyUnitTest, TestDownloadQuarterlyIndexFile)
{
	if (fs::exists("/tmp/2000/QTR1/form.idx"))
		fs::remove("/tmp/2000/QTR1/form.idx");

	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2000-01-01"));
	auto local_daily_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp", true);

	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.idx"), Eq(true));
	//ASSERT_THAT(fs::exists(idxFileRet.GetLocalIndexFilePath()), Eq(true));
}

// /* TEST_F(QuarterlyUnitTest, TestDownloadQuarterlyIndexFileThenUnzipIt) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2000-01-01")); */
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp", true); */
//
// /* 	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.idx"), Eq(true)); */
// /* } */
//
// /* TEST_F(QuarterlyUnitTest, TestDownloadQuarterlyIndexFileThenUnzipItThenDeleteZipFile) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2000-01-01")); */
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp", true); */
//
// /* 	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.idx"), Eq(true)); */
// /* 	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.zip"), Eq(false)); */
// /* } */
//
// /* TEST_F(QuarterlyUnitTest, VerifyReplaceOptionWorksOffFinalName_NotZipFileName) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2000-01-01")); */
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp/x"); */
//
// /* 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/x"); */
//
// /* 	std::this_thread::sleep_for(std::chrono::seconds{1}); */
//
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp/x"); */
//
// /* 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/x"); */
//
// /* 	ASSERT_THAT(x1 == x2, Eq(true)); */
// /* } */
//
// /* TEST_F(QuarterlyUnitTest, VerifyReplaceOptionWorksOffFinalNameAndDoesDoReplaceWhenSpecified) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2000-01-01")); */
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp/x"); */
//
// /* 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/x"); */
//
// /* 	std::this_thread::sleep_for(std::chrono::seconds{1}); */
//
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp/x", true); */
//
// /* 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/x"); */
//
// /* 	ASSERT_THAT(x1 == x2, Eq(false)); */
// /* } */
//
//
//
class QuarterlyRetrieveMultipleFiles : public Test
{
public:
	HTTPS_Downloader a_server{"https://localhost:8443"};
	QuarterlyIndexFileRetriever idxFileRet{a_server, "/edgar/full-index", *THE_LOGGER};
};

 TEST_F(QuarterlyRetrieveMultipleFiles, VerifyRejectsFutureDates)
 {
 	ASSERT_THROW(idxFileRet.MakeIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-13"), bg::from_simple_string("2018-10-18")),
 			   Poco::AssertionViolationException);
 }

TEST_F(QuarterlyRetrieveMultipleFiles, VerifyFindsCorrectNumberOfIndexFilesInRange)
{
	decltype(auto) file_list = idxFileRet.MakeIndexFileNamesForDateRange(bg::from_simple_string("2013-Sep-10"), bg::from_simple_string("2014-10-21"));
	ASSERT_THAT(file_list.size(), Eq(5));
}

// TEST_F(QuarterlyRetrieveMultipleFiles, VerifyThrowsWhenNoIndexFilesInRange)
// {
// 	ASSERT_THROW(idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2012-Oct-10"), bg::from_simple_string("2012-10-21")),
// 			std::range_error);
// }
//
 TEST_F(QuarterlyRetrieveMultipleFiles, VerifyDownloadsCorrectNumberOfIndexFilesForDateRange)
 {
    if (fs::exists("/tmp/downloaded_q"))
	   fs::remove_all("/tmp/downloaded_q");

 	decltype(auto) file_list = idxFileRet.MakeIndexFileNamesForDateRange(bg::from_simple_string("2013-Sep-10"), bg::from_simple_string("2014-10-21"));
 	idxFileRet.HierarchicalCopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_q");

 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/downloaded_q"), Eq(file_list.size()));
 }

 TEST_F(QuarterlyRetrieveMultipleFiles, VerifyDownloadOfIndexFilesForDateRangeDoesNotReplaceWhenReplaceNotSpecified)
 {
    if (fs::exists("/tmp/downloaded_q"))
	   fs::remove_all("/tmp/downloaded_q");

 	[[maybe_unused]] decltype(auto) file_list = idxFileRet.MakeIndexFileNamesForDateRange(bg::from_simple_string("2013-Sep-10"), bg::from_simple_string("2014-10-21"));
 	idxFileRet.HierarchicalCopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_q");
 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/downloaded_q");

 	std::this_thread::sleep_for(std::chrono::seconds{1});

 	idxFileRet.HierarchicalCopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_q");
 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/downloaded_q");

 	ASSERT_THAT(x1 == x2, Eq(true));
 }

 TEST_F(QuarterlyRetrieveMultipleFiles, VerifyDownloadOfIndexFilesForDateRangeDoesReplaceWhenReplaceIsSpecified)
 {
    if (fs::exists("/tmp/downloaded_q"))
	   fs::remove_all("/tmp/downloaded_q");

 	[[maybe_unused]] decltype(auto) file_list = idxFileRet.MakeIndexFileNamesForDateRange(bg::from_simple_string("2013-Sep-10"), bg::from_simple_string("2014-10-21"));
 	idxFileRet.HierarchicalCopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_q");
 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/downloaded_q");

 	std::this_thread::sleep_for(std::chrono::seconds{1});

 	idxFileRet.HierarchicalCopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_q", true);
 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/downloaded_q");

 	ASSERT_THAT(x1 == x2, Eq(false));
 }

class QuarterlyParserUnitTest : public Test
{
public:
	HTTPS_Downloader a_server{"https://localhost:8443"};
	QuarterlyIndexFileRetriever idxFileRet{a_server, "/edgar/full-index", *THE_LOGGER};
};


TEST_F(QuarterlyParserUnitTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFile)
{
	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2002-01-01"));
	auto local_quarterly_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{a_server, *THE_LOGGER};
	std::vector<std::string> forms_list{"10-Q"};
	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name);

    // for (const auto& e : file_list["10-Q"])
    //     std::cout << e << std::endl;

	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(1920));

}

// /* TEST_F(QuarterlyParserUnitTest, VerifyDownloadOfFormFilesListedInQuarterlyIndexFile) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2009-10-10")); */
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp"); */
// /* 	decltype(auto) local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath(); */
//
// /* 	FormFileRetriever form_file_getter{a_server}; */
// /* 	std::vector<std::string> forms_list{"10-Q"}; */
// /* 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name); */
//
// /* 	//	since there are thousands of form files listed in the quarterly index, */
// /* 	//	we will only download a small number to verify functionality */
//
// /* 	if (file_list.begin()->second.size() > 10) */
// /* 		file_list.begin()->second.resize(10); */
//
// /* 	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit4"); */
// /* 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms_unit4"), Eq(CountTotalFormsFilesFound(file_list))); */
// /* } */
//
//
// /* TEST_F(QuarterlyParserUnitTest, VerifyDownloadOfFormFilesDoesNotReplaceWhenReplaceNotSpecified) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2009-10-10")); */
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp"); */
// /* 	decltype(auto) local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath(); */
//
// /* 	FormFileRetriever form_file_getter{a_server}; */
// /* 	std::vector<std::string> forms_list{"10-Q"}; */
// /* 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name); */
//
// /* 	//	since there are thousands of form files listed in the quarterly index, */
// /* 	//	we will only download a small number to verify functionality */
//
// /* 	if (file_list.begin()->second.size() > 10) */
// /* 		file_list.begin()->second.resize(10); */
//
// /* 	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit5"); */
// /* 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit5"); */
//
// /* 	std::this_thread::sleep_for(std::chrono::seconds{1}); */
//
// /* 	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit5"); */
// /* 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit5"); */
//
// /* 	ASSERT_THAT(x1 == x2, Eq(true)); */
// /* } */
//
class TickerLookupUnitTest : public Test
{
public:

};

TEST_F(TickerLookupUnitTest, VerifyConvertsSingleTickerThatExistsToCIK)
{
	TickerConverter sym{*THE_LOGGER};
	decltype(auto) CIK = sym.ConvertTickerToCIK("AAPL");

	ASSERT_THAT(CIK == "0000320193", Eq(true));

}

 TEST_F(TickerLookupUnitTest, DISABLED_VerifyConvertsFileOfTickersToCIKs)
 {
	TickerConverter sym{*THE_LOGGER};
 	decltype(auto) CIKs = sym.ConvertTickerFileToCIKs("./test_tickers_file", 1);

 	ASSERT_THAT(CIKs, Eq(50));

 }

TEST_F(TickerLookupUnitTest, VerifyFailsToConvertsSingleTickerThatDoesNotExistToCIK)
{
	TickerConverter sym{*THE_LOGGER};
	decltype(auto) CIK = sym.ConvertTickerToCIK("DHS");

	ASSERT_THAT(CIK == "**no_CIK_found**", Eq(true));

}

// /* class TickerConverterStub : public TickerConverter */
// /* { */
// /* 	public: */
// /* 		MOCK_METHOD1(EDGAR_CIK_Lookup, std::string(const std::string&)); */
// /* }; */
//
// /* //	NOTE: Google Mock needs mocked methods to be virtual !!  Disable this test */
// /* //			when method is de-virtualized... */
// /* // */
// /* TEST_F(TickerLookupUnitTest, DISABLED_VerifyConvertsSingleTickerThatExistsToCIKUsesCache) */
// /* { */
// /* 	TickerConverterStub sym_stub; */
// /* 	std::string apple{"AAPL"}; */
// /* 	EXPECT_CALL(sym_stub, EDGAR_CIK_Lookup(apple)) */
// /* 		.WillOnce(Return("0000320193")); */
//
// /* 	decltype(auto) CIK = sym_stub.ConvertTickerToCIK(apple); */
// /* 	decltype(auto) CIK2 = sym_stub.ConvertTickerToCIK(apple); */
//
// /* 	ASSERT_THAT(CIK == CIK2 && CIK == "0000320193", Eq(true)); */
//
// /* } */
//
// /* TEST_F(TickerLookupUnitTest, DISABLED_VerifyConvertsSingleTickerDoesNotUseCacheForNewTicker) */
// /* { */
// /* 	TickerConverterStub sym_stub; */
// /* 	std::string apple{"AAPL"}; */
// /* 	EXPECT_CALL(sym_stub, EDGAR_CIK_Lookup(_)) */
// /* 		.Times(2) */
// /* 		.WillOnce(Return("0000320193")) */
// /* 		.WillOnce(Return("")); */
//
// /* 	decltype(auto) CIK = sym_stub.ConvertTickerToCIK(apple); */
// /* 	decltype(auto) CIK2 = sym_stub.ConvertTickerToCIK("DHS"); */
// /* 	decltype(auto) CIK3 = sym_stub.ConvertTickerToCIK(apple); */
// /* 	decltype(auto) CIK4 = sym_stub.ConvertTickerToCIK("DHS"); */
//
// /* 	ASSERT_THAT(CIK != CIK2, Eq(true)); */
//
// /* } */
//
// /* TEST_F(TickerLookupUnitTest, VerifyWritesTickerToFile) */
// /* { */
// /* 	TickerConverter sym; */
// /* 	decltype(auto) CIK = sym.ConvertTickerToCIK("AAPL"); */
// /* 	decltype(auto) CIK2 = sym.ConvertTickerToCIK("DHS"); */
// /* 	decltype(auto) CIK3 = sym.ConvertTickerToCIK("GOOG"); */
// /* 	sym.UseCacheFile("/tmp/EDGAR_Ticker_CIK.txt"); */
// /* 	sym.SaveCIKDataToFile(); */
//
// /* 	ASSERT_THAT(fs::file_size("/tmp/EDGAR_Ticker_CIK.txt"), Gt(0)); */
//
// /* } */
//
//
// class QuarterlyParserFilterTest : public Test
// {
// public:
// 	//FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
// 	FTP_Server a_server{"ftp.sec.gov", "anonymous", "aaa@bbb.net"};
// 	QuarterlyIndexFileRetriever idxFileRet{a_server, *THE_LOGGER};
// };
//
// TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileForTicker)
// {
// 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2009-09-10"));
// 	idxFileRet.CopyRemoteIndexFileTo("/tmp");
// 	decltype(auto) local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath();
//
// 	TickerConverter sym{*THE_LOGGER};
// 	decltype(auto) CIK = sym.ConvertTickerToCIK("AAPL");
// 	std::map<std::string, std::string> ticker_map;
// 	ticker_map["AAPL"] = CIK;
//
// 	FormFileRetriever form_file_getter{a_server, *THE_LOGGER};
// 	std::vector<std::string> forms_list{"10-Q"};
// 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name, ticker_map);
// 	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(1));
//
// }
//
// /* TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileDateRangeForTicker) */
// /* { */
// /* 	idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2009-Sep-10"), bg::from_simple_string("2010-10-21")); */
// /* 	idxFileRet.CopyIndexFilesForDateRangeTo("/tmp/downloaded_s"); */
// /* 	decltype(auto) index_file_list = idxFileRet.GetLocalIndexFileNamesForDateRange(); */
//
// /* 	TickerConverter sym; */
// /* 	decltype(auto) CIK = sym.ConvertTickerToCIK("AAPL"); */
// /* 	std::map<std::string, std::string> ticker_map; */
// /* 	ticker_map["AAPL"] = CIK; */
//
// /* 	FormFileRetriever form_file_getter{a_server}; */
// /* 	std::vector<std::string> forms_list{"10-Q"}; */
// /* 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, "/tmp/downloaded_s", index_file_list, ticker_map); */
// /* 	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(4));		//	no 10-Q in 1 quarter. */
//
// /* } */
//
// class MultipleFormsParserUnitTest : public Test
// {
// public:
// 	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
// 	/* FTP_Server a_server{"ftp.sec.gov", "anonymous", "aaa@bbb.net"}; */
// 	DailyIndexFileRetriever idxFileRet{a_server, *THE_LOGGER};
// };
//
// //	The following block of tests relate to working with the Index file located above.
//
// TEST_F(MultipleFormsParserUnitTest, VerifyFindProperNumberOfFormEntriesInIndexFile)
// {
// 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
// 	idxFileRet.CopyRemoteIndexFileTo("/tmp");
// 	decltype(auto) local_daily_index_file_name = idxFileRet.GetLocalIndexFilePath();
//
// 	FormFileRetriever form_file_getter{a_server, *THE_LOGGER};
// 	std::vector<std::string> forms_list{"4", "10-K", "10-Q"};
//
// 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);
//
// 	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(520));
//
// }
//
// TEST_F(MultipleFormsParserUnitTest, VerifyFindProperNumberOfFormEntriesInIndexFileForDateRange)
// {
// 	idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-14"), bg::from_simple_string("2013-10-20"));
// 	idxFileRet.CopyIndexFilesForDateRangeTo("/tmp/daily_index1");
// 	decltype(auto) index_file_list = idxFileRet.GetfRemoteIndexFileNamesForDateRange();
//
// 	FormFileRetriever form_file_getter{a_server, *THE_LOGGER};
// 	std::vector<std::string> forms_list{"4", "10-K", "10-Q"};
//
// 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, "/tmp/daily_index1", index_file_list);
//
// 	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(4395));
//
// }
//
// /* TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileForMultipleTickers) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(bg::from_simple_string("2009-09-10")); */
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp"); */
// /* 	decltype(auto) local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath(); */
//
// /* 	TickerConverter sym; */
// /* 	decltype(auto) CIK1 = sym.ConvertTickerToCIK("AAPL"); */
// /* 	decltype(auto) CIK2 = sym.ConvertTickerToCIK("DHS"); */
// /* 	decltype(auto) CIK3 = sym.ConvertTickerToCIK("GOOG"); */
// /* 	std::map<std::string, std::string> ticker_map; */
// /* 	ticker_map["AAPL"] = CIK1; */
// /* 	ticker_map["DHS"] = CIK2; */
// /* 	ticker_map["GOOG"] = CIK3; */
//
// /* 	FormFileRetriever form_file_getter{a_server}; */
// /* 	std::vector<std::string> forms_list{"10-Q"}; */
// /* 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name, ticker_map); */
// /* 	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(2)); */
//
// /* } */
//
// /* TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileDateRangeForMultipleTickers) */
// /* { */
// /* 	idxFileRet.FindRemoteIndexFileNamesForDateRange(bg::from_simple_string("2009-Sep-10"), bg::from_simple_string("2010-10-21")); */
// /* 	idxFileRet.CopyIndexFilesForDateRangeTo("/tmp/downloaded_t"); */
// /* 	decltype(auto) index_file_list = idxFileRet.GetLocalIndexFileNamesForDateRange(); */
//
// /* 	TickerConverter sym; */
// /* 	decltype(auto) CIK1 = sym.ConvertTickerToCIK("AAPL"); */
// /* 	decltype(auto) CIK2 = sym.ConvertTickerToCIK("DHS"); */
// /* 	decltype(auto) CIK3 = sym.ConvertTickerToCIK("GOOG"); */
// /* 	std::map<std::string, std::string> ticker_map; */
// /* 	ticker_map["AAPL"] = CIK1; */
// /* 	ticker_map["DHS"] = CIK2; */
// /* 	ticker_map["GOOG"] = CIK3; */
//
// /* 	FormFileRetriever form_file_getter{a_server}; */
// /* 	std::vector<std::string> forms_list{"10-Q"}; */
// /* 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, "/tmp/downloaded_t", index_file_list, ticker_map); */
// /* 	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(8));		//	no 10-Q in 1 quarter. */
//
// /* } */


int main(int argc, char** argv)
{
	G_ARGC = argc;
	G_ARGV = argv;

    int result = 0;

	EDGAR_UnitTest the_app;
	try
	{
		the_app.init(argc, argv);

        THE_LOGGER = &Poco::Logger::get("TestLogger");
        AutoPtr<Poco::Channel> pChannel(new Poco::ConsoleChannel);
        // pChannel->setProperty("path", "/tmp/Testing.log");
        THE_LOGGER->setChannel(pChannel);
        THE_LOGGER->setLevel(Poco::Message::PRIO_DEBUG);

    	result = the_app.run();
	}
	catch (Poco::Exception& exc)
	{
		the_app.logger().log(exc);
		result =  Application::EXIT_CONFIG;
	}

    return result;
}
