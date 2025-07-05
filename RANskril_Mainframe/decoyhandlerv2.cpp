#include "decoyhandlerv2.h"
#include "commons.h"
#include "directoryhandlerv2.h"
#include "utils.h"

#include "resource.h"


/* * * * * * * * * */
/* FILE SIGNATURES */
/* * * * * * * * * */

std::unordered_map<std::wstring, std::pair<std::vector<BYTE>, DWORD>> DecoyBuilder::signatures = {
	// --- IMAGES ---
	{L".png",  {{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}, 0}},
	{L".jpg",  {{0xFF, 0xD8, 0xFF, 0xE0}, 0}},
	{L".jpeg", {{0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01}, 0}},
	{L".gif",  {{0x47, 0x49, 0x46, 0x38, 0x37, 0x61}, 0}},
	{L".bmp",  {{0x42, 0x4D}, 0}},
	{L".tif",  {{0x49, 0x49, 0x2A, 0x00}, 0}},
	{L".tiff", {{0x4D, 0x4D, 0x00, 0x2A}, 0}},
	{L".ico",  {{0x00, 0x00, 0x01, 0x00}, 0}},
	{L".webp", {{0x52, 0x49, 0x46, 0x46, 0x00, 0x80, 0xFF, 0xFF, 0x57, 0x45, 0x42, 0x50}, 0}},
	{L".heic", {{0x66, 0x74, 0x79, 0x70, 0x68, 0x65, 0x69, 0x63}, 4}},

	// --- EXECUTABLES ---
	{L".exe",  {{0x4D, 0x5A}, 0}},
	{L".dll",  {{0x4D, 0x5A}, 0}},
	{L".sys",  {{0x4D, 0x5A}, 0}},
	{L".scr",  {{0x4D, 0x5A}, 0}},
	{L".mui",  {{0x4D, 0x5A}, 0}},
	{L".elf",  {{0x7F, 0x45, 0x4C, 0x46}, 0}},

	// --- ARCHIVES ---
	{L".zip",  {{0x50, 0x4B, 0x03, 0x04}, 0}},
	{L".rar",  {{0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x01, 0x00}, 0}},
	{L".7z",   {{0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C}, 0}},
	{L".gz",   {{0x1F, 0x8B}, 0}},
	{L".bz2",  {{0x42, 0x5A, 0x68}, 0}},
	{L".tar",  {{0x75, 0x73, 0x74, 0x61, 0x72, 0x00, 0x30, 0x30}, 257}},
	{L".aar",  {{0x50, 0x4B, 0x03, 0x04}, 0}},
	{L".apk",  {{0x50, 0x4B, 0x03, 0x04}, 0}},

	// --- DOCUMENTS ---
	{L".pdf",  {{0x25, 0x50, 0x44, 0x46, 0x2D}, 0}},
	{L".doc",  {{0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1}, 0}},
	{L".docx", {{0x50, 0x4B, 0x03, 0x04}, 0}},
	{L".xls",  {{0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1}, 0}},
	{L".xlsx", {{0x50, 0x4B, 0x03, 0x04}, 0}},
	{L".ppt",  {{0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1}, 0}},
	{L".pptx", {{0x50, 0x4B, 0x03, 0x04}, 0}},
	{L".odt",  {{0x50, 0x4B, 0x03, 0x04}, 0}},
	{L".ods",  {{0x50, 0x4B, 0x03, 0x04}, 0}},
	{L".odp",  {{0x50, 0x4B, 0x03, 0x04}, 0}},
	{L".epub", {{0x50, 0x4B, 0x03, 0x04}, 0}},
	{L".jar",  {{0x50, 0x4B, 0x03, 0x04}, 0}},

	// --- DATABASES ---
	{L".sqlite", {{0x53, 0x51, 0x4C, 0x69, 0x74, 0x65, 0x20, 0x66, 0x6F, 0x72, 0x6D, 0x61, 0x74, 0x20, 0x33, 0x00}, 0}},
	{L".sqlitedb", {{0x53, 0x51, 0x4C, 0x69, 0x74, 0x65, 0x20, 0x66, 0x6F, 0x72, 0x6D, 0x61, 0x74, 0x20, 0x33, 0x00}, 0}},
	{L".db", {{0x53, 0x51, 0x4C, 0x69, 0x74, 0x65, 0x20, 0x66, 0x6F, 0x72, 0x6D, 0x61, 0x74, 0x20, 0x33, 0x00}, 0}},
	{L".accdb", {{0x00, 0x01, 0x00, 0x00, 0x53, 0x74, 0x61, 0x6E, 0x64, 0x61, 0x72, 0x64, 0x20, 0x41, 0x43, 0x45, 0x20, 0x44, 0x42}, 0}},
	{L".mdb",   {{0x00, 0x01, 0x00, 0x00, 0x53, 0x74, 0x61, 0x6E, 0x64, 0x61, 0x72, 0x64, 0x20, 0x4A, 0x65, 0x74, 0x20, 0x44, 0x42}, 0}},

	// --- AUDIO/VIDEO ---
	{L".mp3",  {{0x49, 0x44, 0x33}, 0}},
	{L".wav",  {{0x52, 0x49, 0x46, 0x46, 0x00, 0x80, 0xFF, 0xFF, 0x57, 0x41, 0x56, 0x45}, 0}},
	{L".flac", {{0x66, 0x4C, 0x61, 0x43}, 0}},
	{L".ogg",  {{0x4F, 0x67, 0x67, 0x53}, 0}},
	{L".oga",  {{0x4F, 0x67, 0x67, 0x53}, 0}},
	{L".ogv",  {{0x4F, 0x67, 0x67, 0x53}, 0}},
	{L".avi",  {{0x52, 0x49, 0x46, 0x46, 0x00, 0x80, 0xFF, 0xFF, 0x41, 0x56, 0x49, 0x20}, 0}},
	{L".mp4",  {{0x66, 0x74, 0x79, 0x70, 0x4D, 0x53, 0x4E, 0x56}, 4}},
	{L".3gp",  {{0x66, 0x74, 0x79, 0x70, 0x33, 0x67}, 4}},
	{L".flv",  {{0x46, 0x4C, 0x56}, 0}},
	{L".mkv",  {{0x1A, 0x45, 0xDF, 0xA3}, 0}},
	{L".webm", {{0x1A, 0x45, 0xDF, 0xA3}, 0}},

	// --- MISCELLANOUS ---
	{L".iso",  {{0x43, 0x44, 0x30, 0x30, 0x31}, 0x8001}},
	{L".psd",  {{0x38, 0x42, 0x50, 0x53}, 0}},
	{L".msi",  {{0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1}, 0}},
	{L".xml",  {{0x3C, 0x3F, 0x78, 0x6D, 0x6C, 0x20}, 0}},
	{L".wim",  {{0x4D, 0x53, 0x57, 0x49, 0x4D, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x00}, 0}},
};


/* * * * * * * * * * * * * * * * * */
/* TOKENS FOR FILE NAME GENERATION */
/* * * * * * * * * * * * * * * * * */

// .txt, .rtf, .xls, .doc and any other member of this family should get away with this
const wchar_t* prefixes_docs[] = {
	L"invoices", L"invoice", L"personal", L"passwords", L"password", L"credentials", L"book", L"cv", L"ssn", L"cnp", L"info"
};
const wchar_t* suffixes_docs[] = {
	L"school", L"university", L"bank", L"for-later", L"work", L"job", L"facebook", L"twitter", L"github", L"linkedin", L"for-job", L"for-work", L"important", L"for-me"
};

// .mp4, .mkv and any other member of this family should get away with this
const wchar_t* prefixes_vids[] = {
	L"friends", L"family", L"game", L"anniversary"
};
const wchar_t* suffixes_vids[] = {
	L"video", L"special-video", L"reel", L"moment", L"youtube"
};

// .mp3, .ogg and any other member of this family should get away with this
const wchar_t* prefixes_music[] = {
	L"my_rock", L"my_midi", L"my_pop", L"my_rap"
};
const wchar_t* suffixes_music[] = {
	L"attempt", L"song", L"melody", L"lalala"
};

// .ppt only
const wchar_t* prefixes_ppts[] = {
	L"presentation", L"showcase", L"homework", L"material_for_raise", L"manager's_plan", L"info", L"backup"
};
const wchar_t* suffixes_ppts[] = {
	L"for-me", L"for-later", L"important", L"for-work", L"for-job", L"for-school", L"for-university"
};

// generics
const wchar_t* names_generics[] = {
	L"real", L"important", L"top", L"don't_forget", L"don't_touch_until_later", L"come_back_to_this", L"backup",
	L"for_me", L"for_later", L"important", L"for_work", L"for_job", L"for_school", L"for_university"
};

// prefixes to prepend to the filename, to control its placement
const wchar_t* arrow[] = {
	L"!!!", L"zzz",
	L"alpha", L"bravo", L"charlie", L"delta", L"echo",
	L"foxtrot", L"golf", L"hotel", L"india", L"juliett",
	L"kilo", L"lima", L"mike", L"november", L"oscar",
	L"papa", L"quebec", L"romeo", L"sierra", L"tango",
	L"uniform", L"victor", L"whiskey", L"xray", L"yankee", L"zulu",
};

std::unordered_map<std::wstring, WORD> resourcestringToType = {
	{L"DECOY_AVI", DECOY_AVI}, {L"DECOY_CSV", DECOY_CSV}, {L"DECOY_DOC", DECOY_DOC}, {L"DECOY_DOCX", DECOY_DOCX}, {L"DECOY_GIF", DECOY_GIF},
	{L"DECOY_HTML", DECOY_HTML}, {L"DECOY_JPG", DECOY_JPG}, {L"DECOY_JSON", DECOY_JSON}, {L"DECOY_MKV", DECOY_MKV}, {L"DECOY_MOV", DECOY_MOV},
	{L"DECOY_MP3", DECOY_MP3}, {L"DECOY_MP4", DECOY_MP4}, {L"DECOY_ODP", DECOY_ODP}, {L"DECOY_ODS", DECOY_ODS}, {L"DECOY_ODT", DECOY_ODT},
	{L"DECOY_OGG", DECOY_OGG}, {L"DECOY_PDF", DECOY_PDF}, {L"DECOY_PNG", DECOY_PNG}, {L"DECOY_PPT", DECOY_PPT}, {L"DECOY_RTF", DECOY_RTF},
	{L"DECOY_SVG", DECOY_SVG}, {L"DECOY_TIFF", DECOY_TIFF}, {L"DECOY_WAV", DECOY_WAV}, {L"DECOY_WEBM", DECOY_WEBM}, {L"DECOY_WEBP", DECOY_WEBP},
	{L"DECOY_WMV", DECOY_WMV}, {L"DECOY_XLS", DECOY_XLS}, {L"DECOY_XLSX", DECOY_XLSX}, {L"DECOY_XML", DECOY_XML}, {L"DECOY_ZIP", DECOY_ZIP}
};


/* * * * * * * * * * * * * * *  */
/* DECOY BUILDER IMPLEMENTATION */
/* * * * * * * * * * * * * * *  */

DecoyBuilder::DecoyBuilder()
{
	this->decoyFile.name = L"";
	this->decoyFile.path = L"";
	this->decoyFile.extension = L"";
	this->decoyFile.signature = std::pair<std::vector<BYTE>, DWORD>({}, 0);
	this->decoyFile.registryLocationKey = RegistryKey{};
}

DecoyBuilder::~DecoyBuilder() {}

void DecoyBuilder::Reset()
{
	this->decoyFile.name = L"";
	this->decoyFile.path = L"";
	this->decoyFile.extension = L"";
	this->decoyFile.signature = std::pair<std::vector<BYTE>, DWORD>({}, 0);
	this->decoyFile.registryLocationKey = RegistryKey{};
}

DecoyBuilder* DecoyBuilder::SetPath(std::wstring path)
{
	this->decoyFile.path = path;
	return this;
}

DecoyBuilder* DecoyBuilder::SetName(std::wstring name)
{
	this->decoyFile.name = name;
	return this;
}

DecoyBuilder* DecoyBuilder::SetExtension(std::wstring extension)
{
	this->decoyFile.extension = extension;
	auto it = signatures.find(extension);
	if (it != signatures.end())
		this->decoyFile.signature = std::pair<std::vector<BYTE>, DWORD>(it->second.first, it->second.second);
	else
		this->decoyFile.signature = std::pair<std::vector<BYTE>, DWORD>({}, 0);

	return this;
}

DecoyBuilder* DecoyBuilder::SetRegistryLocation(RegistryKey registryLocationKey)
{
	this->decoyFile.registryLocationKey = registryLocationKey;
	return this;
}

DecoyFile DecoyBuilder::Build()
{
	return this->decoyFile;
}


/* * * * * * * * * * * * * * * * * * *  */
/* DECOY FILE ACTUALIZER IMPLEMENTATION */
/* * * * * * * * * * * * * * * * * * *  */

DWORD DecoyFileActualizer::CreateDecoyInFilesystem(DecoyFile decoyFile)
{
	WCHAR logMessage[256];

	Utils::ImpersonateCurrentlyLoggedInUser();
	Utils::LogEvent(std::wstring(L"SWITCHED TO USER CONTEXT"), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

	HANDLE hTrapFile = CreateFileW((decoyFile.path + decoyFile.name).c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hTrapFile == INVALID_HANDLE_VALUE) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT CREATING DECOY FILE %ws", (decoyFile.path + decoyFile.name).c_str());
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		RevertToSelf();

		return errorCode;
	}

	std::wstring resourceName = L"DECOY_";
	std::wstring resourceExtension = decoyFile.extension;
	std::transform(resourceExtension.begin(), resourceExtension.end(), resourceExtension.begin(), ::toupper);
	resourceName += resourceExtension.substr(1);

	bool handledViaCache = true;
	do {
		HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourcestringToType[resourceName]), RT_RCDATA);
		if (!hRes) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO FIND %ws", resourceName.c_str());
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			handledViaCache = false;
			break;
		}

		HGLOBAL hLoadedRes = LoadResource(NULL, hRes);
		if (!hLoadedRes) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO LOAD %ws", resourceName.c_str());
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			handledViaCache = false;
			break;
		}

		DWORD size = SizeofResource(NULL, hRes);
		void* pData = LockResource(hLoadedRes);
		if (!pData) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO LOCK %ws", resourceName.c_str());
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			handledViaCache = false;
			break;
		}

		DWORD written = 0;
		BOOL ok = WriteFile(hTrapFile, pData, size, &written, NULL);
		if (!ok) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"FAILED TO WRITE FROM %ws TO %ws", resourceName.c_str(), (decoyFile.path + decoyFile.name).c_str());
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);

			handledViaCache = false;
			break;
		}

	} while (false);

	if (handledViaCache == true) {
		CloseHandle(hTrapFile);
		RevertToSelf();
		Utils::LogEvent(std::wstring(L"SWITCHED TO SERVICE CONTEXT"), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

		return ERROR_SUCCESS;
	}

	DWORD bytesWritten = 0;
	BOOL status;

	if (decoyFile.signature.second > 0) {
		BYTE* preSignatureData = new BYTE[decoyFile.signature.second];
		for (DWORD i = 0; i < decoyFile.signature.second; i++) {
			preSignatureData[i] = rand() % 256;
		}
		bytesWritten = 0;
		status = WriteFile(hTrapFile, preSignatureData, decoyFile.signature.second, &bytesWritten, NULL);
		if (!status) {
			int errorCode = GetLastError();
			RtlZeroMemory(logMessage, sizeof(logMessage));
			swprintf(logMessage, 256, L"ERROR AT WRITING PRE-SIGNATURE BYTES IN DECOY FILE %ws", (decoyFile.path + decoyFile.name).c_str());
			Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
			RevertToSelf();

			return errorCode;
		}
	}

	bytesWritten = 0;
	status = WriteFile(hTrapFile, decoyFile.signature.first.data(), decoyFile.signature.first.size(), &bytesWritten, NULL);
	if (!status) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT WRITING SIGNATURE BYTES IN DECOY FILE %ws", (decoyFile.path + decoyFile.name).c_str());
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		RevertToSelf();

		return errorCode;
	}

	DWORD maxSize = rand() % (1 << 18) + 1;
	char* buffer = new char[maxSize];
	for (DWORD i = 0; i < maxSize; i++) {
		buffer[i] = rand() % 256;
	}

	bytesWritten = 0;
	status = WriteFile(hTrapFile, buffer, maxSize, &bytesWritten, NULL);
	if (!status) {
		int errorCode = GetLastError();
		RtlZeroMemory(logMessage, sizeof(logMessage));
		swprintf(logMessage, 256, L"ERROR AT WRITING POST-SIGNATURE JUNK BYTES IN DECOY FILE %ws", (decoyFile.path + decoyFile.name).c_str());
		Utils::LogEvent(std::wstring(logMessage), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, errorCode, WINEVENT_LEVEL_ERROR);
		RevertToSelf();

		return errorCode;
	}

	CloseHandle(hTrapFile);
	RevertToSelf();
	Utils::LogEvent(std::wstring(L"SWITCHED TO SERVICE CONTEXT"), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

	return ERROR_SUCCESS;
}

DWORD DecoyFileActualizer::PlaceDecoyInsideRegistry(DecoyFile decoyFile)
{
	RegistryHandler& regHandler = RegistryHandler::GetInstance();
	RegistryKey origDecoyMonPath{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\RANskril\\DecoyMon" };
	RegistryValue pathsCount{ origDecoyMonPath, L"pathsCount", REG_DWORD, nullptr, 0 };

	DWORD oldPathsCountValue = 0;
	regHandler.RetrieveValue(pathsCount, &oldPathsCountValue, sizeof(oldPathsCountValue));
	++oldPathsCountValue;
	pathsCount.pValueBuffer = &oldPathsCountValue;
	pathsCount.valueSize = sizeof(oldPathsCountValue);
	regHandler.SetValue(pathsCount);

	DWORD one = 1;
	RegistryValue decoyPath{ decoyFile.registryLocationKey, decoyFile.path + decoyFile.name, REG_DWORD, &one, sizeof(one) };
	regHandler.SetValue(decoyPath);

	return ERROR_SUCCESS;
}


/* * * * * * * * * * * * * * *  */
/* DECOY HANDLER IMPLEMENTATION */
/* * * * * * * * * * * * * * *  */

DecoyHandler& DecoyHandler::GetInstance()
{
	static DecoyHandler instance;
	return instance;
}

DecoyHandler::DecoyHandler() : decoyBuilder(new DecoyBuilder()), decoyFileActualizer(new DecoyFileActualizer())
{
}

DecoyHandler::~DecoyHandler()
{
	delete this->decoyBuilder;
	delete this->decoyFileActualizer;
}

std::wstring DecoyHandler::GenerateDecoyName(std::wstring extension, DWORD prefixLocation)
{
	std::wstring fullName = L"";
	std::wstring prefix = std::wstring(arrow[prefixLocation % ARRAYSIZE(arrow)]);

	if (extension == L".doc" || extension == L".docx" || extension == L".pdf" || extension == L".txt" || extension == L".odt" || extension == L".xlsx" || extension == L".xls" || extension == L".ods") {
		std::wstring part1 = std::wstring(prefixes_docs[rand() % ARRAYSIZE(prefixes_docs)]);
		std::wstring part2 = std::wstring(suffixes_docs[rand() % ARRAYSIZE(suffixes_docs)]);
		fullName = prefix + L"_" + part1 + L"_" + part2 + extension;
	}
	else if (extension == L".mp3" || extension == L".wav" || extension == L".flac" || extension == L".ogg" || extension == L".oga" || extension == L".ogv") {
		std::wstring part1 = std::wstring(prefixes_music[rand() % ARRAYSIZE(prefixes_music)]);
		std::wstring part2 = std::wstring(suffixes_music[rand() % ARRAYSIZE(suffixes_music)]);
		fullName = prefix + L"_" + part1 + L"_" + part2 + extension;
	}
	else if (extension == L".mp4" || extension == L".mkv" || extension == L".avi" || extension == L".3gp" || extension == L".flv" || extension == L".webm") {
		std::wstring part1 = std::wstring(prefixes_vids[rand() % ARRAYSIZE(prefixes_vids)]);
		std::wstring part2 = std::wstring(suffixes_vids[rand() % ARRAYSIZE(suffixes_vids)]);
		fullName = prefix + L"_" + part1 + L"_" + part2 + extension;
	}
	else if (extension == L".ppt" || extension == L".pptx" || extension == L".odp") {
		std::wstring part1 = std::wstring(prefixes_ppts[rand() % ARRAYSIZE(prefixes_ppts)]);
		std::wstring part2 = std::wstring(suffixes_ppts[rand() % ARRAYSIZE(suffixes_ppts)]);
		fullName = prefix + L"_" + part1 + L"_" + part2 + extension;
	}
	else {
		std::wstring part0 = std::wstring(names_generics[rand() % ARRAYSIZE(names_generics)]);
		fullName = prefix + L"_" + part0 + extension;
	}

	return fullName;
}

DecoyFile DecoyHandler::PrepareHeadDecoy(std::wstring path, std::wstring extension, RegistryKey registryLocationKey)
{
	this->decoyBuilder->Reset();
	std::wstring name;
	DecoyFile file = this->decoyBuilder->SetPath(path)->SetName(std::wstring(L"!!!_index_folder") + extension)->SetExtension(extension)->SetRegistryLocation(registryLocationKey)->Build();
	return file;
}

DecoyFile DecoyHandler::PrepareTailDecoy(std::wstring path, std::wstring extension, RegistryKey registryLocationKey) {
	this->decoyBuilder->Reset();
	std::wstring name;
	DecoyFile file = this->decoyBuilder->SetPath(path)->SetName(GenerateDecoyName(extension, 1))->SetExtension(extension)->SetRegistryLocation(registryLocationKey)->Build();
	return file;
}

DecoyFile DecoyHandler::PrepareMiddleDecoy(std::wstring path, std::wstring extension, RegistryKey registryLocationKey) {
	this->decoyBuilder->Reset();
	std::wstring name;
	DWORD position = rand() % ARRAYSIZE(arrow);
	if (position <= 1)
		position = 2;

	DecoyFile file = this->decoyBuilder->SetPath(path)->SetName(GenerateDecoyName(extension, position))->SetExtension(extension)->SetRegistryLocation(registryLocationKey)->Build();
	return file;
}

DWORD DecoyHandler::CreateDecoyInFilesystem(DecoyFile decoyFile)
{
	return decoyFileActualizer->CreateDecoyInFilesystem(decoyFile);
}

DWORD DecoyHandler::PlaceDecoyInsideRegistry(DecoyFile decoyFile)
{
	return decoyFileActualizer->PlaceDecoyInsideRegistry(decoyFile);
}

DWORD DecoyHandler::ArmSystem()
{
	RegistryHandler& registryHandler = RegistryHandler::GetInstance();
	DirectoryHandler& directoryHandler = DirectoryHandler::GetInstance();

	/* * * * * * * * * * * * * * */
	/* FIRST TIME INITIALIZATION */
	/* * * * * * * * * * * * * * */

	DWORD operationStatus;
	RegistryKey subkeyDecoyMonStruct{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\RANskril\\DecoyMon" };
	RegistryValue initializedStruct{ subkeyDecoyMonStruct, std::wstring(L"Initialized"), REG_DWORD, nullptr, 0 };
	DWORD newValue = 1;
	initializedStruct.pValueBuffer = &newValue;
	initializedStruct.valueSize = sizeof(DWORD);
	operationStatus = registryHandler.SetValue(initializedStruct);
	if (operationStatus != ERROR_SUCCESS) {
		Utils::LogEvent(std::wstring(L"FAILED TO SET INITIALIZATION STATUS"), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, operationStatus, WINEVENT_LEVEL_ERROR);
		return operationStatus;
	}

	Utils::ImpersonateCurrentlyLoggedInUser();
	std::vector<std::wstring> directories;
	for (auto& driveLetter : directoryHandler.RetrieveLogicalDosDrives()) {
		for (auto& path : directoryHandler.GetDirectories(driveLetter)) {
			directories.push_back(path);
		}
	}
	RevertToSelf();

	std::vector<DecoyFile> decoyFiles{};
	for (auto& path : directories) {
		auto extensions = directoryHandler.GetFileRatios(path);

		// convert the path to forward-slash, so it can be stored inside the registry
		std::wstring forwardSlashPath(path);
		std::replace(forwardSlashPath.begin(), forwardSlashPath.end(), L'\\', L'/');
		RegistryKey directoryKey{ HKEY_LOCAL_MACHINE, (std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths\\") + forwardSlashPath) };
		registryHandler.CreateKeyIfMissing(directoryKey);

		// create head decoy, always pdf
		DecoyFile headDecoy = PrepareHeadDecoy(path, L".pdf", directoryKey);
		//CreateDecoyInFilesystem(headDecoy);
		//PlaceDecoyInsideRegistry(headDecoy);
		decoyFiles.push_back(headDecoy);

		// create tail decoy, always of the same type as the type shared by most files, or jpg by default
		DWORD maxAppearances = 0;
		std::wstring tailExtension = L".jpg";
		std::vector<std::wstring> presentExtensions;
		for (auto& extension : extensions) {
			if (extension.first == L"all")
				continue;
			presentExtensions.push_back(extension.first);
			if (maxAppearances < extension.second) {
				tailExtension = extension.first;
				maxAppearances = extension.second;
			}
		}
		DecoyFile tailDecoy = PrepareTailDecoy(path, tailExtension, directoryKey);
		//CreateDecoyInFilesystem(tailDecoy);
		//PlaceDecoyInsideRegistry(tailDecoy);
		decoyFiles.push_back(tailDecoy);

		// foreach 16 files, create another decoy, with a random extension out of those inside the directory
		DWORD noFiles = extensions[L"all"];
		DWORD noDecoys = noFiles / 16;
		for (DWORD i = 0; i < noDecoys; i++) {
			DecoyFile middleDecoy = PrepareMiddleDecoy(path, presentExtensions[rand() % presentExtensions.size()], directoryKey);
			//CreateDecoyInFilesystem(middleDecoy);
			//PlaceDecoyInsideRegistry(middleDecoy);
			decoyFiles.push_back(middleDecoy);
		}
	}

	for (DecoyFile decoy : decoyFiles)
		CreateDecoyInFilesystem(decoy);

	for (DecoyFile decoy : decoyFiles)
		PlaceDecoyInsideRegistry(decoy);

	return ERROR_SUCCESS;
}

DWORD DecoyHandler::DisarmSystem()
{
	RegistryHandler& registryHandler = RegistryHandler::GetInstance();

	RegistryKey directoryKey{ HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths\\") };
	std::vector<std::wstring> armedDirectories = registryHandler.EnumerateKeysFromKey(directoryKey);
	for (std::wstring directory : armedDirectories) {
		RegistryKey subDirectoryKey{ HKEY_LOCAL_MACHINE, (std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths\\") + directory) };
		std::vector<std::wstring> decoys = registryHandler.EnumerateValuesFromKey(subDirectoryKey);
		for (std::wstring decoy : decoys) {
			DeleteFileW(decoy.c_str());
		}
		registryHandler.DeleteKey(subDirectoryKey);
	}

	RegistryKey subkeyDecoyMonStruct{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\RANskril\\DecoyMon" };
	RegistryValue initializedStruct{ subkeyDecoyMonStruct, std::wstring(L"Initialized"), REG_DWORD, nullptr, 0 };
	DWORD newValue = 0;
	initializedStruct.pValueBuffer = &newValue;
	initializedStruct.valueSize = sizeof(DWORD);
	registryHandler.SetValue(initializedStruct);


	RegistryValue pathsCount{ subkeyDecoyMonStruct, L"pathsCount", REG_DWORD, nullptr, 0 };
	DWORD oldPathsCountValue = 0;
	pathsCount.pValueBuffer = &oldPathsCountValue;
	pathsCount.valueSize = sizeof(oldPathsCountValue);
	registryHandler.SetValue(pathsCount);
	return ERROR_SUCCESS;
}

DWORD DecoyHandler::RefreshSystem()
{
	RegistryHandler& registryHandler = RegistryHandler::GetInstance();

	// Utils::ImpersonateCurrentlyLoggedInUser();
	// Utils::LogEvent(std::wstring(L"SWITCHED TO USER CONTEXT"), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

	SYSTEMTIME systemTime, changedTime;
	GetLocalTime(&systemTime);

	FILETIME fileTime;

	RegistryKey directoryKey{ HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths\\") };
	std::vector<std::wstring> armedDirectories = registryHandler.EnumerateKeysFromKey(directoryKey);
	for (std::wstring directory : armedDirectories) {
		RegistryKey subDirectoryKey{ HKEY_LOCAL_MACHINE, (std::wstring(L"SOFTWARE\\RANskril\\DecoyMon\\Paths\\") + directory) };
		std::vector<std::wstring> decoys = registryHandler.EnumerateValuesFromKey(subDirectoryKey);
		for (std::wstring decoy : decoys) {
			Utils::LogEvent(std::wstring(L"TRYING TO OPEN DECOY: ") + decoy, SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, 0, WINEVENT_LEVEL_INFO);
			HANDLE hFile = CreateFile(decoy.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

			changedTime = systemTime;
			changedTime.wHour = max(0, systemTime.wHour - ((rand() % 6) + 1));
			changedTime.wMinute = max(0, systemTime.wMinute - ((rand() % 30) + 1));
			changedTime.wDay = max(1, systemTime.wDay - (rand() % 3));

			BOOL status = SystemTimeToFileTime(&changedTime, &fileTime);
			if (!status) {
				CloseHandle(hFile);
				continue;
			}

			FILETIME localCreationTime;
			GetFileTime(hFile, &localCreationTime, nullptr, nullptr);
			BOOL comparisonResult = CompareFileTime(&localCreationTime, &fileTime);  // lCT < fT => -1, lCT > fT => 1

			comparisonResult == -1 ? SetFileTime(hFile, NULL, &fileTime, &fileTime) : SetFileTime(hFile, NULL, &localCreationTime, &localCreationTime);
			CloseHandle(hFile);
		}
	}

	// RevertToSelf();
	// Utils::LogEvent(std::wstring(L"SWITCHED TO SERVICE CONTEXT"), SERVICE_IDENTIFIER, DECOY_SUBIDENTIFIER, ERROR_SUCCESS, WINEVENT_LEVEL_INFO);

	return ERROR_SUCCESS;
}
