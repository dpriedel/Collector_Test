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
#include <gmock/gmock.h>

#include <Poco/Net/NetException.h>
#include "FTP_Connection.h"
#include "DailyIndexFileRetriever.h"
#include "FormFileRetriever.h"
#include "QuarterlyIndexFileRetriever.h"
#include "TickerConverter.h"

using namespace testing;

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
	for (auto x = fs::directory_iterator(directory); x != fs::directory_iterator(); ++x)
	{
		if (x->status().type() == fs::file_type::regular_file)
			results[x->path().leaf().string()] = fs::last_write_time(x->path());
	}

	return results;
}

std::map<std::string, std::time_t> CollectLastModifiedTimesForFilesInDirectoryTree(const fs::path& directory)
{
	std::map<std::string, std::time_t> results;
	for (auto x = fs::recursive_directory_iterator(directory); x != fs::recursive_directory_iterator(); ++x)
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

class FTP_UnitTest : public Test
{


};

/* 

TEST_F(RetrieverUnitTest, VerifyRetrievesIndexFileforNearestDate)
{
	idxFileRet.UseDate(std::string{"2013-12-13"});
	auto nearest_date = idxFileRet.FindIndexFileDateNearest();
	
	idxFileRet.MakeRemoteIndexFileURI();
	idxFileRet.MakeLocalIndexFileName();
	idxFileRet.RetrieveRemoteIndexFile();

	ASSERT_THAT(fs::exists(idxFileRet.GetLocalIndexFileName()), Eq(true));

}
*/

TEST_F(FTP_UnitTest, TestExceptionOnFailureToConnectToFTPServer)
{
	FTP_Server a_server{"localxxxx", "anonymous", "aaa@bbb.net"};
	ASSERT_THROW(a_server.OpenFTPConnection(), Poco::Net::NetException);
}

TEST_F(FTP_UnitTest, TestAbilityToConnectToFTPServer)
{
	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	ASSERT_NO_THROW(a_server.OpenFTPConnection());
}

TEST_F(FTP_UnitTest, TestAbilityToChangeWorkingDirectory)
{
	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	a_server.OpenFTPConnection();
	a_server.ChangeWorkingDirectoryTo("edgar/daily-index");
	ASSERT_THAT(a_server.GetWorkingDirectory().size(), Gt(0));

}

TEST_F(FTP_UnitTest, TestExceptionOnFailureToChangeWorkingDirectory)
{
	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	a_server.OpenFTPConnection();
	ASSERT_THROW(a_server.ChangeWorkingDirectoryTo("edgar/xxxxy-index"), Poco::Net::FTPException);

}

TEST_F(FTP_UnitTest, TestAbilityToListWorkingDirectory)
{
	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	a_server.OpenFTPConnection();
	a_server.ChangeWorkingDirectoryTo("edgar/daily-index");
	auto directory_list = a_server.ListWorkingDirectoryContents();
	ASSERT_THAT(directory_list.size(), Gt(0));

}

TEST_F(FTP_UnitTest, VerifyAbilityToDownloadFileWhichExists)
{
	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	a_server.OpenFTPConnection();
	a_server.ChangeWorkingDirectoryTo("edgar/daily-index");

	a_server.DownloadFile("form.20131010.idx", "/tmp/form.20131010.idx");
	ASSERT_THAT(fs::exists("/tmp/form.20131010.idx"), Eq(true));
}

TEST_F(FTP_UnitTest, VerifyThrowsExceptionWhenTryToDownloadFileDoesntExist)
{
	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	a_server.OpenFTPConnection();
	a_server.ChangeWorkingDirectoryTo("edgar/daily-index");
	auto directory_list = a_server.ListWorkingDirectoryContents();

	ASSERT_THROW(a_server.DownloadFile("form.20131010.abc", "/tmp/downloaded/form.20131010.abc"), Poco::Net::FTPException);
}

class RetrieverUnitTest : public Test
{
public:
	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	DailyIndexFileRetriever idxFileRet{a_server};
};

TEST_F(RetrieverUnitTest, VerifyRejectsInvalidDates)
{
	ASSERT_THROW(idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-Oxt-13")), std::out_of_range);
}

TEST_F(RetrieverUnitTest, VerifyRejectsFutureDates)
{
	ASSERT_THROW(idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2014-10-13")), std::range_error);
}


//	Finally, we get to the point

TEST_F(RetrieverUnitTest, TestFindIndexFileDateNearestWhereDateExists)
{
	auto file_name = idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
	ASSERT_THAT(idxFileRet.GetActualIndexFileDate() == bg::from_simple_string("2013-10-11"), Eq(true));
}

TEST_F(RetrieverUnitTest, TestFindIndexFileDateNearestWhereDateDoesNotExist)
{
	auto file_name = idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-10-14"));
	ASSERT_THAT(idxFileRet.GetActualIndexFileDate() == bg::from_simple_string("2013-10-11"), Eq(true));
}

TEST_F(RetrieverUnitTest, TestExceptionThrownWhenRemoteIndexFileNameUnknown)
{
	ASSERT_THROW(idxFileRet.RetrieveRemoteIndexFileTo("/tmp"), std::runtime_error);
}

TEST_F(RetrieverUnitTest, TestRetrieveIndexFileWhereDateExists)
{
	auto file_name = idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	
	ASSERT_THAT(fs::exists(idxFileRet.GetLocalIndexFilePath()), Eq(true));
}

TEST_F(RetrieverUnitTest, TestRetrieveIndexFileDoesNotReplaceWhenReplaceNotSpecified)
{
	auto file_name = idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto d1 = fs::last_write_time(idxFileRet.GetLocalIndexFilePath());	

	std::this_thread::sleep_for(std::chrono::seconds{1});

	//	we will wait 1 second then do it again so we can compare timestamps
	
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp", false);
	auto d2 = fs::last_write_time(idxFileRet.GetLocalIndexFilePath());	
	ASSERT_THAT(d2 == d1, Eq(true));
}

TEST_F(RetrieverUnitTest, TestRetrieveIndexFileDoesReplaceWhenReplaceIsSpecified)
{
	auto file_name = idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-10-11"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto d1 = fs::last_write_time(idxFileRet.GetLocalIndexFilePath());	

	std::this_thread::sleep_for(std::chrono::seconds{1});

	//	we will wait 1 second then do it again so we can compare timestamps
	
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp", true);
	auto d2 = fs::last_write_time(idxFileRet.GetLocalIndexFilePath());	
	ASSERT_THAT(d2 == d1, Eq(false));
}

class ParserUnitTest : public Test
{
public:
	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	//FTP_Server a_server{"ftp.sec.gov", "anonymous", "aaa@bbb.net"};
	DailyIndexFileRetriever idxFileRet{a_server};
};

//	The following block of tests relate to working with the Index file located above.

TEST_F(ParserUnitTest, VerifyFindProperNumberOfFormEntriesInIndexFile)
{
	auto file_name = idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto local_daily_index_file_name = idxFileRet.GetLocalIndexFilePath();

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);
	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(25));
		
}

TEST_F(ParserUnitTest, VerifyDownloadOfFormFilesListedInIndexFile)
{
	auto file_name = idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto local_daily_index_file_name = idxFileRet.GetLocalIndexFilePath();

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);
	
	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit");
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms_unit"), Eq(CountTotalFormsFilesFound(file_list)));
}

TEST_F(ParserUnitTest, VerifyDownloadOfFormFilesDoesNotReplaceWhenReplaceNotSpecified)
{
	auto file_name = idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto local_daily_index_file_name = idxFileRet.GetLocalIndexFilePath();

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);
	
	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit2");
	auto x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit2");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit2");
	auto x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit2");

	ASSERT_THAT(x1 == x2, Eq(true));
}

TEST_F(ParserUnitTest, VerifyDownloadOfFormFilesDoesReplaceWhenReplaceIsSpecified)
{
	auto file_name = idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto local_daily_index_file_name = idxFileRet.GetLocalIndexFilePath();

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);
	
	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit3");
	auto x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit3");

	std::this_thread::sleep_for(std::chrono::seconds{1});


	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit3", true);
	auto x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit3");

	ASSERT_THAT(x1 == x2, Eq(false));
}

class RetrieverMultipleDailies : public Test
{
public:
	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	//FTP_Server a_server{"ftp.sec.gov", "anonymous", "aaa@bbb.net"};
	DailyIndexFileRetriever idxFileRet{a_server};
};

TEST_F(RetrieverMultipleDailies, VerifyRejectsFutureDates)
{
	ASSERT_THROW(idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-13"), bg::from_simple_string("2014-10-18")),
			   std::range_error);
}

TEST_F(RetrieverMultipleDailies, VerifyFindsCorrectDateRangeWhenBoundingDatesNotFound)
{
	idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-14"), bg::from_simple_string("2013-10-20"));
	auto results = idxFileRet.GetActualDateRange();

	ASSERT_THAT(results, Eq(std::make_pair(bg::from_simple_string("2013-Oct-15"), bg::from_simple_string("2013-10-18"))));
}

TEST_F(RetrieverMultipleDailies, VerifyFindsCorrectNumberOfIndexFilesInRange)
{
	auto results = idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-10"), bg::from_simple_string("2013-10-21"));
	ASSERT_THAT(results.size(), Eq(7));
}

TEST_F(RetrieverMultipleDailies, VerifyThrowsWhenNoIndexFilesInRange)
{
	ASSERT_THROW(idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2012-Oct-10"), bg::from_simple_string("2012-10-21")),
			std::range_error);
}

TEST_F(RetrieverMultipleDailies, VerifyDownloadsCorrectNumberOfIndexFilesForDateRange)
{
	auto file_list = idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-10"), bg::from_simple_string("2013-10-21"));
	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_l");

	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/downloaded_l"), Eq(file_list.size()));
}

TEST_F(RetrieverMultipleDailies, VerifyDownloadOfIndexFilesForDateRangeDoesNotReplaceWhenReplaceNotSpecified)
{
	auto file_list = idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-10"), bg::from_simple_string("2013-10-21"));
	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_r");
	auto x1 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_r");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_r");
	auto x2 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_r");

	ASSERT_THAT(x1 == x2, Eq(true));
}

TEST_F(RetrieverMultipleDailies, VerifyDownloadOfIndexFilesForDateRangeDoesReplaceWhenReplaceIsSpecified)
{
	auto file_list = idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-10"), bg::from_simple_string("2013-10-21"));
	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_r");
	auto x1 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_r");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_r", true);
	auto x2 = CollectLastModifiedTimesForFilesInDirectory("/tmp/downloaded_r");

	ASSERT_THAT(x1 == x2, Eq(false));
}


TEST_F(ParserUnitTest, VerifyFindsCorrectNumberOfFormFilesListedInMultipleIndexFiles)
{
	auto results = idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-10"), bg::from_simple_string("2013-10-21"));
	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_p");
	auto index_file_list = idxFileRet.GetfRemoteIndexFileNamesForDateRange();

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto form_file_list = form_file_getter.FindFilesForForms(forms_list, "/tmp/downloaded_p", index_file_list);

	ASSERT_THAT(CountTotalFormsFilesFound(form_file_list), Eq(255));	
}


class QuarterlyUnitTest : public Test
{
public:
	//FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	FTP_Server a_server{"ftp.sec.gov", "anonymous", "aaa@bbb.net"};
	QuarterlyIndexFileRetriever idxFileRet{a_server};
};

TEST_F(QuarterlyUnitTest, VerifyRejectsInvalidDates)
{
	ASSERT_THROW(idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2013-Oxt-13")), std::out_of_range);
}

TEST_F(QuarterlyUnitTest, VerifyRejectsFutureDates)
{
	ASSERT_THROW(idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2014-10-13")), std::range_error);
}

TEST_F(QuarterlyUnitTest, TestFindIndexFileGivenFirstDayInQuarter)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2000-01-01"));
	ASSERT_THAT(file_name == "2000/QTR1/form.zip", Eq(true));
}

TEST_F(QuarterlyUnitTest, TestFindIndexFileGivenLastDayInQuarter)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2002-06-30"));
	ASSERT_THAT(file_name == "2002/QTR2/form.zip", Eq(true));
}

TEST_F(QuarterlyUnitTest, TestFindAllQuarterlyIndexFilesForAYear)
{
	auto file_names = idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2002-Jan-01"),\
			bg::from_string("2002-Dec-31"));
	ASSERT_THAT(file_names.size(), Eq(4));
}

TEST_F(QuarterlyUnitTest, TestDownloadQuarterlyIndexFile)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2000-01-01"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	
	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.idx"), Eq(true));
	//ASSERT_THAT(fs::exists(idxFileRet.GetLocalIndexFilePath()), Eq(true));
}

TEST_F(QuarterlyUnitTest, TestDownloadQuarterlyIndexFileThenUnzipIt)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2000-01-01"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp", true);
	
	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.idx"), Eq(true));
}

TEST_F(QuarterlyUnitTest, TestDownloadQuarterlyIndexFileThenUnzipItThenDeleteZipFile)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2000-01-01"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp", true);
	
	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.idx"), Eq(true));
	ASSERT_THAT(fs::exists("/tmp/2000/QTR1/form.zip"), Eq(false));
}

TEST_F(QuarterlyUnitTest, VerifyReplaceOptionWorksOffFinalName_NotZipFileName)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2000-01-01"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp/x");
	
	auto x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/x");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	idxFileRet.RetrieveRemoteIndexFileTo("/tmp/x");
	
	auto x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/x");

	ASSERT_THAT(x1 == x2, Eq(true));
}

TEST_F(QuarterlyUnitTest, VerifyReplaceOptionWorksOffFinalNameAndDoesDoReplaceWhenSpecified)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2000-01-01"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp/x");
	
	auto x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/x");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	idxFileRet.RetrieveRemoteIndexFileTo("/tmp/x", true);
	
	auto x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/x");

	ASSERT_THAT(x1 == x2, Eq(false));
}



class QuarterlyRetrieveMultipleFiles : public Test
{
public:
	//FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	FTP_Server a_server{"ftp.sec.gov", "anonymous", "aaa@bbb.net"};
	QuarterlyIndexFileRetriever idxFileRet{a_server};
};

TEST_F(QuarterlyRetrieveMultipleFiles, VerifyRejectsFutureDates)
{
	ASSERT_THROW(idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-13"), bg::from_simple_string("2014-10-18")),
			   std::range_error);
}

TEST_F(QuarterlyRetrieveMultipleFiles, VerifyFindsCorrectNumberOfIndexFilesInRange)
{
	auto file_list = idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2009-Sep-10"), bg::from_simple_string("2010-10-21"));
	ASSERT_THAT(file_list.size(), Eq(5));
}

/* TEST_F(QuarterlyRetrieveMultipleFiles, VerifyThrowsWhenNoIndexFilesInRange) */
/* { */
/* 	ASSERT_THROW(idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2012-Oct-10"), bg::from_simple_string("2012-10-21")), */
/* 			std::range_error); */
/* } */

TEST_F(QuarterlyRetrieveMultipleFiles, VerifyDownloadsCorrectNumberOfIndexFilesForDateRange)
{
	auto file_list = idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2009-Sep-10"), bg::from_simple_string("2010-10-21"));
	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_q");

	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/downloaded_q"), Eq(file_list.size()));
}

TEST_F(QuarterlyRetrieveMultipleFiles, VerifyDownloadOfIndexFilesForDateRangeDoesNotReplaceWhenReplaceNotSpecified)
{
	auto file_list = idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2009-Sep-10"), bg::from_simple_string("2010-10-21"));
	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_q2");
	auto x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/downloaded_q2");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_q2");
	auto x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/downloaded_q2");

	ASSERT_THAT(x1 == x2, Eq(true));
}

TEST_F(QuarterlyRetrieveMultipleFiles, VerifyDownloadOfIndexFilesForDateRangeDoesReplaceWhenReplaceIsSpecified)
{
	auto file_list = idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2009-Sep-10"), bg::from_simple_string("2010-10-21"));
	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_q3");
	auto x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/downloaded_q3");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_q3", true);
	auto x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/downloaded_q3");

	ASSERT_THAT(x1 == x2, Eq(false));
}

class QuarterlyParserUnitTest : public Test
{
public:
	//FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	FTP_Server a_server{"ftp.sec.gov", "anonymous", "aaa@bbb.net"};
	QuarterlyIndexFileRetriever idxFileRet{a_server};
};


TEST_F(QuarterlyParserUnitTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFile)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2009-10-10"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath();

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name);
	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(8294));
		
}

TEST_F(QuarterlyParserUnitTest, VerifyDownloadOfFormFilesListedInQuarterlyIndexFile)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2009-10-10"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath();

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name);
	
	//	since there are thousands of form files listed in the quarterly index,
	//	we will only download a small number to verify functionality
	
	if (file_list.begin()->second.size() > 10)
		file_list.begin()->second.resize(10);

	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit4");
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms_unit4"), Eq(CountTotalFormsFilesFound(file_list)));
}


TEST_F(QuarterlyParserUnitTest, VerifyDownloadOfFormFilesDoesNotReplaceWhenReplaceNotSpecified)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2009-10-10"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath();

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name);
	
	//	since there are thousands of form files listed in the quarterly index,
	//	we will only download a small number to verify functionality
	
	if (file_list.begin()->second.size() > 10)
		file_list.begin()->second.resize(10);

	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit5");
	auto x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit5");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	form_file_getter.RetrieveSpecifiedFiles(file_list, "/tmp/forms_unit5");
	auto x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms_unit5");

	ASSERT_THAT(x1 == x2, Eq(true));
}

class TickerLookupUnitTest : public Test
{
public:

};

TEST_F(TickerLookupUnitTest, VerifyConvertsSingleTickerThatExistsToCIK)
{
	TickerConverter sym;
	auto CIK = sym.ConvertTickerToCIK("AAPL");

	ASSERT_THAT(CIK == "0000320193", Eq(true));

}

TEST_F(TickerLookupUnitTest, VerifyConvertsFileOfTickersToCIKs)
{
	TickerConverter sym;
	auto CIKs = sym.ConvertTickerFileToCIKs("./test_tickers_file", 1);

	ASSERT_THAT(CIKs, Eq(50));

}

TEST_F(TickerLookupUnitTest, VerifyFailsToConvertsSingleTickerThatDoesNotExistToCIK)
{
	TickerConverter sym;
	auto CIK = sym.ConvertTickerToCIK("DHS");

	ASSERT_THAT(CIK == "**no_CIK_found**", Eq(true));

}

class TickerConverterStub : public TickerConverter
{
	public:
		MOCK_METHOD1(EDGAR_CIK_Lookup, std::string(const std::string&));
};

//	NOTE: Google Mock needs mocked methods to be virtual !!  Disable this test
//			when method is de-virtualized...
//
TEST_F(TickerLookupUnitTest, DISABLED_VerifyConvertsSingleTickerThatExistsToCIKUsesCache)
{
	TickerConverterStub sym_stub;
	std::string apple{"AAPL"};
	EXPECT_CALL(sym_stub, EDGAR_CIK_Lookup(apple))
		.WillOnce(Return("0000320193"));

	auto CIK = sym_stub.ConvertTickerToCIK(apple);
	auto CIK2 = sym_stub.ConvertTickerToCIK(apple);

	ASSERT_THAT(CIK == CIK2 && CIK == "0000320193", Eq(true));

}

TEST_F(TickerLookupUnitTest, DISABLED_VerifyConvertsSingleTickerDoesNotUseCacheForNewTicker)
{
	TickerConverterStub sym_stub;
	std::string apple{"AAPL"};
	EXPECT_CALL(sym_stub, EDGAR_CIK_Lookup(_))
		.Times(2)
		.WillOnce(Return("0000320193"))
		.WillOnce(Return(""));

	auto CIK = sym_stub.ConvertTickerToCIK(apple);
	auto CIK2 = sym_stub.ConvertTickerToCIK("DHS");
	auto CIK3 = sym_stub.ConvertTickerToCIK(apple);
	auto CIK4 = sym_stub.ConvertTickerToCIK("DHS");

	ASSERT_THAT(CIK != CIK2, Eq(true));

}

TEST_F(TickerLookupUnitTest, VerifyWritesTickerToFile)
{
	TickerConverter sym;
	auto CIK = sym.ConvertTickerToCIK("AAPL");
	auto CIK2 = sym.ConvertTickerToCIK("DHS");
	auto CIK3 = sym.ConvertTickerToCIK("GOOG");
	sym.UseCacheFile("/tmp/EDGAR_Ticker_CIK.txt");
	sym.SaveCIKDataToFile();

	ASSERT_THAT(fs::file_size("/tmp/EDGAR_Ticker_CIK.txt"), Gt(0));

}


class QuarterlyParserFilterTest : public Test
{
public:
	//FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	FTP_Server a_server{"ftp.sec.gov", "anonymous", "aaa@bbb.net"};
	QuarterlyIndexFileRetriever idxFileRet{a_server};
};

TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileForTicker)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2009-09-10"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath();

	TickerConverter sym;
	auto CIK = sym.ConvertTickerToCIK("AAPL");
	std::map<std::string, std::string> ticker_map;
	ticker_map["AAPL"] = CIK;

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name, ticker_map);
	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(1));
		
}

TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileDateRangeForTicker)
{
	idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2009-Sep-10"), bg::from_simple_string("2010-10-21"));
	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_s");
	auto index_file_list = idxFileRet.GetLocalIndexFileNamesForDateRange();

	TickerConverter sym;
	auto CIK = sym.ConvertTickerToCIK("AAPL");
	std::map<std::string, std::string> ticker_map;
	ticker_map["AAPL"] = CIK;

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, "/tmp/downloaded_s", index_file_list, ticker_map);
	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(4));		//	no 10-Q in 1 quarter.
		
}

class MultipleFormsParserUnitTest : public Test
{
public:
	FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	//FTP_Server a_server{"ftp.sec.gov", "anonymous", "aaa@bbb.net"};
	DailyIndexFileRetriever idxFileRet{a_server};
};

//	The following block of tests relate to working with the Index file located above.

TEST_F(MultipleFormsParserUnitTest, VerifyFindProperNumberOfFormEntriesInIndexFile)
{
	auto file_name = idxFileRet.FindIndexFileNameNearestDate(bg::from_simple_string("2013-10-10"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto local_daily_index_file_name = idxFileRet.GetLocalIndexFilePath();

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"4", "10-K", "10-Q"};

	auto file_list = form_file_getter.FindFilesForForms(forms_list, local_daily_index_file_name);
	
	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(520));
		
}

TEST_F(MultipleFormsParserUnitTest, VerifyFindProperNumberOfFormEntriesInIndexFileForDateRange)
{
	idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2013-Oct-14"), bg::from_simple_string("2013-10-20"));
	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/daily_index1");
	auto index_file_list = idxFileRet.GetfRemoteIndexFileNamesForDateRange();

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"4", "10-K", "10-Q"};

	auto file_list = form_file_getter.FindFilesForForms(forms_list, "/tmp/daily_index1", index_file_list);

	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(4395));
		
}

TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileForMultipleTickers)
{
	auto file_name = idxFileRet.MakeQuarterIndexPathName(bg::from_simple_string("2009-09-10"));
	idxFileRet.RetrieveRemoteIndexFileTo("/tmp");
	auto local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath();

	TickerConverter sym;
	auto CIK1 = sym.ConvertTickerToCIK("AAPL");
	auto CIK2 = sym.ConvertTickerToCIK("DHS");
	auto CIK3 = sym.ConvertTickerToCIK("GOOG");
	std::map<std::string, std::string> ticker_map;
	ticker_map["AAPL"] = CIK1;
	ticker_map["DHS"] = CIK2;
	ticker_map["GOOG"] = CIK3;

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, local_quarterly_index_file_name, ticker_map);
	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(2));
		
}

TEST_F(QuarterlyParserFilterTest, VerifyFindProperNumberOfFormEntriesInQuarterlyIndexFileDateRangeForMultipleTickers)
{
	idxFileRet.FindIndexFileNamesForDateRange(bg::from_simple_string("2009-Sep-10"), bg::from_simple_string("2010-10-21"));
	idxFileRet.RetrieveIndexFilesForDateRangeTo("/tmp/downloaded_t");
	auto index_file_list = idxFileRet.GetLocalIndexFileNamesForDateRange();

	TickerConverter sym;
	auto CIK1 = sym.ConvertTickerToCIK("AAPL");
	auto CIK2 = sym.ConvertTickerToCIK("DHS");
	auto CIK3 = sym.ConvertTickerToCIK("GOOG");
	std::map<std::string, std::string> ticker_map;
	ticker_map["AAPL"] = CIK1;
	ticker_map["DHS"] = CIK2;
	ticker_map["GOOG"] = CIK3;

	FormFileRetriever form_file_getter{a_server};
	std::vector<std::string> forms_list{"10-Q"};
	auto file_list = form_file_getter.FindFilesForForms(forms_list, "/tmp/downloaded_t", index_file_list, ticker_map);
	ASSERT_THAT(CountTotalFormsFilesFound(file_list), Eq(8));		//	no 10-Q in 1 quarter.
		
}

int main(int argc, char** argv) {
	testing::InitGoogleMock(&argc, argv);
   return RUN_ALL_TESTS();
}



