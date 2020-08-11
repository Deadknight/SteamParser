#pragma once
class BasicTaskExecutor : public ThreadBase
{
	CallbackBase * cb;
	uint32 priority;
public:
	BasicTaskExecutor(CallbackBase * Callback, uint32 Priority) : cb(Callback), priority(Priority) {}
	~BasicTaskExecutor() { delete cb; }
	bool run();
};

class Task
{
	CallbackBase * _cb;
public:
	Task(CallbackBase * cb) : _cb(cb), completed(false), in_progress(false) {}
	~Task() { delete _cb; }
	bool completed;
	bool in_progress;
	void execute();
};

class TaskList
{
	std::set<Task*> tasks;
	Mutex queueLock;
public:
	Task * GetTask();
	void AddTask(Task* task);
	void RemoveTask(Task * task)
	{
		queueLock.Acquire();
		tasks.erase(task);
		queueLock.Release();
	}

	void spawn();
	void kill();

	void wait();
	void waitForThreadsToExit();
	uint32 thread_count;
	bool running;

	Mutex tcMutex;
	void incrementThreadCount()
	{
		tcMutex.Acquire();
		++thread_count;
		tcMutex.Release();
	}

	void decrementThreadCount()
	{
		tcMutex.Acquire();
		--thread_count;
		tcMutex.Release();
	}
};

enum BasicTaskExecutorPriorities
{
	BTE_PRIORITY_LOW		= 0,
	BTE_PRIORITY_MED		= 1,
	BTW_PRIORITY_HIGH	   = 2,
};

class TaskExecutor : public ThreadBase
{
	TaskList * starter;
public:
	TaskExecutor(TaskList * l) : starter(l) { l->incrementThreadCount(); }
	~TaskExecutor() { starter->decrementThreadCount(); }

	bool run();
};

struct KillBy
{
	std::wstring weaponName;
	uint32 killCount;
	uint32 critKillCount;
};

struct DeathBy
{
	std::wstring weaponName;
	uint32 deathCount;
};

struct UpdateItem
{
	std::string name;
	std::string version;
	uint32 length;
	bool newItem;
};

struct ProgressBar
{
	std::ofstream *ofs;
	HWND dlg;
	HWND prog;
	uint32 len;
	uint32 total;
};

struct MapItem
{
	std::wstring name;
	uint32 killCount;
	uint32 deathCount;
	uint32 captureCountBlue;
	uint32 defendCountBlue;
	uint32 captureCountRed;
	uint32 defendCountRed;
	std::wstring lastPlayed;
	double totalTime;
};

struct PlayerItem
{
	uint32 killCount;
	uint32 critKillCount;
	uint32 deathCount;
};

class Component : public Singleton <Component>
{
public:
	Component();
	~Component();

	void Dispose() { exit = true; }
	void Update();
	void UpdateFile();
	void UpdateConfig();
	uint32 UpdateToWeb();
	void UpdateMapTime();

	static INT_PTR CALLBACK UpdateDialog(HWND, UINT, WPARAM, LPARAM);
	static size_t write_data(char *data, size_t size, size_t nmemb, std::string *bufferdata);
	static size_t write_data_file(char *data, size_t size, size_t nmemb, std::ofstream *bufferdata);
	static size_t write_data_file_s(char *data, size_t size, size_t nmemb, ProgressBar *bufferdata);
	bool CheckUpdate(HINSTANCE hInst, HWND hWnd);
	std::string GetUpdateLink() { return updateLink; }

	void ParseW(std::wstring line, std::vector<std::wstring> results, std::wstring date);

	void InitFileWatcher();
	void DisposeFileWatcher();

	time_t GetSeconds(std::wstring date);
	std::wstring GetPath() { return path; }
	md5wrapper *GetMD5() { return wMD5; }
	void SetPath(std::wstring value) { path = value; }
	void SetPath(std::string value);
	COXFileWatcher *GetFileWatcher() { return fw; }
	void SetUsername(std::wstring value) { username = value; }
	void SetPassword(std::wstring value) { password = value; }
	std::wstring GetPasswordHash() { return passwordHash; }
	void SetPasswordHash(std::wstring value) { passwordHash = value; }

	void AcquirePathLock() { m_pathLock.Acquire(); }
	void ReleasePathLock() { m_pathLock.Release(); }

	//Run Check
	bool IsRunning(std::wstring exe);
	
	//Ban
	void Ban();

	//Crypto
	std::string Encrypt(std::string str);
	std::string Decrypt(std::string str);

	//Run
	void RunExecutable(std::wstring name, std::wstring params);

private:
	bool safe;
	bool exit;
	std::wstring username;
	std::wstring password;
	std::wstring passwordHash;

	//Parseing
	std::wstring path;
	COXFileWatcher *fw;
	bool m_parsing;
	uint64 position;
	std::wstring lastLine;
	Mutex m_pathLock;
	Mutex m_banLock;

	//return data
	std::string data;

	//version map
	std::map<std::string, std::string> versionMap;
	std::string updateLink;

	//Md5
	md5wrapper *wMD5;

	//Security
	OpenFileFind *off;

	//Ban Control
	bool banned;
	uint64 banCheck;

	//Stats
	std::wstring nick;
	bool nameSearch;
	time_t currentDate;
	std::wstring lastDate;
	bool dateSend;
	uint32 killCount;
	uint32 deathCount;
	uint32 critKillCount;
	uint32 captureCount;
	uint32 defendCount;
	uint32 suicideCount;
	std::map<std::wstring, MapItem*> maps;
	std::map<std::wstring, PlayerItem*> pkdCount;
	MapItem *currentMapItem;
	std::set<KillBy*> lstKillBy;
	std::set<DeathBy*> lstDeathBy;
};

#define sComponent Component::getSingleton()
