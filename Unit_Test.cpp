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

	/* This file is part of Collector. */

	/* Collector is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* Collector is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with Collector.  If not, see <http://www.gnu.org/licenses/>. */


// =====================================================================================
//        Class:
//  Description:
// =====================================================================================


#include <chrono>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>

#include <boost/algorithm/string/predicate.hpp>
#include <gmock/gmock.h>

#include "DailyIndexFileRetriever.h"
#include "FormFileRetriever.h"
#include "HTTPS_Downloader.h"
#include "PathNameGenerator.h"
#include "QuarterlyIndexFileRetriever.h"
#include "TickerConverter.h"

#include "Collector_Utils.h"

namespace fs = std::filesystem;

using namespace testing;

//  some utility routines used to check test output.

MATCHER_P(StringEndsWith, value, std::string(negation ? "doesn't" : "does") +
                        " end with " + value)
{
	return boost::algorithm::ends_with(arg, value);
}

int CountFilesInDirectoryTree(const fs::path& directory)
{
	int count = std::count_if(fs::recursive_directory_iterator(directory), fs::recursive_directory_iterator(),
			[](const fs::directory_entry& entry) { return entry.status().type() == fs::file_type::regular; });
	return count;
}

std::map<std::string, fs::file_time_type> CollectLastModifiedTimesForFilesInDirectory(const fs::path& directory)
{
	std::map<std::string, fs::file_time_type> results;

    auto save_mod_time([&results] (const auto& dir_ent)
    {
		if (dir_ent.status().type() == fs::file_type::regular)
			results[dir_ent.path().filename().string()] = fs::last_write_time(dir_ent.path());
    });

    std::for_each(fs::directory_iterator(directory), fs::directory_iterator(), save_mod_time);

	return results;
}

std::map<std::string, fs::file_time_type> CollectLastModifiedTimesForFilesInDirectoryTree(const fs::path& directory)
{
	std::map<std::string, fs::file_time_type> results;

    auto save_mod_time([&results] (const auto& dir_ent)
    {
		if (dir_ent.status().type() == fs::file_type::regular)
			results[dir_ent.path().filename().string()] = fs::last_write_time(dir_ent.path());
    });

    std::for_each(fs::recursive_directory_iterator(directory), fs::recursive_directory_iterator(), save_mod_time);

	return results;
}

int CountTotalFormsFilesFound(const FormFileRetriever::FormsAndFilesList& file_list)
{
	int grand_total{0};
    // grand_total = std::accumulate(std::begin(file_list), std::end(file_list), 0, [] (auto a, const auto& b) { return a + b.second.size(); });
	for (const auto& [form_type, form_list] : file_list)
		grand_total += form_list.size();

	return grand_total;
}

// NOTE: for some of these tests, I run an HTTPS server on localhost using
// a directory structure that mimics part of the SEC server.

class PathNameGenerator_UnitTest : public Test
{


};

TEST_F(PathNameGenerator_UnitTest, TestSameStartEndDates)
{
	DateRange date_range{StringToDateYMD("%Y-%b-%d", "2013-Oct-13"), StringToDateYMD("%Y-%b-%d", "2013-Oct-13")};
	int count = 0;
	for ([[maybe_unused]] auto a_date : date_range)
		++count;

	ASSERT_THAT(count, Eq(1));
}

TEST_F(PathNameGenerator_UnitTest, TestSameStartEndDatesQuarterBegin)
{
	DateRange range{StringToDateYMD("%F", "2014-01-01"), StringToDateYMD("%F", "2014-01-01")};
	int count{0};

	for ([[maybe_unused]] auto a_date : range)
		++count;

	int count2{0};

	for (auto quarter_begin = std::begin(range); quarter_begin < std::end(range); ++quarter_begin)
		++count2;

	ASSERT_TRUE(count == 1 && count2 == 1);
}

TEST_F(PathNameGenerator_UnitTest, TestSameStartEndDatesQuarterEnd)
{
	DateRange range{StringToDateYMD("%F", "2014-12-31"), StringToDateYMD("%F", "2014-12-31")};
	int count{0};

	for ([[maybe_unused]] auto a_date : range)
		++count;

	int count2{0};

	for (auto quarter_begin = std::begin(range); quarter_begin < std::end(range); ++quarter_begin)
		++count2;

	ASSERT_TRUE(count == 1 && count2 == 1);
}

TEST_F(PathNameGenerator_UnitTest, TestStartEndDatesQuarterBeginEnd)
{
	DateRange range{StringToDateYMD("%F", "2014-1-1"), StringToDateYMD("%F", "2014-3-31")};
	int count{0};

	for ([[maybe_unused]] auto a_date : range)
		++count;

	int count2{0};

	for (auto quarter_begin = std::begin(range); quarter_begin < std::end(range); ++quarter_begin)
		++count2;

	ASSERT_TRUE(count == 1 && count2 == 1);
}

TEST_F(PathNameGenerator_UnitTest, TestStartEndDatesYearBeginEnd)
{
	DateRange range{StringToDateYMD("%F", "2014-1-1"), StringToDateYMD("%F", "2015-1-1")};
	int count{0};

	for ([[maybe_unused]] auto a_date : range)
		++count;

	int count2{0};

	for (auto quarter_begin = std::begin(range); quarter_begin < std::end(range); ++quarter_begin)
		++count2;

	ASSERT_TRUE(count == 5 && count2 == 5);
}

TEST_F(PathNameGenerator_UnitTest, TestStartYear1EndYear2)
{
	int count{0};

	for ([[maybe_unused]] const auto& quarter : DateRange{StringToDateYMD("%F", "2014-7-1"), StringToDateYMD("%F", "2015-6-30")})
		++count;

	int count2{0};

	for ([[maybe_unused]] const auto& quarter : DateRange{StringToDateYMD("%F", "2014-7-1"), StringToDateYMD("%F", "2015-7-1")})
		++count2;

	ASSERT_TRUE(count == 4 && count2 == 5);
}

TEST_F(PathNameGenerator_UnitTest, TestArbitraryStartEnd)
{
	int count{0};

	for ([[maybe_unused]] const auto& quarter : DateRange{StringToDateYMD("%F", "2013-12-20"), StringToDateYMD("%F", "2014-5-21")})
		++count;

	ASSERT_THAT(count, Eq(3));
}

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
	HTTPS_Downloader a_server{"xxxlocalhost", 8443};
	ASSERT_THROW(a_server.RetrieveDataFromServer(""), std::runtime_error);
}

TEST_F(HTTPS_UnitTest, TestAbilityToConnectToHTTPSServer)
{
	HTTPS_Downloader a_server{"localhost", 8443};
	// ASSERT_NO_THROW(a_server.OpenHTTPSConnection());
	std::string data = a_server.RetrieveDataFromServer("/Archives/test.txt");
    std::cout << "data: " << data << '\n';
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
	HTTPS_Downloader a_server{"localhost", 8443};
	decltype(auto) directory_list = a_server.ListDirectoryContents("/Archives/edgar/full-index/2013/QTR4");
	ASSERT_TRUE(std::find(directory_list.begin(), directory_list.end(), "company.gz") != directory_list.end());
}

TEST_F(HTTPS_UnitTest, VerifyAbilityToDownloadFileWhichExists)
{
	if (fs::exists("/tmp/form.20131010.idx"))
		fs::remove("/tmp/form.20131010.idx");

	HTTPS_Downloader a_server{"localhost", 8443};
	a_server.DownloadFile("/Archives/edgar/daily-index/2013/QTR4/form.20131010.idx.gz", "/tmp/form.20131010.idx");
	ASSERT_TRUE(fs::exists("/tmp/form.20131010.idx"));
}

TEST_F(HTTPS_UnitTest, VerifyThrowsExceptionWhenTryToDownloadFileDoesntExist)
{
	if (fs::exists("/tmp/form.20131008.idx"))
		fs::remove("/tmp/form.20131008.idx");
	HTTPS_Downloader a_server{"localhost", 8443};
	ASSERT_THROW(a_server.DownloadFile("/Archives/edgar/daily-index/2013/QTR4/form.20131008.idx", "/tmp/form.20131008.idx"), std::runtime_error);
}

TEST_F(HTTPS_UnitTest, VerifyExceptionWhenDownloadingToFullDisk)
{
	HTTPS_Downloader a_server{"localhost", 8443};
	ASSERT_THROW(a_server.DownloadFile("/Archives/edgar/daily-index/2013/QTR4/form.20131015.idx", "/tmp/ofstream_test/form.20131015.idx"), std::system_error);
}

TEST_F(HTTPS_UnitTest, VerifyExceptionWhenDownloadingGZFileToFullDisk)
{
	HTTPS_Downloader a_server{"localhost", 8443};
	ASSERT_THROW(a_server.DownloadFile("/Archives/edgar/daily-index/2013/QTR4/form.20131015.idx.gz", "/tmp/ofstream_test/form.20131015.idx"), std::system_error);
}

TEST_F(HTTPS_UnitTest, VerifyExceptionWhenDownloadingZipFileToFullDisk)
{
	HTTPS_Downloader a_server{"localhost", 8443};
	ASSERT_THROW(a_server.DownloadFile("/Archives/edgar/full-index/2013/QTR4/form.zip", "/tmp/ofstream_test/form.idx"), std::system_error);
//	a_server.DownloadFile("/Archives/edgar/full-index/2013/QTR4/form.zip", "/tmp/ofstream_test/form.idx");
}

class RetrieverUnitTest : public Test
{
public:

	DailyIndexFileRetriever idxFileRet{"localhost", 8443, "/Archives/edgar/daily-index"};
};

TEST_F(RetrieverUnitTest, VerifyRejectsInvalidDates)
{
	ASSERT_THROW(idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%Y-%b-%d", "2013-Oxt-13")), Collector::AssertionException);
}

TEST_F(RetrieverUnitTest, VerifyRejectsFutureDates)
{
	// let's make a date in the future

	auto today = floor<date::days>(std::chrono::system_clock::now());
	auto tomorrow = today + date::days(1);

	ASSERT_THROW(idxFileRet.FindRemoteIndexFileNameNearestDate(tomorrow), Collector::AssertionException);
}

//	Finally, we get to the point

TEST_F(RetrieverUnitTest, TestFindIndexFileDateNearestWhereDateExists)
{
	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-11"));
	ASSERT_THAT(idxFileRet.GetActualIndexFileDate() == StringToDateYMD("%F", "2013-10-11"), Eq(true));
}

TEST_F(RetrieverUnitTest, TestFindIndexFileDateNearestWhereDateDoesNotExist)
{
	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-14"));
	ASSERT_THAT(idxFileRet.GetActualIndexFileDate() == StringToDateYMD("%F", "2013-10-11"), Eq(true));
}

// TEST_F(RetrieverUnitTest, TestExceptionThrownWhenRemoteIndexFileNameUnknown)
// {
// 	ASSERT_THROW(idxFileRet.CopyRemoteIndexFileTo("/tmp"), Collector::AssertionException);
// }
//
 TEST_F(RetrieverUnitTest, TestRetrieveIndexFileWhereDateExists)
 {
	if (fs::exists("/tmp/form.20131011.idx"))
		fs::remove("/tmp/form.20131011.idx");

 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-11"));
 	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

 	ASSERT_THAT(fs::exists(local_daily_index_file_name), Eq(true));
 }

 TEST_F(RetrieverUnitTest, TestHierarchicalRetrieveIndexFileWhereDateExists)
 {
	if (fs::exists("/tmp/2013/QTR4/form.20131011.idx"))
		fs::remove("/tmp/2013/QTR4/form.20131011.idx");

 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-11"));
 	auto local_daily_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp");

 	ASSERT_THAT(fs::exists(local_daily_index_file_name), Eq(true));
 }

TEST_F(RetrieverUnitTest, TestRetrieveIndexFileDoesNotReplaceWhenReplaceNotSpecified)
{
	if (fs::exists("/tmp/form.20131011.idx"))
		fs::remove("/tmp/form.20131011.idx");

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-11"));
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

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-11"));
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

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-11"));
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

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-11"));
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
	DailyIndexFileRetriever idxFileRet{"localhost", 8443, "/Archives/edgar/daily-index"};
};

//	The following block of tests relate to working with the Index file located above.

TEST_F(ParserUnitTest, VerifyFindProperNumberOfFormEntriesInIndexFile)
{
	if (fs::exists("/tmp/form.20131010.idx"))
		fs::remove("/tmp/form.20131010.idx");

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-10"));
	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{"localhost", 8443};
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

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-10"));
	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{"localhost", 8443};
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

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-10"));
	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{"localhost", 8443};
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

 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-10"));
 	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{"localhost", 8443};
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

 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-10"));
 	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{"localhost", 8443};
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

 	[[maybe_unused]] decltype(auto) results = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-10"), StringToDateYMD("%F", "2013-10-21"));
 	auto local_index_files = idxFileRet.CopyIndexFilesForDateRangeTo(results, "/tmp/downloaded_p");
 // 	decltype(auto) index_file_list = idxFileRet.GetfRemoteIndexFileNamesForDateRange();

    FormFileRetriever form_file_getter{"localhost", 8443};
 	std::vector<std::string> forms_list{"10-Q"};
 	decltype(auto) form_file_list = form_file_getter.FindFilesForForms(forms_list, local_index_files);

 	ASSERT_THAT(CountTotalFormsFilesFound(form_file_list), Eq(255));
 }


 class RetrieverMultipleDailies : public Test
 {
 public:
	HTTPS_Downloader a_server{"localhost", 8443};
	DailyIndexFileRetriever idxFileRet{"localhost", 8443, "/Archives/edgar/daily-index"};
 };

 TEST_F(RetrieverMultipleDailies, VerifyRejectsFutureDates)
 {
	// let's make a date in the future

	auto today = floor<date::days>(std::chrono::system_clock::now());
	auto tomorrow = today + date::days(1);

 	ASSERT_THROW(idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-13"), tomorrow),
 			   Collector::AssertionException);
 }

 TEST_F(RetrieverMultipleDailies, VerifyFindsCorrectDateRangeWhenBoundingDatesNotFound)
 {
 	idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-14"), StringToDateYMD("%F", "2013-10-20"));
 	decltype(auto) results = idxFileRet.GetActualDateRange();

 	ASSERT_THAT(results, Eq(std::pair(StringToDateYMD("%Y-%b-%d", "2013-Oct-15"), StringToDateYMD("%F", "2013-10-18"))));
 }

 TEST_F(RetrieverMultipleDailies, VerifyFindsCorrectNumberOfIndexFilesInRange)
 {
 	decltype(auto) results = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-10"), StringToDateYMD("%F", "2013-10-21"));
 	ASSERT_THAT(results.size(), Eq(7));
 }

 TEST_F(RetrieverMultipleDailies, VerifyFindsCorrectNumberOfIndexFilesInMultiQuarterRange)
 {
 	decltype(auto) results = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Dec-20"), StringToDateYMD("%F", "2014-05-21"));
 	ASSERT_THAT(results.size(), Eq(98));
 }

 TEST_F(RetrieverMultipleDailies, VerifyThrowsWhenNoIndexFilesInRange)
 {
 	ASSERT_THROW(idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2012-Oct-10"), StringToDateYMD("%F", "2012-10-21")),
 			Collector::AssertionException);
 }

 TEST_F(RetrieverMultipleDailies, VerifyDownloadsCorrectNumberOfIndexFilesForDateRange)
 {
    if (fs::exists("/tmp/downloaded_l"))
	   fs::remove_all("/tmp/downloaded_l");

 	decltype(auto) file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-10"), StringToDateYMD("%F", "2013-10-21"));
 	idxFileRet.CopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l");

 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/downloaded_l"), Eq(file_list.size()));
 }

 TEST_F(RetrieverMultipleDailies, VerifyDownloadOfIndexFilesForDateRangeDoesNotReplaceWhenReplaceNotSpecified)
 {
    if (fs::exists("/tmp/downloaded_l"))
	   fs::remove_all("/tmp/downloaded_l");

 	[[maybe_unused]] decltype(auto) file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-10"), StringToDateYMD("%F", "2013-10-21"));
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

 	[[maybe_unused]] decltype(auto) file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-10"), StringToDateYMD("%F", "2013-10-21"));
 	idxFileRet.CopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l");
 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_l");

 	std::this_thread::sleep_for(std::chrono::seconds{1});

 	idxFileRet.CopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l", true);
 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_l");

 	ASSERT_THAT(x1 == x2, Eq(false));
 }


 class ConcurrentlyRetrieveMultipleDailies : public Test
 {
 public:
	DailyIndexFileRetriever idxFileRet{"localhost", 8443, "/Archives/edgar/daily-index"};
 };

 TEST_F(ConcurrentlyRetrieveMultipleDailies, VerifyDownloadsCorrectNumberOfIndexFilesForDateRange)
 {
    if (fs::exists("/tmp/downloaded_l"))
	   fs::remove_all("/tmp/downloaded_l");

 	decltype(auto) file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-10"), StringToDateYMD("%F", "2013-10-21"));
 	idxFileRet.ConcurrentlyCopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l", 4);

 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/downloaded_l"), Eq(file_list.size()));
 }

 TEST_F(ConcurrentlyRetrieveMultipleDailies, VerifyDownloadOfIndexFilesForDateRangeDoesNotReplaceWhenReplaceNotSpecified)
 {
    if (fs::exists("/tmp/downloaded_l"))
	   fs::remove_all("/tmp/downloaded_l");

 	[[maybe_unused]] decltype(auto) file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-10"), StringToDateYMD("%F", "2013-10-21"));
 	idxFileRet.ConcurrentlyCopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l", 4);
 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_l");

 	std::this_thread::sleep_for(std::chrono::seconds{1});

 	idxFileRet.ConcurrentlyCopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_l", 4);
 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_l");

 	ASSERT_THAT(x1 == x2, Eq(true));
 }

TEST_F(ConcurrentlyRetrieveMultipleDailies, VerifyDownloadOfFormFilesListedInIndexFile)
{
	if (fs::exists("/tmp/form.20131010.idx"))
		fs::remove("/tmp/form.20131010.idx");

    if (fs::exists("/tmp/forms_unit"))
	   fs::remove_all("/tmp/forms_unit");

	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-10"));
	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{"localhost", 8443};
	std::vector<std::string> forms_list{"10-Q"};
	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);

	form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(file_list, "/tmp/forms_unit", 10);
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms_unit"), Eq(CountTotalFormsFilesFound(file_list)));
}

 TEST_F(ConcurrentlyRetrieveMultipleDailies, VerifyDownloadOfFormFilesDoesReplaceWhenReplaceIsSpecified)
 {
	if (fs::exists("/tmp/form.20131010.idx"))
		fs::remove("/tmp/form.20131010.idx");

    if (fs::exists("/tmp/forms_unit3"))
	   fs::remove_all("/tmp/forms_unit3");

 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-10"));
 	auto local_daily_index_file_name = idxFileRet.CopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{"localhost", 8443};
 	std::vector<std::string> forms_list{"10-Q"};
 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);

 	form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(file_list, "/tmp/forms_unit3", 20);
 	decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit3");

 	std::this_thread::sleep_for(std::chrono::seconds{1});


 	form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(file_list, "/tmp/forms_unit3", 20, true);
 	decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit3");

 	ASSERT_THAT(x1 == x2, Eq(false));
 }

 TEST_F(ConcurrentlyRetrieveMultipleDailies, VerifyConcurrentDownloadDoesntMessUpFiles)
 {
	// first we download some index files and a bunch of form files using the
	// single-thread methods.

    if (fs::exists("/tmp/index2"))
	   fs::remove_all("/tmp/index2");

    if (fs::exists("/tmp/forms_unit2"))
	   fs::remove_all("/tmp/forms_unit2");

 	decltype(auto) remote_index_files2 = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-14"), StringToDateYMD("%F", "2013-10-17"));
 	auto index_files2 = idxFileRet.HierarchicalCopyIndexFilesForDateRangeTo(remote_index_files2, "/tmp/index2");

	FormFileRetriever form_file_getter{"localhost", 8443};
 	std::vector<std::string> forms_list2{"10-Q"};
 	decltype(auto) remote_form_file_list2 = form_file_getter.FindFilesForForms(forms_list2, index_files2);

 	form_file_getter.RetrieveSpecifiedFiles(remote_form_file_list2, "/tmp/forms_unit2");

	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms_unit2"), Eq(CountTotalFormsFilesFound(remote_form_file_list2)));

	// next we repeat the downloads but this time concurrently and to a separate set of directories.
	// then, we will compare the file heriarchies and contents to see if they match.

    if (fs::exists("/tmp/index4"))
	   fs::remove_all("/tmp/index4");

    if (fs::exists("/tmp/forms_unit4"))
	   fs::remove_all("/tmp/forms_unit4");

 	decltype(auto) remote_index_files4 = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-14"), StringToDateYMD("%F", "2013-10-17"));
 	auto index_files4 = idxFileRet.ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo(remote_index_files4, "/tmp/index4", 2);

	FormFileRetriever form_file_getter4{"localhost", 8443};
 	std::vector<std::string> forms_list4{"10-Q"};
 	decltype(auto) remote_form_file_list4 = form_file_getter4.FindFilesForForms(forms_list4, index_files4);

 	form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(remote_form_file_list4, "/tmp/forms_unit4", 10);

	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms_unit4"), Eq(CountTotalFormsFilesFound(remote_form_file_list4)));

	// this test downloads 3 index files and 129 form files so it seems like a decent test.

	// for now, I'm just doing:
	// diff -rq /tmp/index2 /tmp/index4
	// diff -rq /tmp/forms_unit2 /tmp/forms_unit4
	// and checking for zero return code
 }


class ConcurrentlyRetrieveMultipleQuarterlyFiles : public Test
{
public:
	HTTPS_Downloader a_server{"localhost", 8443};
	QuarterlyIndexFileRetriever idxFileRet{"localhost", 8443, "/Archives/edgar/full-index"};
};

 TEST_F(ConcurrentlyRetrieveMultipleQuarterlyFiles, VerifyDownloadsCorrectNumberOfIndexFilesForDateRange)
 {
    if (fs::exists("/tmp/downloaded_q"))
	   fs::remove_all("/tmp/downloaded_q");

 	decltype(auto) file_list = idxFileRet.MakeIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Sep-10"), StringToDateYMD("%F", "2014-10-21"));
 	idxFileRet.ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_q", 4);

 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/downloaded_q"), Eq(file_list.size()));
 }


class QuarterlyUnitTest : public Test
{
public:
	HTTPS_Downloader a_server{"localhost", 8443};
	QuarterlyIndexFileRetriever idxFileRet{"localhost", 8443, "/Archives/edgar/full-index"};
};

TEST_F(QuarterlyUnitTest, VerifyRejectsInvalidDates)
{
	ASSERT_THROW(idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%Y-%b-%d", "2013-Oxt-13")), Collector::AssertionException);
}

 TEST_F(QuarterlyUnitTest, VerifyRejectsFutureDates)
 {
	// let's make a date in the future

	auto today = floor<date::days>(std::chrono::system_clock::now());
	auto tomorrow = today + date::days(1);

 	ASSERT_THROW(idxFileRet.MakeQuarterlyIndexPathName(tomorrow), Collector::AssertionException);
 }

 TEST_F(QuarterlyUnitTest, TestFindIndexFileGivenFirstDayInQuarter)
 {
 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2000-01-01"));
 	ASSERT_THAT(file_name == "/Archives/edgar/full-index/2000/QTR1/form.zip", Eq(true));
 }

 TEST_F(QuarterlyUnitTest, TestFindIndexFileGivenLastDayInQuarter)
 {
 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2002-06-30"));
 	ASSERT_THAT(file_name == "/Archives/edgar/full-index/2002/QTR2/form.zip", Eq(true));
 }

TEST_F(QuarterlyUnitTest, TestFindAllQuarterlyIndexFilesForAYear)
{
	decltype(auto) file_names = idxFileRet.MakeIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2002-Jan-01"), StringToDateYMD("%Y-%b-%d", "2003-Jan-1"));
	ASSERT_THAT(file_names.size(), Eq(5));
}

TEST_F(QuarterlyUnitTest, TestDownloadQuarterlyIndexFile)
{
	if (fs::exists("/tmp/2000/QTR1/form.idx"))
		fs::remove("/tmp/2000/QTR1/form.idx");

	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2000-01-01"));
	auto local_daily_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp", true);

	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.idx"), Eq(true));
	//ASSERT_THAT(fs::exists(idxFileRet.GetLocalIndexFilePath()), Eq(true));
}

// /* TEST_F(QuarterlyUnitTest, TestDownloadQuarterlyIndexFileThenUnzipIt) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2000-01-01")); */
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp", true); */
//
// /* 	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.idx"), Eq(true)); */
// /* } */
//
// /* TEST_F(QuarterlyUnitTest, TestDownloadQuarterlyIndexFileThenUnzipItThenDeleteZipFile) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2000-01-01")); */
// /* 	idxFileRet.CopyRemoteIndexFileTo("/tmp", true); */
//
// /* 	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.idx"), Eq(true)); */
// /* 	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.zip"), Eq(false)); */
// /* } */
//
// /* TEST_F(QuarterlyUnitTest, VerifyReplaceOptionWorksOffFinalName_NotZipFileName) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2000-01-01")); */
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
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2000-01-01")); */
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
	QuarterlyIndexFileRetriever idxFileRet{"localhost", 8443, "/Archives/edgar/full-index"};
};

 TEST_F(QuarterlyRetrieveMultipleFiles, VerifyRejectsFutureDates)
 {
	// let's make a date in the future

	auto today = floor<date::days>(std::chrono::system_clock::now());
	auto tomorrow = today + date::days(1);

 	ASSERT_THROW(idxFileRet.MakeIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-13"), tomorrow),
 			   Collector::AssertionException);
 }

TEST_F(QuarterlyRetrieveMultipleFiles, VerifyFindsCorrectNumberOfIndexFilesInRange)
{
	decltype(auto) file_list = idxFileRet.MakeIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Sep-10"), StringToDateYMD("%F", "2014-10-21"));
	ASSERT_THAT(file_list.size(), Eq(6));
}

 TEST_F(QuarterlyRetrieveMultipleFiles, VerifyDownloadsCorrectNumberOfIndexFilesForDateRange)
 {
    if (fs::exists("/tmp/downloaded_q"))
	   fs::remove_all("/tmp/downloaded_q");

 	decltype(auto) file_list = idxFileRet.MakeIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Sep-10"), StringToDateYMD("%F", "2014-10-21"));
 	idxFileRet.HierarchicalCopyIndexFilesForDateRangeTo(file_list, "/tmp/downloaded_q");

 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/downloaded_q"), Eq(file_list.size()));
 }

 TEST_F(QuarterlyRetrieveMultipleFiles, VerifyDownloadOfIndexFilesForDateRangeDoesNotReplaceWhenReplaceNotSpecified)
 {
    if (fs::exists("/tmp/downloaded_q"))
	   fs::remove_all("/tmp/downloaded_q");

 	[[maybe_unused]] decltype(auto) file_list = idxFileRet.MakeIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Sep-10"), StringToDateYMD("%F", "2014-10-21"));
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

 	[[maybe_unused]] decltype(auto) file_list = idxFileRet.MakeIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Sep-10"), StringToDateYMD("%F", "2014-10-21"));
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
	QuarterlyIndexFileRetriever idxFileRet{"localhost", 8443, "/Archives/edgar/full-index"};
};


TEST_F(QuarterlyParserUnitTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFile)
{
	if (fs::exists("/tmp/2002/QTR1/form.idx"))
		fs::remove("/tmp/2002/QTR1/form.idx");

	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2002-01-01"));
	auto local_quarterly_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{"localhost", 8443};
	std::vector<std::string> forms_list{"10-Q"};
	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name);

    // for (const auto& e : file_list["10-Q"])
    //     std::cout << e << std::endl;

	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(1920));

}

 TEST_F(QuarterlyParserUnitTest, VerifyDownloadOfFormFilesListedInQuarterlyIndexFile)
 {
	if (fs::exists("/tmp/2002/QTR1/form.idx"))
		fs::remove("/tmp/2002/QTR1/form.idx");

	if (fs::exists("/tmp/forms_unit4"))
		fs::remove_all("/tmp/forms_unit4");

 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2009-10-10"));
	auto local_quarterly_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp");

	FormFileRetriever form_file_getter{"localhost", 8443};
 	std::vector<std::string> forms_list{"10-Q"};
 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name);

 	//	since there are thousands of form files listed in the quarterly index,
 	//	we will only download a small number to verify functionality

 	if (file_list.begin()->second.size() > 10)
 		file_list.begin()->second.resize(10);

 	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit4");
 	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms_unit4"), Eq(CountTotalFormsFilesFound(file_list)));
 }


// /* TEST_F(QuarterlyParserUnitTest, VerifyDownloadOfFormFilesDoesNotReplaceWhenReplaceNotSpecified) */
// /* { */
// /* 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2009-10-10")); */
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
	TickerConverter sym;
	decltype(auto) CIK = sym.ConvertTickerToCIK("AAPL");

	ASSERT_THAT(CIK == "0000320193", Eq(true));

}

 TEST_F(TickerLookupUnitTest, DISABLED_VerifyConvertsFileOfTickersToCIKs)
 {
	TickerConverter sym;
 	decltype(auto) CIKs = sym.ConvertTickerFileToCIKs("./test_tickers_file", 1);

 	ASSERT_THAT(CIKs, Eq(50));

 }

TEST_F(TickerLookupUnitTest, VerifyFailsToConvertsSingleTickerThatDoesNotExistToCIK)
{
	TickerConverter sym;
	decltype(auto) CIK = sym.ConvertTickerToCIK("DHS");

	ASSERT_THAT(CIK == "**no_CIK_found**", Eq(true));

}

// /* class TickerConverterStub : public TickerConverter */
// /* { */
// /* 	public: */
// /* 		MOCK_METHOD1(SEC_CIK_Lookup, std::string(const std::string&)); */
// /* }; */
//
// /* //	NOTE: Google Mock needs mocked methods to be virtual !!  Disable this test */
// /* //			when method is de-virtualized... */
// /* // */
// /* TEST_F(TickerLookupUnitTest, DISABLED_VerifyConvertsSingleTickerThatExistsToCIKUsesCache) */
// /* { */
// /* 	TickerConverterStub sym_stub; */
// /* 	std::string apple{"AAPL"}; */
// /* 	EXPECT_CALL(sym_stub, SEC_CIK_Lookup(apple)) */
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
// /* 	EXPECT_CALL(sym_stub, SEC_CIK_Lookup(_)) */
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
// /* 	sym.UseCacheFile("/tmp/SEC_Ticker_CIK.txt"); */
// /* 	sym.SaveCIKDataToFile(); */
//
// /* 	ASSERT_THAT(fs::file_size("/tmp/SEC_Ticker_CIK.txt"), Gt(0)); */
//
// /* } */
//
//
class QuarterlyParserFilterTest : public Test
{
public:
	QuarterlyIndexFileRetriever idxFileRet{"localhost", 8443, "/Archives/edgar/full-index"};
};

TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileForTicker)
{
	if (fs::exists("/tmp/2009/QTR3/form.idx"))
		fs::remove("/tmp/2009/QTR3/form.idx");

	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2009-09-10"));
	auto local_quarterly_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp");

	TickerConverter sym;
	decltype(auto) CIK = sym.ConvertTickerToCIK("AAPL");
	std::map<std::string, std::string> ticker_map;
	ticker_map["AAPL"] = CIK;

	FormFileRetriever form_file_getter{"localhost", 8443};
	std::vector<std::string> forms_list{"10-Q"};
	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name, ticker_map);
	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(1));

}

TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileForMultipleTickersAndForms)
{
	if (fs::exists("/tmp/2009/QTR3/form.idx"))
		fs::remove("/tmp/2009/QTR3/form.idx");

	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2013-09-10"));
	auto local_quarterly_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp");

	TickerConverter sym;
	std::map<std::string, std::string> ticker_map{
		{"AAPL", sym.ConvertTickerToCIK("AAPL")},
		{"DHS", sym.ConvertTickerToCIK("DHS")},
		{"GOOG", "1288776"},
	};

	FormFileRetriever form_file_getter{"localhost", 8443};
	std::vector<std::string> forms_list{"10-Q", "10-MQ"};
	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name, ticker_map);
	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(2));

}

// /* TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileDateRangeForTicker) */
// /* { */
// /* 	idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2009-Sep-10"), StringToDateYMD("%F", "2010-10-21")); */
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
class MultipleFormsParserUnitTest : public Test
{
public:
	DailyIndexFileRetriever idxFileRet{"localhost", 8443, "/Archives/edgar/daily-index"};
};

// //	The following block of tests relate to working with the Index file located above.
//
// TEST_F(MultipleFormsParserUnitTest, VerifyFindProperNumberOfFormEntriesInIndexFile)
// {
// 	decltype(auto) file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(StringToDateYMD("%F", "2013-10-10"));
// 	idxFileRet.CopyRemoteIndexFileTo("/tmp");
// 	decltype(auto) local_daily_index_file_name = idxFileRet.GetLocalIndexFilePath();
//
// 	FormFileRetriever form_file_getter{a_server};
// 	std::vector<std::string> forms_list{"4", "10-K", "10-Q"};
//
// 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);
//
// 	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(520));
//
// }
//
TEST_F(MultipleFormsParserUnitTest, VerifyFindProperNumberOfFormEntriesInIndexFileForDateRange)
{
	if (fs::exists("/tmp/daily_index1"))
		fs::remove_all("/tmp/daily_index1");

	auto index_file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2013-Oct-14"), StringToDateYMD("%F", "2013-10-20"));
	auto local_index_files = idxFileRet.CopyIndexFilesForDateRangeTo(index_file_list, "/tmp/daily_index1");

	FormFileRetriever form_file_getter{"localhost", 8443};
	std::vector<std::string> forms_list{"4", "10-K", "10-Q", "10-MQ"};

	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_index_files);

	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(4395));
}

 // TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileForMultipleTickers)
 // {
 // 	decltype(auto) file_name = idxFileRet.MakeQuarterlyIndexPathName(StringToDateYMD("%F", "2009-09-10"));
 // auto local_quarterly_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(file_name, "/tmp");
 //
 // 	TickerConverter sym;
 // // 	decltype(auto) CIK1 = sym.ConvertTickerToCIK("AAPL");
 // // 	decltype(auto) CIK2 = sym.ConvertTickerToCIK("DHS");
 // // 	decltype(auto) CIK3 = sym.ConvertTickerToCIK("GOOG");
 // 	std::map<std::string, std::string> ticker_map{{"AAPL", sym.ConvertTickerToCIK("AAPL")}, {"DHS", sym.ConvertTickerToCIK("DHS")}, {"GOOG", sym.ConvertTickerToCIK("GOOG")}};
 // // 	ticker_map["AAPL"] = CIK1;
 // // 	ticker_map["DHS"] = CIK2;
 // // 	ticker_map["GOOG"] = CIK3;
 //
 // 	std::vector<std::string> forms_list{"10-Q"};
 // 	decltype(auto) file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name, ticker_map);
 // 	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(2));
 // }
//
// /* TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileDateRangeForMultipleTickers) */
// /* { */
// /* 	idxFileRet.FindRemoteIndexFileNamesForDateRange(StringToDateYMD("%Y-%b-%d", "2009-Sep-10"), StringToDateYMD("%F", "2010-10-21")); */
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


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  InitLogging
 *  Description:  
 * =====================================================================================
 */
void InitLogging ()
{
//    logging::core::get()->set_filter
//    (
//        logging::trivial::severity >= logging::trivial::trace
//    );
}		/* -----  end of function InitLogging  ----- */

int main(int argc, char** argv)
{

    InitLogging();

    InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

