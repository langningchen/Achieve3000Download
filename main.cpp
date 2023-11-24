/**********************************************************************
Achieve3000Download: Download Achieve3000 article as HTML to make it easier to print.
Copyright (C) 2023  langningchen

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
**********************************************************************/

#include <Curl.hpp>
#include <regex>
#include <unistd.h>
json LessonData;
string RemoveHTMLElement(string Data)
{
    Data = regex_replace(Data, regex("(<[/]?[^>]*>|\n|\t)"), "");
    Data = StringReplaceAll(Data, "\n", "");
    Data = StringReplaceAll(Data, "\t", "");
    return Data;
}
void OutputToHtml(string URL)
{
    const string VocabularyTemplate = R"(<span><b>${Word}</b> (${SpeechPart})</span>
<span>${Definition}</span>
<br>
)";
    const string Template = R"(<html>

<head>
    <title>
        ${LESSON_NAME}
    </title>
    <style>
        * {
            font-size: 15px;
        }

        span {
            margin-right: 3px;
        }

        .Big1,
        .Big2,
        .Big3 {
            font-weight: 600;
        }

        .Big1 {
            font-size: 25px;
        }

        .Big2 {
            font-size: 22px;
        }

        .Big3 {
            font-size: 18px;
        }

        .Small {
            font-size: 10px;
            color: rgb(0, 0, 0, 0.5);
        }

        .Content {
            line-height: 2;
        }
    </style>
</head>

<body>
    <div class="Big1">
        ${LESSON_NAME}
    </div>
    <div class="Small">${TIME} ${URL}</div>
    <hr>
    <div class="Content">
        ${ARTICLE_CONTENT}
    </div>
    <hr>
    <div class="Big2">
        Vocabulary
    </div>
    <br>
    <div class="Content">
        ${VOCABULARY_LIST}
    </div>
    <hr>
    <div class="Big2">
        Respond
    </div>
    <br>
    ${QUESTION_LIST}
</body>

</html>)";
    cout << "Generating content... " << flush;
    curl_slist *HeaderList = NULL;
    HeaderList = curl_slist_append(HeaderList, string("X-ACHIEVE-SESSION: " + LessonData["session"]["resumeSessionId"].as_string()).c_str());
    HeaderList = curl_slist_append(HeaderList, string("X-ACHIEVE-SESSION-KEY: " + LessonData["session"]["resumeSessionToken"].as_string()).c_str());
    HeaderList = curl_slist_append(HeaderList, string("X-XSRF-TOKEN: " + LessonData["session"]["csrfToken"].as_string()).c_str());
    string FileName = regex_replace(RemoveHTMLElement(LessonData["lessonInfo"]["lessonName"].as_string()), regex("(\\?|\"|/|\\|\\|<|>|:|\\*)"), "");

    string OutputContent = Template;
    OutputContent = StringReplaceAll(OutputContent, "${LESSON_NAME}", RemoveHTMLElement(LessonData["lessonInfo"]["lessonName"].as_string()));

    time_t Time = time(NULL);
    tm *LocalTime = localtime(&Time);
    char Date[20];
    strftime(Date, sizeof(Date), "%Y-%m-%d %H:%M:%S", LocalTime);
    OutputContent = StringReplaceAll(OutputContent, "${TIME}", Date);

    OutputContent = StringReplaceAll(OutputContent, "${URL}", URL);

    string Article = LessonData["lessonContent"]["articles"][0]["pages"][0]["content"].as_string();
    Article = regex_replace(Article, regex("<em>[a-zA-Z\\s]* contributed to this story.</em>"), "");
    Article = regex_replace(Article, regex("Credit for photo and all related images: [^<]*"), "");
    Article = regex_replace(Article, regex("<[/]?(span|br|video|p|div)[^>]*>"), "");
    Article = regex_replace(Article, regex("<a class='dict-word' href='#' title='Show dictionary definition'>[a-zA-Z\\s]*</a>"), "");
    Article = regex_replace(Article, regex("<!--.*-->"), "");
    Article = regex_replace(Article, regex("<img[^>]*src=\"([^\"]*)\"[^>]*>"), "");
    Article = StringReplaceAll(Article, "\t", "");
    Article = StringReplaceAll(Article, " \n", "\n");
    Article = StringReplaceAll(Article, "\n ", "\n");
    Article = StringReplaceAll(Article, "\n\n\n", "\n\n");
    while (Article.size() > 0 && Article[Article.size() - 1] == '\n')
        Article.erase(Article.size() - 1, 1);
    while (Article.size() > 0 && Article[0] == '\n')
        Article.erase(0, 1);
    Article = StringReplaceAll(Article, "\n\n", "\n  </span>\n  <br />\n  <span>\n    ");
    Article = "  <span>\n    " + Article + "\n  </span>";
    OutputContent = StringReplaceAll(OutputContent, "${ARTICLE_CONTENT}", Article);

    string VocabularyList = "";
    for (json::iterator jit = LessonData["lessonContent"]["vocabulary"].begin(); jit != LessonData["lessonContent"]["vocabulary"].end(); jit++)
    {
        string CurrentVocabularyList = VocabularyTemplate;
        CurrentVocabularyList = StringReplaceAll(CurrentVocabularyList, "${Word}", jit.value()["word"].as_string());
        CurrentVocabularyList = StringReplaceAll(CurrentVocabularyList, "${SpeechPart}", jit.value()["speechPart"].as_string());
        CurrentVocabularyList = StringReplaceAll(CurrentVocabularyList, "${Definition}", jit.value()["definition"].as_string());
        VocabularyList += CurrentVocabularyList;
    }
    OutputContent = StringReplaceAll(OutputContent, "${VOCABULARY_LIST}", VocabularyList);

    size_t Counter = 0;
    string QuestionList = "";
    for (json::iterator jit = LessonData["activities"]["14"][0]["questions"].begin(); jit != LessonData["activities"]["14"][0]["questions"].end(); jit++)
    {
        QuestionList += "  <span class=\"Big3\">\n"s +
                        "    Question " + to_string(Counter + 1) + "\n" +
                        "  </span>\n" +
                        "  <br />\n";
        string Question = jit.value()["collection"][0]["question"].as_string();
        Question = regex_replace(Question, regex("<[/]?(span|br|video|p|div|a)[^>]*>"), "");
        Question = regex_replace(Question, regex("<img[^>]*src=\"([^\"]*)\"[^>]*>"), "<img src=\"https://portal.achieve3000.com$1\" style=\"width: 48%; \">");
        Question = StringReplaceAll(Question, "\t", "");
        Question = StringReplaceAll(Question, " \n", "\n");
        Question = StringReplaceAll(Question, "\n ", "\n");
        Question = StringReplaceAll(Question, "\n\n\n", "\n\n");
        while (Question.size() > 0 && Question[Question.size() - 1] == '\n')
            Question.erase(Question.size() - 1, 1);
        while (Question.size() > 0 && Question[0] == '\n')
            Question.erase(0, 1);
        Question = StringReplaceAll(Question, "\n\n", "\n  </span>\n  <br />\n  <span>\n    ");
        Question = "  <span>\n    " + Question + "\n  </span>";
        QuestionList += "(&ensp;&ensp;&ensp;) " + Question + "\n  <br />\n";
        short Counter2 = 0;
        for (json::iterator jit2 = LessonData["activities"]["14"][0]["questions"][Counter]["collection"][0]["items"].begin(); jit2 != LessonData["activities"]["14"][0]["questions"][Counter]["collection"][0]["items"].end(); jit2++)
        {
            string Item = jit2.value()["label"].as_string();
            Item = regex_replace(Item, regex("<div.*class=\"screenreader-only\">[^<]*</div>"), "");
            Item = regex_replace(Item, regex("<[/]?(span|br|video|p|div|a)[^>]*>"), "");
            Item = StringReplaceAll(Item, "\t", "");
            Item = StringReplaceAll(Item, " \n", "\n");
            Item = StringReplaceAll(Item, "\n ", "\n");
            Item = StringReplaceAll(Item, "\n\n\n", "\n\n");
            while (Item.size() > 0 && Item[Item.size() - 1] == '\n')
                Item.erase(Item.size() - 1, 1);
            while (Item.size() > 0 && Item[0] == '\n')
                Item.erase(0, 1);
            Item = StringReplaceAll(Item, "\n\n", "\n    </span>\n    <br />\n    <span>\n      ");
            Item = "    <span>\n      " + Item + "\n    </span>";
            QuestionList += "  <span>\n    <span>\n      ";
            QuestionList.push_back(Counter2 + 'A');
            QuestionList += ".&ensp;\n    </span>\n" + Item + "\n  </span>\n  <br />\n";
            Counter2++;
        }
        QuestionList += "  <br />\n";
        Counter++;
    }
    OutputContent = StringReplaceAll(OutputContent, "${QUESTION_LIST}", QuestionList);

    SetDataFromStringToFile(FileName + ".html", OutputContent);
    cout << "Succeed" << endl;
}
void Answer()
{
    size_t LessonID = LessonData["lessonInfo"]["lessonId"].as_integer();
    size_t ProfileLessonID = LessonData["lessonInfo"]["profileLessonId"].as_integer();
    size_t CategoryID = LessonData["lessonInfo"]["categoryId"].as_integer();
    size_t LanguageID = LessonData["lessonInfo"]["languageId"].as_integer();
    size_t SubcategoryID = LessonData["lessonInfo"]["subCategoryId"].as_integer();
    size_t ReadingLevel = LessonData["lessonInfo"]["readingLevel"].as_integer();
    size_t LessonTypeId = LessonData["lessonInfo"]["lessonTypeId"].as_integer();

    // vector<pair<size_t, string>> PollChoices;
    // for (auto i : LessonData["poll"]["choices"])
    //     PollChoices.push_back({i["choiceId"].as_integer(), i["choiceText"].as_string()});
    // cout << LessonData["poll"]["question"].as_string() << endl;
    // for (size_t i = 0; i < PollChoices.size(); i++)
    //     cout << "\033[32m#" << (i + 1) << "\033[0m\t\033[31m" << PollChoices[i].second << "\033[0m" << endl;
    // size_t PollChoice = 0;
    // cout << "Please input the id: ";
    // cin >> PollChoice;
    // if (PollChoice > PollChoices.size() || PollChoice <= 0)
    //     TRIGGER_ERROR("Input which is " + to_string(PollChoice) + " must less or equal than" + to_string(PollChoices.size()) + " and greater than 0");
    // cout << "Saving..." << flush;
    // GetDataToFile("https://portal.achieve3000.com/kb/lesson/save_poll", "", "", true,
    //               "poll_id=" + LessonData["poll"]["pollId"].as_string() +
    //                   "&lid=" + to_string(LessonID) +
    //                   "&step=10" +
    //                   "&oid=0" +
    //                   "&ot=0" +
    //                   "&choice_id=" + to_string(PollChoices[PollChoice].first) +
    //                   "&response=",
    //               NULL, NULL, FORM);
    // cout << json::parse(GetDataFromFileToString())["alert"].as_string() << endl;

    // cout << "Reading..." << flush;
    // GetDataToFile("https://portal.achieve3000.com/api/v1/progress/update", "", "", true,
    //               "lid=" + to_string(LessonID) +
    //                   "&c=" + to_string(CategoryID) +
    //                   "&sc=" + to_string(SubcategoryID) +
    //                   "&oid=0" +
    //                   "&ot=0" +
    //                   "&asn=0" +
    //                   "&lesson_id=" + to_string(LessonID) +
    //                   "&profile_lesson_id=" + to_string(ProfileLessonID) +
    //                   "&step=read" +
    //                   "&category_id=" + to_string(CategoryID) +
    //                   "&step_id=11" +
    //                   "&language_id=" + to_string(LanguageID) +
    //                   "&reading_level=" + to_string(ReadingLevel) +
    //                   "&lesson_type_id=" + to_string(LessonTypeId),
    //               NULL, NULL, FORM);
    // cout << "Succeed" << endl;

    size_t ActivityID = LessonData["activities"]["14"][0]["id"].as_integer();
    size_t ActivityTypeID = LessonData["activities"]["14"][0]["activityTypeId"].as_integer();
    size_t ActivityGuesses = LessonData["activities"]["14"][0]["guesses"].as_integer();
    for (size_t ActivityProblem = 0; ActivityProblem < LessonData["activities"]["14"][0]["questions"].size(); ActivityProblem++)
    {
        size_t QuestionID = LessonData["activities"]["14"][0]["questions"][ActivityProblem]["id"].as_integer();
        size_t QuestionCollectionID = LessonData["activities"]["14"][0]["questions"][ActivityProblem]["collection"][0]["id"].as_integer();
        string QuestionDetail = LessonData["activities"]["14"][0]["questions"][ActivityProblem]["collection"][0]["question"].as_string();
        string QuestionHint = LessonData["activities"]["14"][0]["questions"][ActivityProblem]["collection"][0]["hint"].as_string();
        string QuestionAnswerExplanation = LessonData["activities"]["14"][0]["questions"][ActivityProblem]["collection"][0]["answerExplanation"].as_string();
        size_t QuestionCorrectAnswers = LessonData["activities"]["14"][0]["questions"][ActivityProblem]["collection"][0]["correctAnswers"].as_integer();
        vector<pair<string, string>> QuestionChoices;
        for (auto i : LessonData["activities"]["14"][0]["questions"][ActivityProblem]["collection"][0]["items"])
            QuestionChoices.push_back({i["id"].as_string(), i["label"].as_string()});
        for (size_t GuessCount = 1; GuessCount <= ActivityGuesses; GuessCount++)
        {
            cout << "\033[32m#" << (ActivityProblem + 1) << "\033[0m\t\033[31m" << RemoveHTMLElement(QuestionDetail) << "\033[0m" << endl;
            for (size_t i = 0; i < QuestionChoices.size(); i++)
                cout << "\033[32m#" << (i + 1) << "\033[0m\t\033[31m" << QuestionChoices[i].second << "\033[0m" << endl;
            size_t QuestionChoice = 0;
            cout << "Please input the id: ";
            cin >> QuestionChoice;
            if (QuestionChoice > QuestionChoices.size() || QuestionChoice <= 0)
                TRIGGER_ERROR("Input which is " + to_string(QuestionChoice) + " must less or equal than" + to_string(QuestionChoices.size()) + " and greater than 0");
            cout << "Saving..." << flush;

            json QuestionJSON;
            QuestionJSON["activities"][0]["id"] = ActivityID;
            QuestionJSON["activities"][0]["activity_type_id"] = ActivityTypeID;
            QuestionJSON["activities"][0]["questions"][0]["id"] = QuestionID;
            QuestionJSON["activities"][0]["questions"][0]["activityId"] = ActivityID;
            QuestionJSON["activities"][0]["questions"][0]["guess"] = GuessCount;
            QuestionJSON["activities"][0]["questions"][0]["collection"][0]["type"] = "mc";
            QuestionJSON["activities"][0]["questions"][0]["collection"][0]["id"] = QuestionCollectionID;
            QuestionJSON["activities"][0]["questions"][0]["collection"][0]["question"] = QuestionDetail;
            QuestionJSON["activities"][0]["questions"][0]["collection"][0]["hint"] = QuestionHint;
            QuestionJSON["activities"][0]["questions"][0]["collection"][0]["answerExplanation"] = QuestionAnswerExplanation;
            QuestionJSON["activities"][0]["questions"][0]["collection"][0]["correctAnswers"] = QuestionCorrectAnswers;
            for (size_t i = 0; i < QuestionChoices.size(); i++)
            {
                QuestionJSON["activities"][0]["questions"][0]["collection"][0]["items"][i]["id"] = QuestionChoices[i].first;
                QuestionJSON["activities"][0]["questions"][0]["collection"][0]["items"][i]["label"] = QuestionChoices[i].second;
                QuestionJSON["activities"][0]["questions"][0]["collection"][0]["items"][i]["selected"] = i + 1 == QuestionChoice;
            }
            QuestionJSON["general_info"]["lesson_id"] = LessonID;
            QuestionJSON["general_info"]["step_id"] = 14;
            QuestionJSON["general_info"]["category_id"] = CategoryID;
            QuestionJSON["general_info"]["s_category_id"] = SubcategoryID;
            QuestionJSON["general_info"]["activity_id"] = ActivityID;

            cout << QuestionJSON.dump() << endl;
            GetDataToFile("https://portal.achieve3000.com/kb/lesson_activity/save_scored_activity", "", "", true,
                          QuestionJSON.dump());
            cout << GetDataFromFileToString() << endl;
            if (json::parse(GetDataFromFileToString())["question"]["isCorrect"].as_bool())
            {
                cout << "Succeed" << endl;
                break;
            }
            else
                cout << "Failed" << endl;
        }
    }
}
int main(int argc, char **argv)
{
    CLN_TRY
    size_t Index = 0;
    std::string Username = "";
    std::string Password = "";
    for (int i = 1; i < argc; i++)
    {
        string Argument = argv[i];
        string NextArgument = i + 1 == argc ? "" : argv[i + 1];
        if (Argument == "-i" || Argument == "--id")
        {
            if ((Index = atoi(NextArgument.c_str())) == 0)
                TRIGGER_ERROR("Invalid id passed");
        }
        else if (Argument == "-u" || Argument == "--username")
        {
            Username = NextArgument;
            i++;
        }
        else if (Argument == "-p" || Argument == "--password")
        {
            Password = NextArgument;
            i++;
        }
        else
            TRIGGER_ERROR("Unknown option \"" + Argument + "\"");
    }
    if (Username == "")
        TRIGGER_ERROR("No username provided");
    if (Password == "")
        TRIGGER_ERROR("No password provided");
    curl_slist *HeaderList = NULL;
    HeaderList = curl_slist_append(HeaderList, "X-Requested-With: XMLHttpRequest");
    cout << "Logging in... " << flush;
    GetDataToFile("https://portal.achieve3000.com/util/login.php",
                  "",
                  "",
                  true,
                  "debug=0"s +
                      "&login_goto=" +
                      "&lang=1" +
                      "&ajax_yn=Y" +
                      "&flash_version=" +
                      "&login_name=" + Username +
                      "&wz=0" +
                      "&cli=0" +
                      "&login_url=portal.achieve3000.com%2Findex" +
                      "&password=" + Password +
                      "&cdn=VIDEOCDN%3A0%3BAUDIOCDN%3A0%3BIMAGECDN%3A0%3BDOCSCDN%3A0%3BIMAGEASSETSCDN%3A0%3BAPPASSETSCDN%3A0%3BJSASSETSCDN%3A0%3BCSSASSETSCDN%3A0" +
                      "&banner=1" +
                      "&redirectedFromLE=" +
                      "&login_page_type=" +
                      "&domain_id=1" +
                      "&walkme=" +
                      "&lid=" +
                      "&login_error_message=" +
                      "&login_name1=" +
                      "&password1=" +
                      "&lost_login_name=" +
                      "&isAjax=Y",
                  NULL,
                  NULL,
                  FORM);
    cout << GetDataFromFileToString() << endl;
    if (GetDataFromFileToString().find("REDIRECT") == string::npos)
        TRIGGER_ERROR("Login failed");
    cout << "Succeed" << endl;

    cout << "Getting your lessons... " << flush;
    GetDataToFile("https://portal.achieve3000.com/api/v1/student/my-lessons");
    configor::json JSONData = configor::json::parse(GetDataFromFileToString());
    cout << "Succeed" << endl
         << "Searching available lessons... " << flush;
    vector<string> FetchURLs;
    cout << "Succeed" << endl;
    for (size_t i = 0; i < JSONData.size(); i++)
        if (JSONData[i]["lessonType"].as_string() == "5-Step Lesson" && !JSONData[i]["isLessonCompleted"].as_bool())
        {
            FetchURLs.push_back("https://portal.achieve3000.com/api/v1/lessoncontent/fetch?" + JSONData[i]["lessonUrl"].as_string().substr(8));
            cout << "\033[32m#" << FetchURLs.size() << "\033[0m\t\033[31m" << JSONData[i]["lessonName"].as_string() << "\033[0m\r\033[40C\033[K\033[10C\033[33m" << JSONData[i]["categoryName"].as_string() << "\033[0m: \033[34m" << JSONData[i]["sCategoryName"].as_string() << "\033[0m" << endl;
        }
    if (Index == 0)
    {
        cout << "Please input the id: ";
        cin >> Index;
    }
    if (Index > FetchURLs.size() || Index <= 0)
        TRIGGER_ERROR("Input which is " + to_string(Index) + " must less or equal than" + to_string(FetchURLs.size()) + " and greater than 0");

    cout << "Getting lessons detail... " << flush;
    GetDataToFile(FetchURLs[Index - 1].c_str());
    LessonData = json::parse(GetDataFromFileToString());
    cout << "Succeed" << endl;

    // OutputToHtml(FetchURLs[Index - 1]);
    Answer();
    CLN_CATCH
    return 0;
}
