#include "Curl.hpp"
#include <regex>
#include <unistd.h>
const string VocabularyTemplate = R"(<span class="Big3\">
    <span>
        ${Word}
    </span>
    <span class="Code">
        ${SpeechPart}
    </span>
</span>
<br />
<span>
    ${Definition}
</span>
<br />
)";
const string PollChoicesTemplate = R"(<input type="checkbox" class="CheckBox" />
<span>
    ${ChoiceText}
</span>
<br />)";
const string PrintTemplate = R"(<script>
    window.print();
    setTimeout(function () {
        window.close();
    }, 1500);
</script>)";
const string Template = R"(<html>

<head>
    <title>
        ${LESSON_NAME}
    </title>
    <style>
        * {
            font-size: 15px;
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
            font-size: 20px;
        }

        .Big3 {
            font-size: 18px;
        }

        .Code,
        .CodeBlock {
            border-radius: 3px;
            background-color: rgba(0, 0, 0, 0.05);
            padding: 2px 5px;
            color: black;
        }

        .CodeBlock {
            border-radius: 10px;
            width: 100%;
        }

        .CheckBox {
            vertical-align: middle;
            height: 20px;
            width: 20px;
        }

    </style>
</head>

<body>
    <span class="Big1">
        ${LESSON_NAME}
    </span>
    <br />
    <br />
    <span class="Big2">
        Ready
    </span>
    <br />
    <br />
    <span>
        ${POLL_QUESTION}
    </span>
    <br />
    <span>
        ${OPTION_STATEMENT}
    </span>
    <br />
    ${POLL_CHOICES}
    <span>
        Explain why you voted the way you did.
    </span>
    <br />
    <br />
    <div class="CodeBlock">
        <br />
        <br />
        <br />
    </div>
    <span class="Big2">
        Vocabulary
    </span>
    <br />
    <br />
    ${VOCABULARY_LIST}
    <span class="Big2">
        Ready
    </span>
    <br />
    <br />
    ${ARTICLE_CONTENT}
    <br />
    ${ARTICLE_IMAGES}
    <span class="Big2">
        Respond
    </span>
    <br />
    <br />
    ${QUESTION_LIST}
    <span class="Big2">
        Reflect
    </span>
    <br />
    <br />
    <span>
        ${POLL_QUESTION}
    </span>
    <br />
    <span>
        ${OPTION_STATEMENT}
    </span>
    <br />
    ${POLL_CHOICES}
    </div>
    <span class="Big2">
        Write
    </span>
    <br />
    <br />
    ${WRITING_QUESTION}
    <br />
    <br />
    <div class="CodeBlock">
        <br />
        <br />
        <br />
        <br />
        <br />
    </div>
    ${PRINT}
</body>

</html>)";
string NormalizeString(string Data)
{
    Data = regex_replace(Data, regex("(<[/]?[^>]*>|\n|\t)"), "");
    Data = StringReplaceAll(Data, "\n", "");
    Data = StringReplaceAll(Data, "\t", "");
    return Data;
}
int main(int argc, char **argv)
{
    CLN_TRY
    int Index = 0;
    bool Open = true;
    bool Print = true;
    bool Delete = true;
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
        else if (Argument == "-no" || Argument == "--no-open")
            Open = false;
        else if (Argument == "-np" || Argument == "--no-print")
            Print = false;
        else if (Argument == "-nd" || Argument == "--no-delete")
            Delete = false;
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
                  "application/x-www-form-urlencoded; charset=UTF-8");
    if (GetDataFromFileToString().find("REDIRECT") == string::npos)
        TRIGGER_ERROR("Login failed");
    cout << "Succeed" << endl
         << "Getting your lessons... " << flush;
    GetDataToFile("https://portal.achieve3000.com/my_lessons");
    string HTMLData = GetDataFromFileToString();
    cout << "Succeed" << endl
         << "Searching available lessons... " << flush;
    vector<pair<string, pair<string, string>>> Lessons;
    smatch Match;
    string::const_iterator StartPos = HTMLData.cbegin();
    while (regex_search(StartPos, HTMLData.cend(), Match, regex("<a class=\"title\" href=\"/lesson\\?lid=([0-9]*)&c=([0-9]*)&sc=([0-9]*)&oid=[0-9]*&ot=[0-9]*&asn=[0-9]*\"  >[^<]*</a>[^5]*5-Step Lesson</span></div>")))
    {
        Lessons.push_back(make_pair(Match.str(1), make_pair(Match.str(2), Match.str(3))));
        StartPos = Match.suffix().first;
    }
    cout << "Succeed" << endl;
    int Counter = 1;
    for (vector<pair<string, pair<string, string>>>::iterator vit = Lessons.begin(); vit != Lessons.end(); vit++)
    {
        GetDataToFile(string("https://portal.achieve3000.com/api/v1/lessoncontent/fetch?lid=" + vit->first + "&c=" + vit->second.first + "&sc=" + vit->second.second).c_str());
        json Data = json::parse(GetDataFromFileToString());
        cout << "#" << Counter << "  " << NormalizeString(Data["lessonInfo"]["lessonName"]) << endl
             << NormalizeString(Data["lessonInfo"]["lessonSummary"]) << endl
             << NormalizeString(Data["lessonInfo"]["categoryName"]) << ": " << NormalizeString(Data["lessonInfo"]["subCategoryName"]) << endl
             << endl;
        Counter++;
    }
    if (Index == 0)
    {
        cout << "Please input the id: ";
        cin >> Index;
    }
    if (Index >= Counter)
        TRIGGER_ERROR("Input which is " + to_string(Index) + " must less than" + to_string(Counter));

    cout << "Getting lessons detail... " << flush;
    string LessonID = Lessons[Index - 1].first;
    string CategoryID = Lessons[Index - 1].second.first;
    string SubcategoryID = Lessons[Index - 1].second.second;
    GetDataToFile(string("https://portal.achieve3000.com/api/v1/lessoncontent/fetch?lid=" + LessonID + "&c=" + CategoryID + "&sc=" + SubcategoryID).c_str());
    json Data = json::parse(GetDataFromFileToString());
    cout << "Succeed" << endl
         << "Generating content... " << flush;
    HeaderList = NULL;
    HeaderList = curl_slist_append(HeaderList, string("X-ACHIEVE-SESSION: " + Data["session"]["resumeSessionId"].as_string()).c_str());
    HeaderList = curl_slist_append(HeaderList, string("X-ACHIEVE-SESSION-KEY: " + Data["session"]["resumeSessionToken"].as_string()).c_str());
    HeaderList = curl_slist_append(HeaderList, string("X-XSRF-TOKEN: " + Data["session"]["csrfToken"].as_string()).c_str());
    string FileName = regex_replace(NormalizeString(Data["lessonInfo"]["lessonName"].as_string()), regex("(\\?|\"|/|\\|\\|<|>|:|\\*)"), "");
    string OutputContent = Template;
    OutputContent = StringReplaceAll(OutputContent, "${LESSON_NAME}", NormalizeString(Data["lessonInfo"]["lessonName"].as_string()));
    OutputContent = StringReplaceAll(OutputContent, "${POLL_QUESTION}", EraseHTMLElement(Data["poll"]["question"].as_string()));
    OutputContent = StringReplaceAll(OutputContent, "${OPTION_STATEMENT}", Data["poll"]["opinionStatement"].as_string());
    string PollChoices = "";
    for (json::iterator jit = Data["poll"]["choices"].begin(); jit != Data["poll"]["choices"].end(); jit++)
    {
        PollChoices += StringReplaceAll(PollChoicesTemplate,
                                        "${ChoiceText}",
                                        jit.value()["choiceText"].as_string());
    }
    OutputContent = StringReplaceAll(OutputContent, "${POLL_CHOICES}", PollChoices);
    string VocabularyList = "";
    for (json::iterator jit = Data["lessonContent"]["vocabulary"].begin(); jit != Data["lessonContent"]["vocabulary"].end(); jit++)
    {
        string CurrentVocabularyList = VocabularyTemplate;
        CurrentVocabularyList = StringReplaceAll(CurrentVocabularyList, "${Word}", jit.value()["word"].as_string());
        CurrentVocabularyList = StringReplaceAll(CurrentVocabularyList, "${SpeechPart}", jit.value()["speechPart"].as_string());
        CurrentVocabularyList = StringReplaceAll(CurrentVocabularyList, "${Definition}", jit.value()["definition"].as_string());
        VocabularyList += CurrentVocabularyList;
    }
    OutputContent = StringReplaceAll(OutputContent, "${VOCABULARY_LIST}", VocabularyList);
    string Article = Data["lessonContent"]["articles"][0]["pages"][0]["content"].as_string();
    Article = regex_replace(Article, regex("<em>[a-zA-Z\\s]* contributed to this story.</em>"), "");
    Article = regex_replace(Article, regex("Credit for photo and all related images: [^<]*"), "");
    Article = regex_replace(Article, regex("<[/]?(span|br|video|p|div)[^>]*>"), "");
    Article = regex_replace(Article, regex("<a class='dict-word' href='#' title='Show dictionary definition'>[a-zA-Z\\s]*</a>"), "");
    Article = regex_replace(Article, regex("<!--.*-->"), "");
    vector<string> PictureVector;
    StartPos = Article.cbegin();
    while (regex_search(StartPos, Article.cend(), Match, regex("<img[^>]*src=\"([^\"]*)\"[^>]*>")))
    {
        PictureVector.push_back(Match.str(1));
        StartPos = Match.suffix().first;
    }
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
    string ArticleImages = "";
    for (vector<string>::iterator vit = PictureVector.begin(); vit != PictureVector.end(); vit++)
        if (vit->find("magnify") == vit->npos && vit->size() > 1)
        {
            string URL = *vit;
            if (URL[0] != 'h')
                URL.insert(0, "https://portal.achieve3000.com");
            ArticleImages += "  <img src=\"" + URL + "\" style=\"width: 48%; \"/>\n";
        }
    OutputContent = StringReplaceAll(OutputContent, "${ARTICLE_IMAGES}", ArticleImages);
    Counter = 0;
    string QuestionList = "";
    for (json::iterator jit = Data["activities"]["14"][0]["questions"].begin(); jit != Data["activities"]["14"][0]["questions"].end(); jit++)
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
        QuestionList += Question + "\n  <br />\n";
        short Counter2 = 0;
        for (json::iterator jit2 = Data["activities"]["14"][0]["questions"][Counter]["collection"][0]["items"].begin(); jit2 != Data["activities"]["14"][0]["questions"][Counter]["collection"][0]["items"].end(); jit2++)
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
            QuestionList += "  <input type=\"checkbox\" class=\"CheckBox\" />\n  <span>\n    <span>\n      ";
            QuestionList.push_back(Counter2 + 'A');
            QuestionList += ".&ensp;\n    </span>\n" + Item + "\n  </span>\n  <br />\n";
            Counter2++;
        }
        QuestionList += "  <br />\n";
        Counter++;
    }
    OutputContent = StringReplaceAll(OutputContent, "${QUESTION_LIST}", QuestionList);
    string WritingQuestion = Data["thoughtQuestion"]["question"].as_string();
    WritingQuestion = regex_replace(WritingQuestion, regex("<[/]?(span|br|video|p|div|a)[^>]*>"), "");
    WritingQuestion = StringReplaceAll(WritingQuestion, "\t", "");
    WritingQuestion = StringReplaceAll(WritingQuestion, " \n", "\n");
    WritingQuestion = StringReplaceAll(WritingQuestion, "\n ", "\n");
    WritingQuestion = StringReplaceAll(WritingQuestion, "\n\n\n", "\n\n");
    while (WritingQuestion.size() > 0 && WritingQuestion[WritingQuestion.size() - 1] == '\n')
        WritingQuestion.erase(WritingQuestion.size() - 1, 1);
    while (WritingQuestion.size() > 0 && WritingQuestion[0] == '\n')
        WritingQuestion.erase(0, 1);
    WritingQuestion = StringReplaceAll(WritingQuestion, "\n\n", "\n  </span>\n  <br />\n  <span>\n    ");
    WritingQuestion = "  <span>\n    " + WritingQuestion + "\n  </span>";
    OutputContent = StringReplaceAll(OutputContent, "${WRITING_QUESTION}", WritingQuestion);
    OutputContent = StringReplaceAll(OutputContent, "${PRINT}", Print ? PrintTemplate : "");
    SetDataFromStringToFile("/mnt/c/Users/Public/" + FileName + ".html", OutputContent);
    if (Open && system(string("\"/mnt/c/Program Files/Google/Chrome/Application/chrome.exe\" "s + (Print ? "--kiosk-printing --kiosk " : "") + "\"C:\\Users\\Public\\" + FileName + ".html\"").c_str()))
        TRIGGER_ERROR("Call chrome.exe to open file failed");
    if (Open && Print && Delete && (remove(string("/mnt/c/Users/Public/" + FileName + ".html").c_str())) == -1)
        TRIGGER_ERROR("Delete failed");
    CLN_CATCH
    return 0;
}
