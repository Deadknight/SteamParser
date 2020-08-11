#include "stdafx.h"

initialiseSingleton( Component );

Component::Component()
{
	printf("We are all haxors :D:D:D:D:D\n");
	fw = NULL;
	username = L"";
	password = L"";
	passwordHash = L"";
	lastLine = L"";
	position = 0;
	path = L"";
	exit = false;
	nick = L"";
	nameSearch = false;
	killCount = 0;
	critKillCount = 0;
	deathCount = 0;
	captureCount = 0;
	defendCount = 0;
	suicideCount = 0;
	currentMapItem = NULL;
	currentDate = 0;
	lastDate = L"";
	dateSend = true;
	printf("Ofc www.sombrenuit.org rulez everytime");

	banCheck = 0;
	std::wifstream ifs("config.cfg");
	if(ifs)
	{
		std::wstring line;3
		std::getline(ifs, line);
		std::string decrypted = "";
		safe = true;
		try
		{
			decrypted = Decrypt(StringUtil::Trim(StringUtil::ToUtf8(line)));
		}
		catch(...)
		{
			safe = false;
		}
		if(safe)
		{
			line = StringUtil::FromUtf8(decrypted);
			std::vector<std::wstring> result;
			StringUtil::SplitStringW(line, L",", result, true);
			if(result.size() > 0)
				path = result[0];
			if(result.size() > 1)
				username = result[1];
			if(result.size() > 2)
				passwordHash = result[2];
			//if(result.size() > 3)
				//position = atoi(StringUtil::Trim(StringUtil::ToUtf8(result[3])).c_str());
			if(result.size() > 4)
				banCheck = atol(StringUtil::Trim(StringUtil::ToUtf8(result[4])).c_str());
			std::wstringstream lastlines;
			for(int32 i = 5; i < result.size(); i++)
			{
				if(i == result.size())
					lastlines << result[i];
				else
					lastlines << result[i] << L",";
			}
			lastLine = lastlines.str();
		}
	}

	//Ban Check
	banned = false;
	registry_int<int> ban(L"Software\\Sombrenuit\\Steam Parser\\T", HKEY_LOCAL_MACHINE);
	if(ban != 0)
	{
		uint64 diff = time(NULL) - ban;
		if(diff > 86400)
		{
			ban = 0;
			banCheck = 0;
		}
		else
		{
			banned = true;
			banCheck = ban;
		}
	}
	else
	{
		//Hacker fucker
		if(banCheck > 0)
		{
			uint64 diff = time(NULL) - banCheck;
			if(diff > 86400)
			{
				ban = 0;
				banCheck = 0;
			}
			else
			{
				banned = true;
				ban = banCheck;
			}
		}
	}
	
	wMD5 = new md5wrapper();
	off = new OpenFileFind();
	
	std::string name = "SteamParser.exe";
	std::string ver = "1.0.0.0";
	versionMap.insert(std::make_pair(name, ver));
	name = "libcurl.dll";
	ver = "7.18.0";
	versionMap.insert(std::make_pair(name, ver));
	name = "zlib1.dll";
	ver = "1.2.3";
	versionMap.insert(std::make_pair(name, ver));
}

Component::~Component()
{
	std::set<KillBy*>::iterator itr = lstKillBy.begin();
	for(; itr != lstKillBy.end();)
	{
		KillBy *kb = (*itr);
		itr++;
		delete kb;
	}
	lstKillBy.clear();

	std::set<DeathBy*>::iterator itrD = lstDeathBy.begin();
	for(; itrD != lstDeathBy.end();)
	{
		DeathBy *kb = (*itrD);
		itrD++;
		delete kb;
	}
	lstDeathBy.clear();

	std::map<std::wstring, MapItem*>::iterator itrM = maps.begin();
	for(; itrM != maps.end();)
	{
		MapItem *mi = itrM->second;
		itrM++;
		delete mi;
	}
	maps.clear();
	currentMapItem = NULL;

	std::map<std::wstring, PlayerItem*>::iterator itrP = pkdCount.begin();
	for(; itrP != pkdCount.end();)
	{
		PlayerItem *pi = itrP->second;
		itrP++;
		delete pi;
	}
	pkdCount.clear();

	delete wMD5;
	delete off;
	versionMap.clear();
}

void Component::Update()
{
	while(!exit)
	{
		Sleep(2000);
		m_pathLock.Acquire();
		if(path != L"")
		{
			uint64 lastPosition = position;
			uint64 lastStorePosition = position;
			
			//Add l4d switch here
			std::wstring pathNew = path + L"tf\\sp.log";
#ifndef TESTDEBUG
			if(!IsRunning(L"hl2.exe"))
				DeleteFile(pathNew.c_str());
#endif

			std::ifstream ifs(pathNew.c_str(), std::ios::binary);
			if(!ifs)
			{
				m_pathLock.Release();
				continue;
			}
			if(position != 0)
			{
				ifs.seekg(position);
				//Not sure
				std::string lineS;
				std::getline(ifs, lineS);
				lineS = StringUtil::FixString(lineS);
				if(lineS != StringUtil::Trim(StringUtil::ToUtf8(lastLine)))
				{
					position = 0;
					lastLine = L"";
					ifs.seekg(0);
					ifs.clear();
				}
			}
			if(!safe)
			{
				ifs.seekg(0, std::ios::end);
				position = ifs.tellg();
				/*std::wstring Buffer;

				// Approx line size
				std::ifstream::pos_type OneLine( 30 );

				// Seek backwards from the eof (approx one line)...
				ifs.seekg( -OneLine, std::ios_base::end );
				std::getline(*/
				safe = true;
			}
			/*if(ifs.fail())
			{
				position = 0;
				lastLine = L"";
				ifs.seekg(0);
				ifs.clear();
			}*/
			while(!ifs.eof())
			{
				lastStorePosition = position;
				position = ifs.tellg();
				std::string lineS;
				std::getline(ifs, lineS);				
				std::wstring line = StringUtil::FromUtf8(lineS);
				line = StringUtil::FixStringW(line);
				std::string fixed = StringUtil::FixString(lineS);
				if(fixed == "")
				{
					position = lastStorePosition;
					continue;
				}
				lastLine = line;

				std::vector<std::wstring> result;
				std::vector<std::wstring> resultDate;
				StringUtil::SplitStringW(line, L":", resultDate, true, 3, 3);
				if(resultDate.size() > 1)
				{
					StringUtil::SplitStringW(resultDate[1], L" ", result, false);
					ParseW(resultDate[1], result, resultDate[0]);
					lastDate = resultDate[0];
				}
				else
				{
					if(line == L"Team Fortress")
					{
						nameSearch = true;
						continue;
					}
					StringUtil::SplitStringW(line, L" ", result, true);
					if(!result.size())
						continue;
					else if(result[0] == L"Map:")
					{
						UpdateMapTime();
						std::map<std::wstring, MapItem*>::iterator itr = maps.find(result[1]);
						if(itr != maps.end())
							currentMapItem = itr->second;
						else
						{
							currentMapItem = new MapItem();
							currentMapItem->name = result[1];
							currentMapItem->killCount = 0;
							currentMapItem->deathCount = 0;
							currentMapItem->captureCountBlue = 0;
							currentMapItem->defendCountBlue = 0;
							currentMapItem->captureCountRed = 0;
							currentMapItem->defendCountRed = 0;
							currentMapItem->lastPlayed = L"";
							currentMapItem->totalTime = 0;
							maps.insert(std::make_pair(result[1], currentMapItem));
						}
						continue;
					}
				}
			}
			UpdateMapTime();
			UpdateConfig();
			if(position && lastPosition != position)
				UpdateToWeb();
			position = lastStorePosition;
			ifs.close();
#ifndef TESTDEBUG
			m_banLock.Acquire();
			if(banned)
				DeleteFile(pathNew.c_str());
			m_banLock.Release();
			if(!IsRunning(L"hl2.exe"))
				DeleteFile(pathNew.c_str());
#endif
		}
		m_pathLock.Release();
	}
	
}


void Component::UpdateFile()
{
	while(!exit)
	{
		Sleep(2000);
#ifndef TESTDEBUG
		std::vector<ProcessInfo*> lst = off->Populate(path, true);
		std::vector<ProcessInfo*>::iterator itr = lst.begin();
		for(; itr != lst.end();)
		{
			ProcessInfo *pi = (*itr);
			std::vector<std::wstring> results;
			std::vector<std::wstring> resultspName;
			StringUtil::SplitStringW(pi->filename, L"\\", results, true);
			StringUtil::SplitStringW(pi->pname, L"\\", resultspName, true);

			std::wstring pname = pi->pname;
			if(resultspName.size() != 0)
				pname = resultspName[resultspName.size() - 1];

			std::wstring filename = results[results.size() - 1];

			if((filename == L"sp.log" && !(pname == L"hl2.exe" || pname == L"SteamParser.exe"))
				|| ((pname == L"notepad.exe") && (filename == L"tf")))
			{
				Ban();
			}
			itr++;
			delete pi;
		}
		lst.clear();
#endif
	}
}

void Component::UpdateConfig()
{
	FILE *fp = fopen("config.cfg", "w+");
	std::wstringstream ss;
	ss << path.c_str() << L",";
	ss << username.c_str() << L",";
	ss << passwordHash.c_str() << L",";
	ss << position << L",";
	ss << banCheck << L",";
	ss << lastLine.c_str() << L"\n";
	std::string enc = Encrypt(StringUtil::Trim(StringUtil::ToUtf8(ss.str())));
	fwprintf(fp, StringUtil::FromUtf8(enc).c_str());
	fclose(fp);
}

uint32 Component::UpdateToWeb()
{
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if(curl)
	{
		/* First set the URL that is about to receive our POST. This URL can
		   just as well be a https:// URL if that is what should receive the
		   data. */
#define USEPOST
#ifdef USEPOST
		curl_easy_setopt(curl, CURLOPT_URL, "http://dk.tf2tr.com/stat/statupdate.php");
#endif

		/* Now specify the POST data */
		std::wstringstream postData;
		postData << L"&killCount=";
		postData << killCount;
		postData << L"&critKillCount=";
		postData << critKillCount;
		postData << L"&deathCount=";
		postData << deathCount;
		postData << L"&captureCount=";
		postData << captureCount;
		postData << L"&defendCount=";
		postData << defendCount;
		postData << L"&suicideCount=";
		postData << suicideCount;
		std::set<KillBy*>::iterator itr = lstKillBy.begin();
		for(; itr != lstKillBy.end(); itr++)
		{
			KillBy *kb = (*itr);
			postData << L"&k";
			postData << kb->weaponName;
			postData << L"=";
			postData << kb->killCount << L"," << kb->critKillCount;
		}
		std::set<DeathBy*>::iterator itrD = lstDeathBy.begin();
		for(; itrD != lstDeathBy.end(); itrD++)
		{
			DeathBy *db = (*itrD);
			postData << L"&d";
			postData << db->weaponName;
			postData << L"=";
			postData << db->deathCount;
		}

		postData << L"&maps=";
		std::map<std::wstring, MapItem*>::iterator itrM = maps.begin();
		for(; itrM != maps.end(); itrM++)
		{
			MapItem *mi = itrM->second;
			postData << "\t" << itrM->first << L"\t,";
			postData << mi->killCount << L",";
			postData << mi->deathCount << L",";
			postData << mi->captureCountBlue << L",";
			postData << mi->defendCountBlue << L",";
			postData << mi->captureCountRed << L",";
			postData << mi->defendCountRed << L",";
			postData << mi->lastPlayed << L",";
			postData << mi->totalTime << L",";
		}
		postData << L"&pkd=";
		std::map<std::wstring, PlayerItem*>::iterator itrP = pkdCount.begin();
		for(; itrP != pkdCount.end(); itrP++)
		{
			PlayerItem *pi = itrP->second;
			postData << "\t" << itrP->first << L"\t,";
			postData << pi->killCount << L",";
			postData << pi->critKillCount << L",";
			postData << pi->deathCount << L",";
		}
		m_banLock.Acquire();
		if(banned)
		{
			postData << L"&banned=";
			postData << banCheck;
		}
		m_banLock.Release();
		if(passwordHash != L"")
		{
			postData << L"&u=";
			postData << username;
			postData << L"&ph=";
			postData << passwordHash;
		}
		else if(username != L"" && password != L"")
		{
			postData << L"&u=";
			postData << username;
			postData << L"&p=";
			postData << password;
		}
		postData << L"&ed=";
		time_t tv = time(NULL);
		postData << tv;

		//md5 part
		std::wstring wstr;
		std::wstringstream wstrs;
		wstrs << postData.str();
		wstrs << L":" << tv << L":";
		std::string str = StringUtil::Trim(StringUtil::ToUtf8(wstrs.str()));
		std::string md5s = wMD5->getHashFromString(str);
		wstr = StringUtil::FromUtf8(md5s);
		postData << L"&ru=";
		postData << wstr;
		std::string ret;

#ifndef USEPOST
		std::wstringstream urlReal;
		urlReal << L"http://dk.tf2tr.com/stat/statupdate.php?";
		urlReal << postData.str();
		std::string urlReala = StringUtil::Trim(StringUtil::ToUtf8(urlReal.str()));;
		curl_easy_setopt(curl, CURLOPT_URL, urlReala.c_str());
#else
		std::string queryString = StringUtil::Trim(StringUtil::ToUtf8(postData.str()));
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, queryString.c_str());
		//curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, wcslen(postData.str().c_str())); 
#endif
#undef USEPOST
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Component::write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);

		if(ret.size() != 0)
		{
			std::ofstream fs("error.log");
			fs.write(ret.c_str(), ret.size());
			fs.flush();
			fs.close();
		}

		/* always cleanup */
		curl_easy_cleanup(curl);

		if(res == CURLE_OK)
		{
			killCount = 0;
			critKillCount = 0;
			deathCount = 0;
			captureCount = 0;
			defendCount = 0;

			std::set<KillBy*>::iterator itr = lstKillBy.begin();
			for(; itr != lstKillBy.end();)
			{
				KillBy *kb = (*itr);
				itr++;
				delete kb;
			}
			lstKillBy.clear();

			std::set<DeathBy*>::iterator itrD = lstDeathBy.begin();
			for(; itrD != lstDeathBy.end();)
			{
				DeathBy *kb = (*itrD);
				itrD++;
				delete kb;
			}
			lstDeathBy.clear();

			std::map<std::wstring, MapItem*>::iterator itrM = maps.begin();
			for(; itrM != maps.end();)
			{
				MapItem *mi = itrM->second;
				itrM++;
				if(mi->name != currentMapItem->name)
					delete mi;
				else
				{
					mi->captureCountBlue = 0;
					mi->captureCountRed = 0;
					mi->deathCount = 0;
					mi->defendCountBlue = 0;
					mi->defendCountRed = 0;
					mi->killCount = 0;
					mi->lastPlayed = L"";
					mi->totalTime = 0;
				}
			}
			maps.clear();
			if(currentMapItem)
				maps.insert(std::make_pair(currentMapItem->name, currentMapItem));

			std::map<std::wstring, PlayerItem*>::iterator itrP = pkdCount.begin();
			for(; itrP != pkdCount.end();)
			{
				PlayerItem *pi = itrP->second;
				itrP++;
				delete pi;
			}
			pkdCount.clear();

			return 0;
		}
		else
			return 1;
		
	}
	return 1;
}

void Component::UpdateMapTime()
{
	if(currentMapItem)
	{
		time_t ttft = GetSeconds(lastDate);
		double diff = difftime(ttft, currentDate);

		//Hacker fucker
		if(diff < 0)
			Ban();

		currentMapItem->totalTime += diff;
		currentMapItem->lastPlayed = lastDate;
		currentDate = ttft;
	}
}

INT_PTR CALLBACK Component::UpdateDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_PATHDIALOG_PATH, sComponent.GetPath().c_str());
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == ID_PATHDIALOG_OK)
		{

		}
		if(LOWORD(wParam) == ID_PATHDIALOG_OK || LOWORD(wParam) == ID_PATHDIALOG_CANCEL)
			EndDialog(hDlg, LOWORD(wParam));
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

size_t Component::write_data(char *data, size_t size, size_t nmemb, std::string *bufferdata)
{
    int result = 0;

    // Is there anything in the buffer?
    if (bufferdata != NULL)
    {
        // Append the data to the buffer
        bufferdata->append (data, size * nmemb);

        // How much did we write?
        result = size * nmemb;
    }
    return result;
}

size_t Component::write_data_file(char *data, size_t size, size_t nmemb, std::ofstream *bufferdata)
{
    int result = 0;

    // Is there anything in the buffer?
    if (bufferdata != NULL)
    {
		size_t total = size * nmemb;
        // Append the data to the buffer
		bufferdata->write(data, total);

        // How much did we write?
        result = size * nmemb;
    }
    return result;
}

size_t Component::write_data_file_s(char *data, size_t size, size_t nmemb, ProgressBar *bufferdata)
{
    int result = 0;

    // Is there anything in the buffer?
    if (bufferdata != NULL)
    {
		size_t total = size * nmemb;
        // Append the data to the buffer
		bufferdata->ofs->write(data, total);
		bufferdata->len += total;
		printf("%u\n", bufferdata->len);
		SendMessage(bufferdata->prog, PBM_SETPOS, (bufferdata->len*100)/bufferdata->total, NULL);
		std::wstringstream ss;
		ss << bufferdata->len << "/" << bufferdata->total;
		SetDlgItemText(bufferdata->dlg, 1008, ss.str().c_str());
		InvalidateRect(bufferdata->dlg, NULL, TRUE);
		UpdateWindow(bufferdata->dlg);

		result = total;
    }
    return result;
}

bool Component::CheckUpdate(HINSTANCE hInst, HWND hWnd)
{
	bool ret = false;
	HWND dlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_UPDATE_DIALOG), hWnd, Component::UpdateDialog);
	ShowWindow(dlg, SW_SHOW);
	HWND prog = GetDlgItem(dlg, IDC_PROGRESS);

	CURL *curl;
	CURLcode res;

	std::vector<UpdateItem*> items;
	uint32 totalLength = 0;
	curl = curl_easy_init();
	if(curl)
	{
		std::stringstream urlu;
		urlu << "http://www.sombrenuit.org/stat/update/update.html?t=";
		urlu << time(NULL);
		curl_easy_setopt(curl, CURLOPT_URL, urlu.str().c_str());

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Component::write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);

		/* always cleanup */
		curl_easy_cleanup(curl);

		if(res == CURLE_OK)
		{
			std::vector<std::string> results;
			StringUtil::SplitString(data, ",", results, true);

			updateLink = results[0];
			registry_int<int> val(L"Software\\Sombrenuit\\Steam Parser\\FirstInstall", HKEY_LOCAL_MACHINE);
			val = atoi(results[1].c_str());

			UpdateItem *item = new UpdateItem();
			for(int32 i = 6; i < results.size();)
			{
				item->name = results[i];
				item->version = results[i + 1];
				item->length = atoi(results[i + 2].c_str());

				std::map<std::string, std::string>::iterator itrVer = versionMap.find(item->name);
				if(itrVer != versionMap.end())
				{
					if(item->version == itrVer->second)
					{
						delete item;
						if(i != results.size())
							item = new UpdateItem();
					}
					item->newItem = false;
				}
				else //new item? Add support?
				{
					item->newItem = true;
				}

				items.push_back(item);
				totalLength += item->length;
				i += 3;
				if(i != results.size())
					item = new UpdateItem();
			}

			SendMessage(prog, PBM_SETRANGE, NULL, MAKELPARAM(0, 100));
			SendMessage(prog, PBM_SETPOS, 0, NULL);
			SendMessage(prog, PBM_STEPIT, 0, 0L);
		}
	}

	
	uint32 currentPos = 0;
	//Downloading files from list
	std::vector<UpdateItem*>::iterator itr = items.begin();
	for(; itr != items.end(); ++itr)
	{
		ret = true;
		curl = NULL;
		curl = curl_easy_init();
		if(curl)
		{
			std::stringstream url;
			url << "http://www.sombrenuit.org/stat/update/";
			url << (*itr)->name;
			url << "?t=";
			url << time(NULL);
			curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());

			if(!(*itr)->newItem)
				rename((*itr)->name.c_str(), ((*itr)->name + "o").c_str());
			std::ofstream ofs((*itr)->name.c_str());
			if(!ofs)
			{
				if(!(*itr)->newItem)
					rename(((*itr)->name + "o").c_str(), (*itr)->name.c_str());
				MessageBox(hWnd, L"Error on update", L"Please restart the program, possible issue internet connection.", 0);
				continue;
			}

			ProgressBar pb;
			pb.ofs = &ofs;
			pb.dlg = dlg;
			pb.prog = prog;
			pb.len = currentPos;
			pb.total = totalLength;
			std::wstring wname = StringUtil::FromUtf8((*itr)->name);
			SetDlgItemText(dlg, 1007, wname.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Component::write_data_file_s);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pb);

			/* Perform the request, res will get the return code */
			res = curl_easy_perform(curl);
			
			//Set position
			currentPos = pb.len;

			/* always cleanup */
			curl_easy_cleanup(curl);

			ofs.flush();
			ofs.close();
			if(!(*itr)->newItem)
				remove(((*itr)->name + "o").c_str());

			currentPos += (*itr)->length;
			SendMessage(prog, PBM_SETPOS, currentPos, NULL);
		}
	}

	itr = items.begin();
	for(; itr != items.end();)
	{
		UpdateItem *ui = (*itr);
		itr++;
		delete ui;
	}
	items.clear();

	EndDialog(dlg, NULL);

	return ret;
}

void Component::ParseW(std::wstring line, std::vector<std::wstring> results, std::wstring date)
{
	for(int32 i = 0; i < results.size(); i++)
	{
		int32 currentPlace = i;
		std::wstring str = results[i];
		if(!nameSearch)
		{
			if(str == L"killed")
			{
				std::vector<std::wstring>::iterator itrName = results.begin();
				int32 count = 0;
				std::wstring name;
				for(; itrName != results.end(); itrName++)
				{
					if(count == currentPlace)
						break;
					count++;
					if(count == currentPlace)
						name = name + (*itrName);
					else
						name = name + (*itrName) + L" ";
				}
				if(name != nick)
				{
					bool crit = false;
					std::wstring weaponName = L"";
					if(results[results.size() - 1] == L"(crit)")
					{
						int32 searchTill = results.size() - 2;
						weaponName = results[searchTill].substr(0, results[searchTill].length() - 1);
						crit = true;
					}
					else
					{
						int32 searchTill = results.size() - 1;
						weaponName = results[searchTill].substr(0, results[searchTill].length() - 1);
					}

					itrName = results.begin();
					count = 0;

					int32 total = results.size() - 3;
					if(crit)
						total--;				

					std::wstring dname;
					for(; itrName != results.end(); itrName++)
					{
						if(count > total)
							break;
						if(count > currentPlace)
						{
							if(count == total)
								dname = dname + (*itrName);
							else
								dname = dname + (*itrName) + L" ";
						}
						count++;
					}
					if(dname == nick)
					{
						std::map<std::wstring, PlayerItem*>::iterator itrPK = pkdCount.find(name);
						if(itrPK != pkdCount.end())
							itrPK->second->deathCount++;
						else
						{
							PlayerItem *pi = new PlayerItem();
							pi->killCount = 0;
							pi->deathCount = 1;
							pkdCount.insert(std::make_pair(name, pi));						
						}

						bool found = false;
						std::set<DeathBy*>::iterator itr = lstDeathBy.begin();
						for(; itr != lstDeathBy.end(); ++itr)
						{
							DeathBy *db = (*itr);
							if(db->weaponName == weaponName)
							{
								db->deathCount++;
								found = true;
								break;
							}
						}
						if(!found)
						{
							DeathBy *db = new DeathBy();
							db->weaponName = weaponName;
							db->deathCount = 1;
							lstDeathBy.insert(db);
						}
						deathCount++;
						if(currentMapItem)
							currentMapItem->deathCount++;
					}
					return;
				}
				std::wstring weaponName = L"";
				bool crit = false;
				if(results[results.size() - 1] == L"(crit)")
				{
					weaponName = results[results.size() - 2].substr(0, results[results.size() - 2].length() - 1);
					critKillCount++;
					crit = true;
				}
				else
					weaponName = results[results.size() - 1].substr(0, results[results.size() - 1].length() - 1);

				//Lets look who we killed
				itrName = results.begin();
				count = 0;

				int32 total = results.size() - 3;
				if(crit)
					total--;				

				std::wstring kname;
				for(; itrName != results.end(); itrName++)
				{
					if(count > total)
						break;
					if(count > currentPlace)
					{
						if(count == total)
							kname = kname + (*itrName);
						else
							kname = kname + (*itrName) + L" ";
					}
					count++;
				}

				std::map<std::wstring, PlayerItem*>::iterator itrPK = pkdCount.find(kname);
				if(itrPK != pkdCount.end())
				{
					itrPK->second->killCount++;
					if(crit)
						itrPK->second->critKillCount++;
				}
				else
				{
					PlayerItem *pi = new PlayerItem();
					pi->killCount = 1;
					if(crit)
						pi->critKillCount = 1;
					else
						pi->critKillCount = 0;
					pi->deathCount = 0;
					pkdCount.insert(std::make_pair(kname, pi));						
				}

				//Ok we can add our kill by now
				bool found = false;
				std::set<KillBy*>::iterator itr = lstKillBy.begin();
				for(; itr != lstKillBy.end(); ++itr)
				{
					KillBy *kb = (*itr);
					if(kb->weaponName == weaponName)
					{
						kb->killCount++;
						if(crit)
							kb->critKillCount++;
						found = true;
						break;
					}
				}
				if(!found)
				{
					KillBy *kb = new KillBy();
					kb->weaponName = weaponName;
					kb->killCount = 1;
					if(crit)
						kb->critKillCount = 1;
					else
						kb->critKillCount = 0;
					lstKillBy.insert(kb);
				}
				killCount++;
				if(currentMapItem)
					currentMapItem->killCount++;
			}
			else if(str == L"captured")
			{
				bool found = false;
				std::vector<std::wstring>::iterator itrName = results.begin();
				int32 count = 0;
				std::wstring name;
				for(; itrName != results.end(); itrName++)
				{
					if(count == currentPlace)
						break;
					count++;
					if(count == currentPlace)
					{
						name = name + (*itrName);
						if(name.length() > 1/* && name[name.length() - 1] == L','*/)
						{
							if(name == nick)
								found = true;
							name = L"";
						}
					}
					else
					{
						name = name + (*itrName) + L" ";
						if(name.length() > 2 && name[name.length() - 2] == L',')
						{
							if((name.substr(0, (name.size() - 2))) == nick)
								found = true;
							name = L"";
						}
					}
				}
				if(name == nick)
					found = true;
				if(!found)
					return;
				captureCount++;
				if(results[results.size() - 1] == L"#3") //Team blue
				{
					if(currentMapItem)
						currentMapItem->captureCountBlue++;
				}
				else
				{
					if(currentMapItem)
						currentMapItem->captureCountRed++;
				}
			}
			else if(str == L"defended")
			{
				bool found = false;
				std::vector<std::wstring>::iterator itrName = results.begin();
				int32 count = 0;
				std::wstring name;
				for(; itrName != results.end(); itrName++)
				{
					if(count == currentPlace)
						break;
					count++;
					if(count == currentPlace)
					{
						name = name + (*itrName);
						if(name.length() > 1 && name[name.length() - 1] == L',')
						{
							if(name == nick)
								found = true;
							name = L"";
						}
					}
					else
					{
						name = name + (*itrName) + L" ";
						if(name.length() > 2 && name[name.length() - 2] == L',')
						{
							if(name == nick)
								found = true;
							name = L"";
						}
					}
				}
				if(name == nick)
					found = true;
				if(!found)
					return;
				defendCount++;
				if(results[results.size() - 1] == L"#3") //Team blue
				{
					if(currentMapItem)
						currentMapItem->defendCountBlue++;
				}
				else
				{
					if(currentMapItem)
						currentMapItem->defendCountRed++;
				}
			}
			else if(str == L"suicided.")
			{
				std::vector<std::wstring>::iterator itrName = results.begin();
				int32 count = 0;
				std::wstring name;
				for(; itrName != results.end(); itrName++)
				{
					if(count == currentPlace)
						break;
					count++;
					if(count == currentPlace)
					{
						name = name + (*itrName);
					}
				}
				if(name != nick)
					return;
				suicideCount++;
			}
		}
		else
		{
			if(str == L"connected")
			{
				nameSearch = false;
				std::vector<std::wstring>::iterator itrName = results.begin();
				int32 count = 0;
				std::wstring name;
				for(; itrName != results.end(); itrName++)
				{
					if(count == currentPlace)
						break;
					count++;
					if(count == currentPlace)
						name = name + (*itrName);
					else
						name = name + (*itrName) + L" ";
				}
				nick = name;
				//Set join date
				tm ttf;
				std::vector<std::wstring> resultsDate;
				StringUtil::SplitStringW(date, L"-", resultsDate, true);
				std::vector<std::wstring> resultsDateLeft;
				StringUtil::SplitStringW(resultsDate[0], L"/", resultsDateLeft, true);
				ttf.tm_mon = _wtoi(resultsDateLeft[0].c_str());
				ttf.tm_mday = _wtoi(resultsDateLeft[1].c_str());
				ttf.tm_year = _wtoi(resultsDateLeft[2].c_str()) - 1900;
				std::vector<std::wstring> resultsDateRight;
				StringUtil::SplitStringW(resultsDate[1], L":", resultsDateRight, true);
				ttf.tm_hour = _wtoi(resultsDateRight[0].c_str());
				ttf.tm_min = _wtoi(resultsDateRight[1].c_str());
				ttf.tm_sec = _wtoi(resultsDateRight[2].c_str());
				ttf.tm_wday = 0;
				ttf.tm_yday = 0;
				time_t ttft = mktime(&ttf);
				currentDate = ttft;
				//std::string time = asctime(&ttf);
				/*if(currentDate != 0)
				{
					lastMapItem
					lastDate = currentDate;
					currentDate = ttft;
					dateSend = false;
				}
				else
					currentDate = ttft;*/
			}
		}
	}
}

void Component::InitFileWatcher()
{
	/*fw = new COXFileWatcher();
	fw->AddWatch(path, false, OXFileWatchChangeSize);*/
}

void Component::DisposeFileWatcher()
{
	/*if(fw != NULL)
	{
		fw->RemoveAllWatches();
		delete fw;
		fw = NULL;
	}*/
}

time_t Component::GetSeconds(std::wstring date)
{
	tm ttf;
	std::vector<std::wstring> resultsDate;
	StringUtil::SplitStringW(date, L"-", resultsDate, true);
	std::vector<std::wstring> resultsDateLeft;
	StringUtil::SplitStringW(resultsDate[0], L"/", resultsDateLeft, true);
	ttf.tm_mon = _wtoi(resultsDateLeft[0].c_str());
	ttf.tm_mday = _wtoi(resultsDateLeft[1].c_str());
	ttf.tm_year = _wtoi(resultsDateLeft[2].c_str()) - 1900;
	std::vector<std::wstring> resultsDateRight;
	StringUtil::SplitStringW(resultsDate[1], L":", resultsDateRight, true);
	ttf.tm_hour = _wtoi(resultsDateRight[0].c_str());
	ttf.tm_min = _wtoi(resultsDateRight[1].c_str());
	ttf.tm_sec = _wtoi(resultsDateRight[2].c_str());
	ttf.tm_wday = 0;
	ttf.tm_yday = 0;
	return mktime(&ttf);
}

void Component::SetPath(std::string value)
{
	path = StringUtil::FromUtf8(value);
}

bool Component::IsRunning(std::wstring exe)
{
	unsigned long aProcesses[1024], cbNeeded, cProcesses;
	if(!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		return false;

	cProcesses = cbNeeded / sizeof(unsigned long);
	for(unsigned int i = 0; i < cProcesses; i++)
	{
		if(aProcesses[i] == 0)
			continue;

		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, aProcesses[i]);
		LPWSTR buffer = new WCHAR[50];
		GetModuleBaseName(hProcess, 0, buffer, 50);
		DWORD pid = GetProcessId(hProcess);
		CloseHandle(hProcess);
		if(exe == std::wstring(buffer))
		{
			delete [] buffer;
			return true;
		}
		delete [] buffer;
	}
	return false;
}

void Component::Ban()
{
	m_banLock.Acquire();
	banned = true;
	banCheck = time(NULL);
	registry_int<int> ban(L"Software\\Sombrenuit\\Steam Parser\\T", HKEY_LOCAL_MACHINE);
	ban = banCheck;
	m_banLock.Release();
}

std::string Component::Encrypt(std::string str)
{
    // Key and IV setup
    byte key[ CryptoPP::AES::DEFAULT_KEYLENGTH ] =
        { 0xA2, 0x39, 0xC2, 0x21, 0xA6, 0x31, 0x67, 0x9A,
          0xCD, 0xA1, 0xA9, 0x73, 0x5A, 0x7D, 0x13, 0xB2 };

    byte  iv[ CryptoPP::AES::BLOCKSIZE ] =
        { 0x1A, 0x3C, 0xAC, 0xBB, 0x19, 0x9D, 0x8A, 0x84,
          0x49, 0xCB, 0x2A, 0xAA, 0xC8, 0xB1, 0x65, 0x48 };


    // Message M
    const std::string PlainText = str;

    // Pseudo Random Number Generator
    CryptoPP::AutoSeededRandomPool rng;

    // Encryptor
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption
        Encryptor( key, sizeof(key), iv );

	//Normal Encryption
    /*// Encryption
    CryptoPP::StringSource( PlainText, true,
        new CryptoPP::StreamTransformationFilter( Encryptor,
            new CryptoPP::Base32Encoder(
                new CryptoPP::StringSink( EncodedText )
            ) // Base32Encoder
        ) // StreamTransformationFilter
    ); // StringSource*/

    ///////////////////////////////////////////
    //            Generation Loop            //
    ///////////////////////////////////////////

    std::string EncodedText = "";
    std::string SaltText = "";
    Encryptor.Resynchronize( iv );

    // Salt
    CryptoPP::RandomNumberSource( rng, 4, true,
        new CryptoPP::StringSink( SaltText )
    ); // RandomNumberSource

    // Encryption
    CryptoPP::StringSource( SaltText + PlainText, true,
        new CryptoPP::StreamTransformationFilter( Encryptor,
            new CryptoPP::Base32Encoder(
                new CryptoPP::StringSink( EncodedText ),
            true, 4, "-") // Base32Encoder
        ) // StreamTransformationFilter
    ); // StringSource

	EncodedText += "RU";

	return EncodedText;
}

std::string Component::Decrypt(std::string str)
{
    ///////////////////////////////////////
    //                DMZ                //
    ///////////////////////////////////////

	std::string EncodedText = str.substr( 0, str.length() - 2 );

    // Key and IV setup
    byte key[ CryptoPP::AES::DEFAULT_KEYLENGTH ] =
        { 0xA2, 0x39, 0xC2, 0x21, 0xA6, 0x31, 0x67, 0x9A,
          0xCD, 0xA1, 0xA9, 0x73, 0x5A, 0x7D, 0x13, 0xB2 };

    byte  iv[ CryptoPP::AES::BLOCKSIZE ] =
        { 0x1A, 0x3C, 0xAC, 0xBB, 0x19, 0x9D, 0x8A, 0x84,
          0x49, 0xCB, 0x2A, 0xAA, 0xC8, 0xB1, 0x65, 0x48 };

    // Message M
    const std::string PlainText = str;

    // Pseudo Random Number Generator
    CryptoPP::AutoSeededRandomPool rng;

    // Decryptior
    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption
        Decryptor( key, sizeof(key), iv );

    ///////////////////////////////////////////
    //            Generation Loop            //
    ///////////////////////////////////////////

    std::string SaltText = "";
    Decryptor.Resynchronize( iv );

    // Salt
    CryptoPP::RandomNumberSource( rng, 4, true,
        new CryptoPP::StringSink( SaltText )
    ); // RandomNumberSource


    // Recovered Text Sink
    std::string RecoveredText;

    CryptoPP::StringSource( EncodedText, true,
        new CryptoPP::Base32Decoder(
            new CryptoPP::StreamTransformationFilter( Decryptor,
                new CryptoPP::StringSink( RecoveredText )
            ) // StreamTransformationFilter
        ) // Base32Decoder
    ); // StringSource
	
	return RecoveredText.substr( 4 );
}

void Component::RunExecutable(std::wstring name, std::wstring params)
{
	SHELLEXECUTEINFO sei;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_UNICODE /*| SEE_MASK_NO_CONSOLE*/;
	sei.nShow = SW_HIDE; 
	sei.lpVerb = NULL;
	sei.lpFile = name.c_str();
	sei.lpParameters = params.c_str();
	ShellExecuteEx(&sei);
}

//Task

void TaskList::AddTask(Task * task)
{
	queueLock.Acquire();
	tasks.insert(task);
	queueLock.Release();
}

Task * TaskList::GetTask()
{
	queueLock.Acquire();

	Task* t = 0;
	for(std::set<Task*>::iterator itr = tasks.begin(); itr != tasks.end(); ++itr)
	{
		if(!(*itr)->in_progress)
		{
			t = (*itr);
			t->in_progress = true;
			break;
		}
	}
	queueLock.Release();
	return t;
}

void TaskList::spawn()
{
	running = true;
	thread_count = 0;

	uint32 threadcount;
	if(/*Config.MainConfig.GetBoolDefault("Startup", "EnableMultithreadedLoading", true)*/true)
	{
		// get processor count
#ifndef WIN32
#if UNIX_FLAVOUR == UNIX_FLAVOUR_LINUX
#ifdef X64
		threadcount = 2;
#else
		long affmask;
		sched_getaffinity(0, 4, (cpu_set_t*)&affmask);
		threadcount = (BitCount8(affmask)) * 2;
		if(threadcount > 8) threadcount = 8;
		else if(threadcount <= 0) threadcount = 1;
#endif
#else
		threadcount = 2;
#endif
#else
		SYSTEM_INFO s;
		GetSystemInfo(&s);
		threadcount = s.dwNumberOfProcessors * 2;
		if(threadcount>8)
			threadcount=8;
#endif
	}
	else
		threadcount = 1;

	/*Log.Line();
	Log.Notice("World", "Beginning %s server startup with %u threads.", (threadcount == 1) ? "progressive" : "parallel", threadcount);
	Log.Line();*/

	for(uint32 x = 0; x < threadcount; ++x)
		ThreadPool.ExecuteTask(new TaskExecutor(this));
}

void TaskList::wait()
{
	bool has_tasks = true;
	while(has_tasks)
	{
		queueLock.Acquire();
		has_tasks = false;
		for(std::set<Task*>::iterator itr = tasks.begin(); itr != tasks.end(); ++itr)
		{
			if(!(*itr)->completed)
			{
				has_tasks = true;
				break;
			}
		}
		queueLock.Release();
		Sleep(20);
	}
}

void TaskList::kill()
{
	running = false;
}

void Task::execute()
{
	_cb->execute();
}

bool TaskExecutor::run()
{
	Task * t;
	try
	{
		while(starter->running)
		{
			t = starter->GetTask();
			if(t)
			{
				t->execute();
				t->completed = true;
				starter->RemoveTask(t);
				delete t;
			}
			else
				Sleep(20);
		}
	}
	catch(...)
	{
	}
	return true;
}

void TaskList::waitForThreadsToExit()
{
	while(thread_count)
	{
		Sleep(20);
	}
}
