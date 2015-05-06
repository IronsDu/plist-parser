#include <assert.h>
#include <iostream>
using namespace std;

#include "plist.h"

/*  TODO::目前没有做错误处理，仅使用断言做简单测试  (可抛出异常)    */

PntArray::~PntArray()
{
    for (auto& v : mValue)
    {
        delete v;
    }
    mValue.clear();
}

PntDict::~PntDict()
{
    for (auto& v : mValue)
    {
        delete v.second;
    }
    mValue.clear();
}

enum EToken
{
    ET_NONE,
    ET_EOF,
    ET_DICT_START,
    ET_DICT_END,
    ET_KEY_START,
    ET_KEY_END,
    ET_ARRAY_START,
    ET_ARRAY_END,
    ET_INT_START,
    ET_INT_END,
    ET_TRUE,
    ET_FALSE,
    ET_STRING_START,
    ET_STRING_END,
    ET_REAL_START,
    ET_REAL_END,
    ET_DATA_START,
    ET_DATA_END,
    ET_DATE_START,
    ET_DATE_END,

    ET_ETOKEN_STR,
};

struct ParseStatus
{
    const char* source;                 /*源文件内容, TODO::可改为string储存  */
    const char* parsePos;               /*当前解析位置*/
    int len;                            /*源文件长度*/

    std::string currentTokenStr;        /*当前所解析出的token的字符串*/
    EToken currentToken;                /*当前token*/

    ~ParseStatus()
    {
        free((void*)source);
        source = nullptr;
        parsePos = nullptr;
    }
};

struct keytoken
{
    const char* value;
    EToken      type;                   /*关键字token类型*/
    EToken      mateToken;              /*关键字所配对的关键字类型,譬如</string>配对的就是<string>，即 ET_STRING_START */
};

static keytoken keywords[] =
{
    { "<dict>", ET_DICT_START, ET_NONE },
    { "</dict>", ET_DICT_END, ET_DICT_START },
    { "<key>", ET_KEY_START, ET_NONE },
    { "</key>", ET_KEY_END, ET_KEY_START },
    { "<array>", ET_ARRAY_START, ET_NONE },
    { "</array>", ET_ARRAY_END, ET_ARRAY_START },
    { "<integer>", ET_INT_START, ET_NONE },
    { "</integer>", ET_INT_END, ET_INT_START },
    { "<true/>", ET_TRUE, ET_NONE },
    { "<false/>", ET_FALSE, ET_NONE },
    { "<string>", ET_STRING_START, ET_NONE },
    { "</string>", ET_STRING_END, ET_STRING_START },
    { "<real>", ET_REAL_START, ET_NONE },
    { "</real>", ET_REAL_END, ET_REAL_START },
    { "<data>", ET_DATA_START, ET_NONE },
    { "</data>", ET_DATA_END, ET_DATA_START },
    { "<date>", ET_DATE_START, ET_NONE },
    { "</date>", ET_DATE_END, ET_DATE_START }
};

static keytoken findKeytokenByKeywordType(EToken keyword)
{
    keytoken ret = { "", ET_NONE, ET_NONE };
    for (int i = 0; i < (sizeof(keywords) / sizeof(keytoken)); ++i)
    {
        if (keywords[i].mateToken == keyword)
        {
            ret = keywords[i];
            break;
        }
    }

    return ret;
}

static AnyValue* parseAny(ParseStatus&);
static AnyValue* parseDict(ParseStatus&);
static AnyValue* parseArray(ParseStatus&);
static AnyValue* parseInt(ParseStatus&);
static AnyValue* parseBool(ParseStatus&);
static AnyValue* parseString(ParseStatus&);
static AnyValue* parseReal(ParseStatus&);
static AnyValue* parseDate(ParseStatus&);
static AnyValue* parseData(ParseStatus&);

static string load_file(const char *filename)
{
    string content;

    FILE *fp = fopen(filename, "rb");
    if (fp != nullptr)
    {
        char* buffer = NULL;
        int file_size = 0;

        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buffer = (char*)malloc(file_size + 1);
        fread(buffer, file_size, 1, fp);
        buffer[file_size] = 0;
        fclose(fp);

        content = string(buffer, file_size + 1);
    }
    

    return std::move(content);
}

static const char* skipSpace(const char* input)
{
    while (true)
    {
        char ch = *input;

        if (ch != '\t' && ch != '\n' && ch != ' ' &&  ch != '\r')
        {
            break;
        }

        input++;
    }

    return input;
}

static void skipSpaceOfPS(ParseStatus& ps)
{
    if (ps.parsePos < (ps.source + ps.len))
    {
        ps.parsePos = skipSpace(ps.parsePos);
    }
}

static EToken getToken(ParseStatus& ps)
{
    skipSpaceOfPS(ps);
    EToken ret = ET_ETOKEN_STR;

    if (ps.parsePos >= (ps.source + ps.len))
    {
        ret = ET_EOF;
    }
    else if (*ps.parsePos == '<')
    {
        const char* find = strchr(ps.parsePos, '>');
        if (find != nullptr)
        {
            find++;
            string tmp(ps.parsePos, find);
            ps.parsePos = find;

            for (int i = 0; i < (sizeof(keywords) / sizeof(keytoken)); ++i)
            {
                if (strcmp(keywords[i].value, tmp.c_str()) == 0)
                {
                    ret = keywords[i].type;
                    break;
                }
            }
        }
    }
    else
    {
        /*  
            这里没直接查找<字符来定位下一个关键字，以获取(之间的)内容
            因为内容里也或许存在< （需要看plist的文法标准）， 所以这里需要去按字符串匹配当前关键字的配对关键字，然后获取其之间的内容    
            TODO::但这里进行查找操作之后，解析过程中又会进行一次查找(解析关键字)，需要优化~可否先一遍解析出plist文件的所有token，然后再做语法分析。
        */

        keytoken mateToken = findKeytokenByKeywordType(ps.currentToken);
        if (mateToken.mateToken != ET_NONE)
        {
            const char* find = strstr(ps.parsePos, mateToken.value);
            if (find != nullptr)
            {
                ps.currentTokenStr = string(ps.parsePos, find);
                ps.parsePos = find;
            }
        }
    }

    ps.currentToken = ret;
    return ret;
}

static AnyValue* parseAny(ParseStatus& ps)
{
    skipSpaceOfPS(ps);

    AnyValue* ret = nullptr;
    EToken token = getToken(ps);
    switch (token)
    {
        case ET_DICT_START:
            ret = parseDict(ps);
            break;
        case ET_ARRAY_START:
            ret = parseArray(ps);
            break;
        case ET_STRING_START:
            ret = parseString(ps);
            break;
        case ET_INT_START:
            ret = parseInt(ps);
            break;
        case ET_TRUE:
            ret = new PntBool(true);
            break;
        case ET_FALSE:
            ret = new PntBool(false);
            break;
        case ET_REAL_START:
            ret = parseReal(ps);
            break;
        case ET_DATE_START:
            ret = parseDate(ps);
            break;
        case ET_DATA_START:
            ret = parseData(ps);
            break;
        default:
            break;
    }

    return ret;
}

static void matchToken(ParseStatus& ps, EToken token)
{
    EToken now = getToken(ps);
    if (now != token)
    {
        assert(now == token);   /*  TODO:: throw Error("")  */
    }
}

static AnyValue* parseArray(ParseStatus& ps)
{
    skipSpaceOfPS(ps);
    PntArray* ret = new PntArray();
    while (true)
    {
        AnyValue* node = parseAny(ps);
        if (node != nullptr)
        {
            ret->addNode(node);
        }
        else
        {
            break;
        }
    }

    assert(ET_ARRAY_END == ps.currentToken);

    return ret;
}

static AnyValue* parseDict(ParseStatus& ps)
{
    skipSpaceOfPS(ps);

    PntDict* ret = new PntDict();
    EToken token;
    while (true)
    {
        skipSpaceOfPS(ps);
        token = getToken(ps);
        if (token == ET_KEY_START)
        {
            getToken(ps);
            string keyname = ps.currentTokenStr;
            matchToken(ps, ET_KEY_END);

            AnyValue* node = parseAny(ps);
            if (node != nullptr)
            {
                ret->addNode(node, keyname);
            }
        }
        else
        {
            break;
        }
    }

    assert(ps.currentToken == ET_DICT_END);
    return ret;
}

static AnyValue* parseInt(ParseStatus& ps)
{
    getToken(ps);
    string keyname = ps.currentTokenStr;
    int value;
    sscanf(keyname.c_str(), "%d", &value);
    matchToken(ps, ET_INT_END);
    AnyValue* ret = new PntInt(value);
    return ret;
}

static AnyValue* parseString(ParseStatus& ps)
{
    matchToken(ps, ET_ETOKEN_STR);
    AnyValue* ret = new PntString(ps.currentTokenStr);
    matchToken(ps, ET_STRING_END);
    return ret;
}

static AnyValue* parseReal(ParseStatus& ps)
{
    getToken(ps);
    string keyname = ps.currentTokenStr;
    double value = atof(keyname.c_str());
    matchToken(ps, ET_REAL_END);
    return new PntReal(value);
}

static AnyValue* parseDate(ParseStatus& ps)
{
    getToken(ps);
    string keyname = ps.currentTokenStr;
    /*TODO::根据字符串读取出时间*/
    time_t value = 0;
    matchToken(ps, ET_DATE_END);
    AnyValue* ret = new PntDate(value);
    return ret;
}

static AnyValue* parseData(ParseStatus& ps)
{
    matchToken(ps, ET_ETOKEN_STR);
    AnyValue* ret = new PntString(ps.currentTokenStr);
    matchToken(ps, ET_DATA_END);
    return ret;
}

AnyValue* parsePlist(const std::string& source)
{
    ParseStatus tmp;
    tmp.source = (char*)malloc(source.size() + 1);
    tmp.len = source.size() + 1;
    memcpy((char*)tmp.source, source.c_str(), source.size());
    ((char*)tmp.source)[tmp.len - 1] = '\0';
    tmp.parsePos = tmp.source;

    /*  TODO::跳过plist属性，直接定位到数据起始位置    */
    const char* find = strstr(tmp.parsePos, "plist version=");
    if (find != nullptr)
    {
        const char* findstart = strchr(find, '>');
        if (findstart != nullptr)
        {
            tmp.parsePos = findstart + 1;
        }
    }
    return parseAny(tmp);
}

AnyValue* parsePlistByFileName(const std::string& source)
{
    return parsePlist(load_file(source.c_str()));
}