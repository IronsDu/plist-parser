#ifndef _PLIST_H
#define _PLIST_H

#include <vector>
#include <map>

enum PLIST_NODE_TYPE
{
    PNT_INT,
    PNT_BOOL,
    PNT_STRING,
    PNT_REAL,
    PNT_DATE,
    PNT_ARRAY,
    PNT_DICT,
};

class PntInt;
class PntReal;
class PntBool;
class PntDate;
class PntString;
class PntArray;
class PntDict;

class AnyValue
{
public:
    AnyValue(PLIST_NODE_TYPE type) : mType(type){}
    virtual ~AnyValue(){}
    PLIST_NODE_TYPE     getType()
    {
        return mType;
    }

    PntInt*             asInt()
    {
        return (mType == PNT_INT) ? (PntInt*)(this) : nullptr;
    }
    PntReal*             asReal()
    {
        return (mType == PNT_REAL) ? (PntReal*)(this) : nullptr;
    }
    PntBool*             asBool()
    {
        return (mType == PNT_BOOL) ? (PntBool*)(this) : nullptr;
    }
    PntDate*             asDate()
    {
        return (mType == PNT_DATE) ? (PntDate*)(this) : nullptr;
    }
    PntString*             asString()
    {
        return (mType == PNT_STRING) ? (PntString*)(this) : nullptr;
    }
    PntArray*             asArray()
    {
        return (mType == PNT_ARRAY) ? (PntArray*)(this) : nullptr;
    }
    PntDict*             asDict()
    {
        return (mType == PNT_DICT) ? (PntDict*)(this) : nullptr;
    }
private:
    PLIST_NODE_TYPE     mType;
    /*  TODO::使用union来储存各种数据结构，去掉继承关系 */
};

class PntInt : public AnyValue
{
public:
    PntInt(int value) : AnyValue(PNT_INT), mValue(value){}
    int                 getValue()
    {
        return mValue;
    }
private:
    int                 mValue;
};

class PntReal : public AnyValue
{
public:
    PntReal(double value) : AnyValue(PNT_REAL), mValue(value){}
    double              getValue()
    {
        return mValue;
    }
private:
    double              mValue;
};

class PntBool : public AnyValue
{
public:
    PntBool(bool value) : AnyValue(PNT_BOOL), mValue(value){}
    bool                getValue()
    {
        return mValue;
    }
private:
    bool                mValue;
};

class PntDate : public AnyValue
{
public:
    PntDate(time_t value) : AnyValue(PNT_DATE), mValue(value){}

private:
    time_t              mValue;
};

class PntString : public AnyValue
{
public:
    PntString(const std::string& value) : AnyValue(PNT_STRING), mValue(value){}
    const std::string&       getValue()
    {
        return mValue;
    }
private:
    std::string         mValue;
};

class PntArray : public AnyValue
{
public:
    typedef std::vector<AnyValue*> VALUE_TYPE;
    PntArray() : AnyValue(PNT_ARRAY){}
    virtual ~PntArray() override;

    void                addNode(AnyValue* value)
    {
        mValue.push_back(value);
    }

    const VALUE_TYPE&  getValue()
    {
        return mValue;
    }
private:
    VALUE_TYPE          mValue;
};

class PntDict : public AnyValue
{
public:
    typedef std::map<std::string, AnyValue*> VALUE_TYPE;
    PntDict() : AnyValue(PNT_DICT){}
    virtual ~PntDict() override;

    void                    addNode(AnyValue* value, const std::string& key)
    {
        mValue[key] = value;
    }

    const VALUE_TYPE&   getValue()
    {
        return mValue;
    }
private:
    VALUE_TYPE          mValue;
};

AnyValue*    parsePlist(const std::string& source);
AnyValue*    parsePlistByFileName(const std::string& filename);

#endif