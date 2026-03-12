#pragma once

#include "stdafx.h"

#include <chrono>
#include "cthelpers.h"

#define LARGE_PERF_FACTOR_DIFFERENCE	0.1
#define SHORT_TEST_TIME_MILLIS			1

////////////////////////////////////////////////////////////////////////////////

class PerfTestException : public std::exception
{
public:
	PerfTestException(const std::string& msg)
		: std::exception(msg.c_str())
	{
		std::cout << "PerfTestException: " << msg << std::endl;
	}
};

////////////////////////////////////////////////////////////////////////////////

namespace PerfTestHelpers
{
	std::string ToMultiByte(const std::wstring& ws)
	{
		std::vector<char> buffer(ws.size() + 1);	
		WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, &buffer[0], 
			int(buffer.size()), NULL, NULL);

		return &buffer[0];
	}

	std::wstring ToWideChar(const std::string& mbs)
	{
		std::vector<wchar_t> buffer(mbs.size() + 1);
		MultiByteToWideChar(CP_ACP, 0, mbs.c_str(), -1, &buffer[0], 
			int(buffer.size()));

		return &buffer[0];
	}

	template<typename T>
	std::string ToString(const T& v, size_t precision = -1)
	{
		std::ostringstream ss;

		if (precision != -1)
		{
			ss.precision(precision);
			ss << std::fixed;
		}

		ss << v;

		return ss.str();
	}

	template<typename T>
	T ToValue(const std::string& str)
	{
		T result;

		std::istringstream ss(str);
		ss >> result;

		if (ss.fail() || !ss.eof())
		{
			throw PerfTestException("Cannot parse '" + str + "' as " + 
				typeid(T).name());
		}

		return result;
	}

	std::vector<std::string> Split(const std::string& str)
	{ 
		std::istringstream ss(std::regex_replace(str, 
			std::regex("[^\\w]"), std::string(" ")));

		std::vector<std::string> tokens;
		while (ss.good())
		{
			std::string t;
			ss >> t;
			tokens.push_back(t);
		}

		return tokens;
	}

	// NOTE: we assume that build info for this is the same as VT core!
	std::string GetBuildInfo()
	{
		std::string bi;
#ifdef _DEBUG 
		bi += "Debug";
#else
		bi += "Release";
#endif
		bi += (sizeof(size_t) == 8 ? "64" : "32");

		return bi;
	}

	std::string GetDate()
	{
		SYSTEMTIME t;
		GetSystemTime(&t);

		std::wostringstream ss;		
		std::vector<wchar_t> buf(256);
		GetDateFormatEx(NULL, 0, &t, L"yyyy-MM-dd", &buf[0], int(buf.size()), 
			NULL);
		ss << &buf[0];

		return PerfTestHelpers::ToMultiByte(ss.str());
	}

	std::string GetTime()
	{
		SYSTEMTIME t;
		GetSystemTime(&t);

		std::wostringstream ss;		
		std::vector<wchar_t> buf(256);
		GetTimeFormatEx(NULL, TIME_FORCE24HOURFORMAT, &t, L"hh-mm-ss", &buf[0], 
			int(buf.size()));
		ss << &buf[0];		

		return PerfTestHelpers::ToMultiByte(ss.str());
	}

	std::string GetMachineName()
	{
		std::vector<wchar_t> buf(256);
		DWORD sz = DWORD(buf.size());
		GetComputerName(&buf[0], &sz);

		return PerfTestHelpers::ToMultiByte(&buf[0]);
	}

	std::string GetUserName()
	{
		std::vector<wchar_t> buf(256);
		DWORD sz = DWORD(buf.size());		
		::GetUserName(&buf[0], &sz);

		return PerfTestHelpers::ToMultiByte(&buf[0]);
	}

	std::vector<std::string> ReadCSVRow(std::ifstream& file)
	{
		std::vector<std::string> tokens;

		std::string line;
		std::getline(file, line);

		if (!line.empty())
		{		
			std::vector<char> str(line.begin(), line.end());
			str.push_back(0);

			char* context;
			const char* pch = strtok_s(&str[0], ",", &context);
			while (pch != NULL)
			{
				std::string t(pch);
				t.erase(remove_if(t.begin(), t.end(), isspace), t.end()); 	
				tokens.push_back(t);

				pch = strtok_s(NULL, ",", &context);
			}
		}

		return tokens;
	}

	template<typename Iterator> 
	void WriteCSVRow(std::ofstream& file, Iterator begin, Iterator end)
	{
		WriteCSVRow(file, begin, end, 
			[](const typename std::iterator_traits<Iterator>::value_type& v)
		{
			return v;
		});
	}

	template<typename Iterator, typename Transform> 
	void WriteCSVRow(std::ofstream& file, Iterator begin, Iterator end,
		Transform func)
	{		
		const size_t dist = std::distance(begin, end);

		Iterator it = begin;

		for (size_t i = 0; i < dist; ++i)
		{
			file << func(*it++);

			if (i < dist - 1)
				file << ", ";
			else
				file << std::endl;
		}
	}

	bool ValidateSchema(const std::string& path, 
		const std::vector<std::pair<std::string, std::string>>& row)
	{
		std::ifstream file(path);

		if (!file.good())
			return false;

		std::vector<std::string> schema = ReadCSVRow(file);

		if (row.size() != schema.size() ||
			!std::equal(row.begin(), row.end(), schema.begin(), 
			[](const std::pair<std::string, std::string>& a,
			const std::string& b)
		{
			return a.first == b;
		}))
		{
			throw PerfTestException("Invalid schema");
		}

		return true;
	}

	void AppendRow(const std::string& path, 
		const std::vector<std::pair<std::string, std::string>>& row)
	{		
		bool file_exists = PerfTestHelpers::ValidateSchema(path, row);

		std::ofstream file(path, std::ios::app);

		for (size_t n_tries = 0; !file.good() && n_tries < 10; ++n_tries)
		{
			Sleep(100);
			file.open(path, std::ios::app);
		}
			
		if (!file.good())
			throw PerfTestException("Cannot write file: " + path);

		if (!file_exists)
		{
			WriteCSVRow(file, row.begin(), row.end(),
				[](const std::pair<std::string, std::string>& col)
			{
				return col.first;
			});
		}

		WriteCSVRow(file, row.begin(), row.end(), 
			[](const std::pair<std::string, std::string>& col)
		{
			return col.second;
		});
	}

	// http://en.wikipedia.org/wiki/Welch%27s_t_test
	bool WelchTest(double m1, double s1, int n1, double m2, double s2, 
		int n2)
	{
		const double student_t_2tail_95[] = { 12.7062, 4.30265, 3.18245, 
			2.77645, 2.57058, 2.44691, 2.36462, 2.306, 2.26216, 2.22814, 
			2.20099, 2.17881, 2.16037, 2.14479, 2.13145, 2.11991, 2.10982, 
			2.10092, 2.09302, 2.08596, 2.07961, 2.07387, 2.06866, 2.0639, 
			2.05954, 2.05553, 2.05183, 2.04841, 2.04523, 2.04227 };

		const double d = m1 - m2;
		const double tmp1 = s1*s1/n1;
		const double tmp2 = s2*s2/n2;
		const double stmp = tmp1 + tmp2;

		const double t = d / sqrt(stmp);

		const double nu1 = n1 - 1;
		const double nu2 = n2 - 1;
		const int nu = (int) (stmp*stmp / (tmp1*tmp1/nu1 + tmp2*tmp2/nu2) 
			+ 0.5);

		const double threshold = student_t_2tail_95[std::min<int>(nu, 
			_countof(student_t_2tail_95) - 1)];

		return abs(t) > threshold;
	}

	bool IsVarianceLarge(double mean, double stddev, int num_trials)
	{
		return 2.0 * stddev / sqrt((double)num_trials) > 
			0.5 * LARGE_PERF_FACTOR_DIFFERENCE * mean;
	}

	std::tuple<double, bool, bool, std::string> ComputeTimeFactor(double m1, 
		double s1, int n1, double m2, double s2, int n2)
	{		
		double factor = m2 / (m1 + 1e-9);			

		bool is_large = (std::max<double>(factor, 1.0/factor) - 1.0) > 
			LARGE_PERF_FACTOR_DIFFERENCE;			

		bool is_significant = WelchTest(m1, s1, n1, m2, s2, n2);

		std::string str = ToString(std::max<double>(factor, 1.0/factor), 2) +
			"x " + (factor > 1.0 ? "SLOWER" : "faster");

		return std::tuple<double, bool, bool, std::string>(factor, 
			is_significant, is_large, str);
	}

	std::string MeanStdErrAsString(double mean, double stddev, int num_trials, 
		size_t precision)
	{
		return PerfTestHelpers::ToString(mean, precision) + " [+/-" 
			+  PerfTestHelpers::ToString(2.0 * stddev / 
			sqrt((double)num_trials), precision) + "]";
	}
};

////////////////////////////////////////////////////////////////////////////////

enum DiffResultType
{
	Error,
	ShortTime,
	HighVariance,
	New,
	Same,
	Better,
	Worse
};

////////////////////////////////////////////////////////////////////////////////

class PerfTestManager
{
private:
	PerfTestManager()
	{		
	}

	~PerfTestManager()
	{		
	}

	PerfTestManager(const PerfTestManager&);
	PerfTestManager& operator=(const PerfTestManager&);

public:
	static void Initialize()	
	{
		GetInstance();
	}

	static void RegisterTestStart(const std::string& name)
	{
		GetInstance().RegisterTestStartInternal(name);
	}

	static void RegisterTestFinish(const std::string& name,
		const std::pair<DiffResultType, std::string>& summary)
	{
		GetInstance().RegisterTestFinishInternal(name, summary);
	}

	static void PrintSummary()
	{
		GetInstance().PrintSummaryInternal();
	}

private:	
	static PerfTestManager& GetInstance() 
	{
		static PerfTestManager self;
		return self;
	}

private:
	void RegisterTestStartInternal(const std::string& name)
	{
		if (tests_.end() != std::find(tests_.begin(), tests_.end(), name))
			throw PerfTestException("Test '" + name + "' already run");

		tests_.push_back(name);
		test_info_[name] = std::make_pair(Error, "Interrupted");
	}

	void RegisterTestFinishInternal(const std::string& name,
		const std::pair<DiffResultType, std::string>& summary)
	{
		if (tests_.end() == std::find(tests_.begin(), tests_.end(), name))
			throw PerfTestException("Test '" + name + "' not started");

		test_info_[name] = summary;
	}

	size_t PrintResultGroup(DiffResultType type, const std::string& description) 
		const
	{
		if (std::all_of(tests_.begin(), tests_.end(), 
			[this, type](const std::string& name)
		{
			return (*test_info_.find(name)).second.first != type;
		}))
		{
			return 0;
		}

		std::cout << "\n" << description << "\n";
		std::cout << "|" << std::endl;

		size_t num_in_group = 0;
		std::for_each(tests_.begin(), tests_.end(), 
			[this, type, &num_in_group](const std::string& name)
		{
			auto info = (*test_info_.find(name)).second;
			
			if (info.first != type)
				return;
			
			std::cout << "[" << name << "]: " << info.second << std::endl;

			++num_in_group;
		});

		return num_in_group;
	}

	void PrintSummaryInternal() const
	{
		if (tests_.empty())
			return;

		//size_t nsame = PrintResultGroup(Same, 
		//	"OK--no significant difference to latest or best record");
		size_t nsame = std::count_if(test_info_.begin(), test_info_.end(), 
			[this](const decltype(*test_info_.begin())& ti)
		{
			return ti.second.first == Same;
		});

		size_t nbetter = PrintResultGroup(Better, 
			"GOOD perf results--faster than latest record");
		size_t nnew = PrintResultGroup(New, 
			"NEW  perf results--no history available");
		size_t nworse = PrintResultGroup(Worse, 
			"BAD perf results--SLOWER than latest record");
		size_t nhighvar = PrintResultGroup(HighVariance, 
			"TOO HIGH VARIANCE perf results--increase sample size or test duration");
		size_t nshort = PrintResultGroup(ShortTime, 
			"TEST TOO SHORT perf results--increase test duration");
		size_t nerror = PrintResultGroup(Error, 
			"ERROR perf results--test failed");

		std::cout << "\nPerf test summary:\n";
		std::cout << "OK: " << nsame << ", GOOD: " << nbetter << ", NEW: " 
			<< nnew << ", BAD:" << nworse << ", INVALID: " 
			<< nhighvar + nshort + nerror << std::endl;		
	}

private:
	std::vector<std::string> tests_;
	std::map<std::string, std::pair<DiffResultType, std::string>> test_info_;
};


////////////////////////////////////////////////////////////////////////////////

template <typename PerfTestImpl>
class PerfTest
{
public:
	PerfTest(const std::string& data_folder = std::string())
	{
		if (data_folder.empty())
		{
			data_folder_ = PerfTestHelpers::ToMultiByte(
				GetEnvironmentVariable(L"PERFDATA_DIR"));

			if (data_folder_.empty())
			{
				data_folder_ = "..\\src\\tests\\coretest\\perf\\";
			}
		}
	}

private:
	PerfTest(const PerfTest&);
	PerfTest& operator=(const PerfTest&);

protected:
	virtual std::string GetName() const = 0;
	virtual std::string GetParamNames() const = 0;
	virtual void Initialize()
	{
	};
	virtual void Cleanup()
	{
	};

public:
	float RunOnce()
	{
		InitializeColumns(1);
		Initialize();
		
		auto t = std::chrono::high_resolution_clock::now();
		static_cast<PerfTestImpl*>(this)->RunSingleTestCase();
		float res = GetElapsedMilliSec(t);
		
		Cleanup();

		return res;
	}
	
	float Run(size_t n_trials, size_t loop_count, DiffResultType* drt = nullptr)
	{
		PerfTestManager::RegisterTestStart(GetFullName());

		InitializeColumns(loop_count);
		
		Initialize();

		bool ok = true;
		std::vector<double> times;
		times.reserve(n_trials);
		for (size_t i = 0; i < n_trials; ++i)
		{
			auto t = std::chrono::high_resolution_clock::now();
			
			for (size_t j = 0; j < loop_count; ++j)
			{
				if (!static_cast<PerfTestImpl*>(this)->RunSingleTestCase())
				{
					ok = false;
					break;
				}
			}

			times.push_back(GetElapsedMilliSec(t)/loop_count);
		}

		/*std::ofstream tst("d:\\temp\\val.txt");
		for (size_t i = 0; i < n_trials; ++i)
		{
			tst << times[i] << std::endl;
		}*/
		
		Cleanup();

		ComputeStats(ok, times);

		auto summary = ComputeDiff();

		if (GetEnvironmentVariable(L"PERFDATA_COMMIT") == L"1")
			PerfTestHelpers::AppendRow(GetLogFileName(), data_);

		if (drt != nullptr)
			*drt = summary.first;

		PerfTestManager::RegisterTestFinish(GetFullName(), summary);

		return PerfTestHelpers::ToValue<float>((*FindColumnByName("AvgMillis"))
			.second);
	}

	template<typename T>
	void SetParamValue(const std::string& name, const T& value)
	{		
		if (!IsUserColumnDefined(name))
			throw PerfTestException("Undefined parameter: " + name);

		user_data_[name] = PerfTestHelpers::ToString(value);
	}

	template<typename T>
	const T GetParamValue(const std::string& name) const
	{
		if (!IsUserColumnDefined(name))
			throw PerfTestException("Undefined parameter: " + name);

		auto it = user_data_.find(name);
		if (it == user_data_.end())
			throw PerfTestException("Parameter value not set: " + name);

		return PerfTestHelpers::ToValue<T>((*it).second);
	}

private:
	float GetElapsedMilliSec(std::chrono::high_resolution_clock::time_point start)
	{
		return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::milliseconds::period>>
			(std::chrono::high_resolution_clock::now() - start).count();
	}

	std::string GetFullName() const
	{
		std::ostringstream ss;
		
		ss << GetName();

		const std::vector<std::string> pnames = 
			PerfTestHelpers::Split(GetParamNames());
		
		std::for_each(pnames.begin(), pnames.end(), 
			[&ss, this](const std::string& n)
		{
			ss << "|" << this->GetParamValue<std::string>(n);
		});

		return ss.str();
	}

	bool IsUserColumnDefined(const std::string& name) const
	{
		const std::vector<std::string> names = 
			PerfTestHelpers::Split(GetParamNames());

		return names.end() != std::find(names.begin(), names.end(), name);
	}

	std::vector<std::pair<std::string, std::string>>::iterator
		FindColumnByName(const std::string& name)
	{
		return std::find_if(data_.begin(), data_.end(), 
			[=](std::pair<std::string, std::string> c)
		{
			return c.first == name;
		});
	}

	std::vector<std::pair<std::string, std::string>>::const_iterator
		FindColumnByName(const std::string& name) const
	{
		return std::find_if(data_.begin(), data_.end(), 
			[=](std::pair<std::string, std::string> c)
		{
			return c.first == name;
		});
	}

	size_t FindColumnIndexByName(const std::string& name) const
	{
		auto it = FindColumnByName(name);
		return it != data_.end() ? it - data_.begin() : -1;
	}

	void AddColumn(const std::string& name, const std::string& value)
	{
		if (data_.end() != FindColumnByName(name))
			throw PerfTestException("Column already exists");

		data_.push_back(std::make_pair(name, value));
	}

	void InitializeColumns(size_t loop_count)
	{
		data_.clear();

		AddColumn("Date", PerfTestHelpers::GetDate());
		AddColumn("Time", PerfTestHelpers::GetTime());
		AddColumn("User", PerfTestHelpers::GetUserName());
		AddColumn("Machine", PerfTestHelpers::GetMachineName());
		AddColumn("Build", PerfTestHelpers::GetBuildInfo());
		AddColumn("LoopCount", PerfTestHelpers::ToString(loop_count));

		auto params = PerfTestHelpers::Split(GetParamNames());
		std::for_each(params.begin(), params.end(), 
			[this](const std::string& name)
		{
			auto it = user_data_.find(name);
			AddColumn(name, it != user_data_.end() ? (*it).second : "");
		});
	}

	std::string GetLogFileName() const
	{
		return data_folder_ + "\\" + GetName() + ".csv";
	}	

	void ComputeStats(bool ok, const std::vector<double>& times)
	{
		AddColumn("Status", ok ? "OK" : "FAILED");
		AddColumn("NumTrials", PerfTestHelpers::ToString(times.size()));

		const double avg = std::accumulate(times.begin(), times.end(), 0.0) / 
			times.size();

		std::vector<double> tmp(times);
		std::transform(tmp.begin(), tmp.end(), tmp.begin(), [avg](double v) 
		{ 
			return (v - avg) * (v - avg);
		});

		const double stddev = sqrt((std::accumulate(tmp.begin(), tmp.end(), 
			0.0) / tmp.size()));

		AddColumn("AvgMillis", PerfTestHelpers::ToString(avg));
		AddColumn("StdMillis", PerfTestHelpers::ToString(stddev));

		AddColumn("MinMillis", PerfTestHelpers::ToString(
			*std::min_element(times.begin(), times.end())));
		AddColumn("MaxMillis", PerfTestHelpers::ToString(
			*std::max_element(times.begin(), times.end())));
	}

	bool DataRowHasSameParams(const std::vector<std::string>& tokens) const
	{
		bool match = true;

		std::for_each(user_data_.begin(), user_data_.end(), 
			[this, &tokens, &match](const decltype(*user_data_.begin())& param)
		{ 
			if (tokens[FindColumnIndexByName(param.first)] != param.second)
				match = false;
		});

		return match;
	}

	std::pair<DiffResultType, std::string> ComputeDiff() const
	{
		PerfTestHelpers::ValidateSchema(GetLogFileName(), data_);

		const size_t meanidx = FindColumnIndexByName("AvgMillis");
		const size_t stdidx = FindColumnIndexByName("StdMillis");
		const size_t nidx = FindColumnIndexByName("NumTrials");

		double mean = PerfTestHelpers::ToValue<double>(data_[meanidx].second);
		double stddev = PerfTestHelpers::ToValue<double>(data_[stdidx].second);
		int num_trials = PerfTestHelpers::ToValue<int>(data_[nidx].second);

		DiffResultType rtype = Error;

		std::ostringstream summary;
		if (mean < 1.0)
		{
			summary << PerfTestHelpers::MeanStdErrAsString(mean * 1000, 
				stddev * 1000, num_trials, 1) << " us";
		}
		else
		{
			summary << PerfTestHelpers::MeanStdErrAsString(mean, stddev, 
				num_trials, 1) << " ms";
		}

		std::string status = (*FindColumnByName("Status")).second;
		int loop_count = PerfTestHelpers::ToValue<int>(
			(*FindColumnByName("LoopCount")).second);

		if (status != "OK")
		{
			rtype = Error;
		}
		else if (mean * loop_count < SHORT_TEST_TIME_MILLIS)
		{
			rtype = ShortTime;
		}
		else if	(PerfTestHelpers::IsVarianceLarge(mean, stddev, num_trials))
		{
			rtype = HighVariance;
		}		
		else
		{
			std::map<std::string, std::tuple<double, double, int>> stats;
			const size_t dateidx = FindColumnIndexByName("Date");
			const size_t timeidx = FindColumnIndexByName("Time");
			const size_t machidx = FindColumnIndexByName("Machine");
			const size_t buildidx = FindColumnIndexByName("Build");

			std::vector<std::string> tokens;
			std::ifstream file(GetLogFileName());
			PerfTestHelpers::ReadCSVRow(file); // skip header
			bool is_new = true;
			while (!(tokens = PerfTestHelpers::ReadCSVRow(file)).empty())
			{
				if (tokens[machidx] != data_[machidx].second ||
					!DataRowHasSameParams(tokens) ||
					tokens[buildidx] != data_[buildidx].second)
					continue;

				is_new = false;

				stats[tokens[dateidx] + "-" + tokens[timeidx]] = 
					std::tuple<double, double, int>(
					PerfTestHelpers::ToValue<double>(tokens[meanidx]),
					PerfTestHelpers::ToValue<double>(tokens[stdidx]),
					PerfTestHelpers::ToValue<int>(tokens[nidx]));
			}

			// compare to latest record
			auto latest = std::max_element(stats.begin(), stats.end(), 
				[](const decltype(*stats.end())& a, 
				const decltype(*stats.end())& b)
			{
				return a.first < b.first;
			});

			if (latest != stats.end())
			{
				auto fac = PerfTestHelpers::ComputeTimeFactor(
					std::get<0>((*latest).second), 
					std::get<1>((*latest).second), 
					std::get<2>((*latest).second), mean, stddev, num_trials);

				if (std::get<1>(fac) && std::get<2>(fac))
				{
					summary << ", " << std::get<3>(fac) << " than latest";
					rtype = std::get<0>(fac) < 1.0 ? Better : Worse;
				}
			}
			/*
			// compare to best
			auto best = std::min_element(stats.begin(), stats.end(), 
				[](const decltype(*stats.end())& a, 
				const decltype(*stats.end())& b)
			{
				return std::get<0>(a.second) < std::get<0>(b.second);
			});

			if (best != stats.end())
			{
				auto fac = PerfTestHelpers::ComputeTimeFactor(
					std::get<0>((*best).second), std::get<1>((*best).second), 
					std::get<2>((*best).second), mean, stddev, num_trials);

				if (std::get<1>(fac) && std::get<2>(fac))
				{
					summary << ", " << std::get<3>(fac) << " than best";
					if (rtype != Worse)
						rtype = std::get<0>(fac) < 1.0 ? Better : Worse;					
				}
				}*/

			if (rtype == Error)
				rtype = is_new ? New : Same;
		}

		return std::make_pair(rtype, summary.str());
	}

private:
	std::string data_folder_;
	std::vector<std::pair<std::string,std::string>> data_;
	std::map<std::string,std::string> user_data_;
};
