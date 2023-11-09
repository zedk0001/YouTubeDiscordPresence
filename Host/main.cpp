#include "Discord/API/discord.h"
#include <iostream>
#include <cstring>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include <ctime>
#include <map>

const int64_t APPLICATION_ID = 847682519214456862;
const std::string TITLE_IDENTIFIER = ":TITLE001:";
const std::string AUTHOR_IDENTIFIER = ":AUTHOR002:";
const std::string TIME_LEFT_IDENTIFIER = ":TIMELEFT003:";
const std::string END_IDENTIFIER = ":END004:";
const std::string IDLE_IDENTIFIER = "#*IDLE*#";
const int LIVESTREAM_TIME_ID = -1;
const int ACTIVITY_BUFFER_SIZE = 1024;

const bool LOGGING = false;

std::unique_ptr<discord::Core> core;
std::time_t elapsedTime = 0;

std::string previousTitle;
std::string previousAuthor;

// CREATE A DISCORD PRESENCE IF ONE DOESN'T ALREADY EXIST

bool createPresence(void) {
    if (core) {
        return true;
    }
    discord::Core* tempCore = nullptr;
    discord::Result result = discord::Core::Create(APPLICATION_ID, DiscordCreateFlags_NoRequireDiscord, &tempCore);
    if (LOGGING && result == discord::Result::Ok) {
        std::cout << "Discord presence has been created" << std::endl;
    }
    else if (LOGGING) {
        std::cout << "Failed to create Discord presence" << std::endl;
    }
    core.reset(tempCore);
    return result == discord::Result::Ok;
}

// DESTROY THE DISCORD PRESENCE IF IT EXISTS

void destroyPresence(void) {
    if (!core) {
        return;
    }
    core.reset(nullptr);
    if (LOGGING && !core) {
        std::cout << "Discord presence has been destroyed" << std::endl;
    }
    else if (LOGGING) {
        std::cout << "Failed to destroy Discord presence" << std::endl;
    }
}

// FORMAT C STRINGS

void formatCString(char* str) { // REMEMBER TO DEAL WITH OTHER SPECIAL CHARACTERS LATER
    int j = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        if (!(str[i] == '\\' && str[i + 1] == '\"') && !(str[i] == '\\' && str[i + 1] == '\'') && !(str[i] == '\\' && str[i + 1] == '\?') && !(str[i] == '\\' && str[i + 1] == '\\')) {
            str[j] = str[i];
            ++j;
        }
    }
    memset(&str[j], '\0', ACTIVITY_BUFFER_SIZE - j);
}

// UPDATE DISCORD PRESENCE WITH DATA

void updatePresence(const std::string& title, const std::string& author, const std::string& timeLeftStr) {
    if (core && (title == IDLE_IDENTIFIER || core->RunCallbacks() != discord::Result::Ok)) {
        destroyPresence();
        return;
    }
    else if (!core) {
        if (title == IDLE_IDENTIFIER) {
            return;
        }
        bool didCreatePresence = createPresence();
        if (!didCreatePresence) {
            destroyPresence();
            return;
        }
    }
    int timeLeft = std::stoi(timeLeftStr);

    discord::Activity activity{};
    discord::ActivityTimestamps& timeStamp = activity.GetTimestamps();
    discord::ActivityAssets& activityAssets = activity.GetAssets();

    char titleCString[ACTIVITY_BUFFER_SIZE], authorCString[ACTIVITY_BUFFER_SIZE];
    memset(authorCString, '\0', sizeof(authorCString));
    memset(titleCString, '\0', sizeof(titleCString));

    strcpy_s(titleCString, ACTIVITY_BUFFER_SIZE, title.c_str());
    if (timeLeft != LIVESTREAM_TIME_ID) {
        strcpy_s(authorCString, ACTIVITY_BUFFER_SIZE, ("by " + author).c_str());
        activityAssets.SetLargeImage("youtube3");
        timeStamp.SetEnd(std::time(nullptr) + timeLeft);
    }
    else {
        if (!(previousTitle == title && previousAuthor == author)) { // MIGHT JUST BE BETTER TO HAVE BACKGROUND.JS SEND THE VIDEO ID
            elapsedTime = std::time(nullptr);
        }
        strcpy_s(authorCString, ACTIVITY_BUFFER_SIZE, ("[LIVE] on " + author).c_str());
        activityAssets.SetLargeImage("youtubelive1");
        timeStamp.SetStart(elapsedTime);
    }

    formatCString(titleCString);
    formatCString(authorCString);

    activity.SetDetails(titleCString);
    activityAssets.SetLargeText(titleCString);
    activity.SetState(authorCString);
    activityAssets.SetSmallImage("githubmark2");
    activityAssets.SetSmallText("YouTubeDiscordPresence on GitHub by XFG16 (2309#2309)"); // keep this here please, so others can find the extension

    previousTitle = title;
    previousAuthor = author;

    bool presenceUpdated = false;
    core->ActivityManager().UpdateActivity(activity, [&presenceUpdated](discord::Result result) {
        presenceUpdated = true;
    });
    int numUpdates = 0;
    while (numUpdates < 4 || (numUpdates < 10 && !presenceUpdated)) {
        core->RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ++numUpdates;
    }
}

// FORMAT ACTUAL INFORMATION FROM DATA SENT BY EXTENSION

void handleData(const std::string& documentData) {
    size_t titleLocation = documentData.find(TITLE_IDENTIFIER);
    size_t authorLocation = documentData.find(AUTHOR_IDENTIFIER);
    size_t timeLeftLocation = documentData.find(TIME_LEFT_IDENTIFIER);
    size_t endLocation = documentData.length() - END_IDENTIFIER.length();

    std::string title = documentData.substr(titleLocation + TITLE_IDENTIFIER.length(), authorLocation - (titleLocation + TITLE_IDENTIFIER.length()));
    std::string author = documentData.substr(authorLocation + AUTHOR_IDENTIFIER.length(), timeLeftLocation - (authorLocation + AUTHOR_IDENTIFIER.length()));
    std::string timeLeft = documentData.substr(timeLeftLocation + TIME_LEFT_IDENTIFIER.length(), endLocation - (timeLeftLocation + TIME_LEFT_IDENTIFIER.length()));

    updatePresence(title, author, timeLeft);
}

// PROGRAM ENTRY

int main(void) {
    std::string documentData;
    if (LOGGING) {
        std::cout << "Program initialized" << std::endl;
    }

    char temp;
    while (std::cin >> std::noskipws >> temp) {
        documentData += temp;
        if (documentData.length() >= END_IDENTIFIER.length() && documentData.substr(documentData.length() - END_IDENTIFIER.length(), END_IDENTIFIER.length()) == END_IDENTIFIER) {
            handleData(documentData);
            documentData.clear();
            if (LOGGING) {
                std::cout << "DATA CLEARED" << std::endl;
            }
        }
    }

    return 0;
}